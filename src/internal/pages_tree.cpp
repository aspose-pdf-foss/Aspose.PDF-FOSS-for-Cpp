#include "pages_tree.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <span>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cmath>
#include <optional>

#include "xref.hpp"
#include "objects.hpp"

namespace foundation::pages_tree {

namespace {

using foundation::objects::Value;
using foundation::objects::Array;
using foundation::objects::Dict;
using foundation::objects::Ref;
using foundation::objects::Dump;
using foundation::objects::IndirectObject;

// Object graph indexed by (id, generation) so every resolution is a
// keyed lookup rather than a linear scan of the dump. A flat /Pages
// node can name hundreds of thousands of pages over millions of
// objects; scanning per lookup makes the walk O(pages x objects).
using ObjIndex = std::unordered_map<std::uint64_t, const IndirectObject*>;

inline std::uint64_t ObjKey(std::uint32_t id, std::uint16_t generation) {
    return (static_cast<std::uint64_t>(id) << 16) |
           static_cast<std::uint64_t>(generation);
}

// Helper forward declarations. Must live in the SAME anonymous
// namespace as the definitions (below, line ~170). An earlier
// version put these in a file-level anonymous
// namespace above foundation::pages_tree, which gave them
// distinct internal linkage and the linker couldn't match
// them against the definitions inside
// foundation::pages_tree::{anonymous}.
void ResolvePagesNode(const Value& node_value,
                      std::uint32_t node_id,
                      std::uint64_t& next_pagenum,
                      std::vector<foundation::pages_tree::LeafPage>& leaves,
                      double inherited_media[4],
                      std::int32_t inherited_rotation,
                      const ObjIndex& index,
                      std::map<std::pair<std::uint32_t, std::uint16_t>, bool>& visited);

void ParseMediaBox(const Value& media_box_value, double out[4]);

std::int32_t NormalizeRotation(std::int64_t raw_rotation);

std::string FormatNumber(double value);

std::optional<std::pair<std::uint32_t, std::uint16_t>> GetRefFromValue(const Value& v);

const Value* Deref(const Value& v, const ObjIndex& index);

} // namespace

Tree Parse(std::span<const std::byte> input) {
    // Step 1: Resolve the root object id via the cross-reference chain.
    // xref v2 follows the startxref offset (+ /Prev) to find /Root even when
    // the file's EOF trailer omits it — linearized / incremental files — which
    // the byte-scanning `trailer` primitive could not. xref::Table exposes
    // root_id directly, and xref::Parse throws if the chain has no /Root.
    xref::Table t = xref::Parse(input);

    if (t.root_id == 0) {
        throw std::runtime_error("Catalog object id is zero");
    }

    // Step 2: Parse objects to get full object graph
    Dump dump = objects::Parse(input);

    // Step 2b: Index the graph by (id, generation). First object in
    // dump order wins on a duplicate key (matches a first-match scan).
    ObjIndex index;
    index.reserve(dump.objects.size());
    for (const auto& obj : dump.objects) {
        index.emplace(ObjKey(obj.id, obj.generation), &obj);
    }

    // Step 3: Find the Catalog object
    const IndirectObject* catalog_obj = nullptr;
    if (auto it = index.find(ObjKey(t.root_id, 0)); it != index.end()) {
        catalog_obj = it->second;
    }

    if (!catalog_obj) {
        throw std::runtime_error("Catalog object not found");
    }

    // Step 4: Catalog must be a Dict
    const Dict* catalog_dict = std::get_if<Dict>(&catalog_obj->value.v);
    if (!catalog_dict) {
        throw std::runtime_error("Catalog is not a dictionary");
    }

    // Step 5: Find /Pages entry in Catalog
    const Value* pages_value = nullptr;
    for (const auto& entry : catalog_dict->entries) {
        if (entry.first == "Pages") {
            pages_value = &entry.second;
            break;
        }
    }

    if (!pages_value) {
        throw std::runtime_error("Catalog missing /Pages entry");
    }

    // Step 6: /Pages must be a Ref
    const Ref* pages_ref = std::get_if<Ref>(&pages_value->v);
    if (!pages_ref) {
        throw std::runtime_error("/Pages entry is not an indirect reference");
    }

    // Step 7: Find the page-tree root object
    const IndirectObject* pages_root_obj = nullptr;
    if (auto it = index.find(ObjKey(pages_ref->id, pages_ref->generation));
        it != index.end()) {
        pages_root_obj = it->second;
    }

    if (!pages_root_obj) {
        throw std::runtime_error("Page-tree root object not found");
    }

    // Step 8: Page-tree root must be a Dict
    const Dict* pages_root_dict = std::get_if<Dict>(&pages_root_obj->value.v);
    if (!pages_root_dict) {
        throw std::runtime_error("Page-tree root is not a dictionary");
    }

    // Step 9: Check /Type is /Pages
    const Value* type_value = nullptr;
    for (const auto& entry : pages_root_dict->entries) {
        if (entry.first == "Type") {
            type_value = &entry.second;
            break;
        }
    }

    // §7.7.3.2 requires the page-tree root to be /Type /Pages, but many
    // real PDFs omit /Type (or set it wrong) on the root. Accept the node
    // when it is structurally a Pages node — i.e. it carries /Kids — and
    // only reject a root that is neither typed /Pages nor has /Kids.
    const std::string* type_str =
        type_value ? std::get_if<std::string>(&type_value->v) : nullptr;
    const bool typed_pages = type_str && *type_str == "Pages";
    bool root_has_kids = false;
    for (const auto& entry : pages_root_dict->entries) {
        if (entry.first == "Kids") { root_has_kids = true; break; }
    }
    if (!typed_pages && !root_has_kids) {
        throw std::runtime_error(
            "Page-tree root is neither /Type /Pages nor has /Kids");
    }

    // Step 10: Initialize state and walk the tree
    Tree result;
    std::uint64_t next_pagenum = 1;
    std::vector<foundation::pages_tree::LeafPage>& leaves = result.leaves;

    // Inherited state: MediaBox defaults to empty (will be checked later)
    double inherited_media[4] = {0.0, 0.0, 0.0, 0.0};
    std::int32_t inherited_rotation = 0;

    std::map<std::pair<std::uint32_t, std::uint16_t>, bool> visited;
    ResolvePagesNode(pages_root_obj->value, pages_ref->id, next_pagenum, leaves,
                     inherited_media, inherited_rotation, index, visited);

    return result;
}

std::string ToCanonical(const Tree& tree) {
    std::string result;
    for (const auto& leaf : tree.leaves) {
        char buf[256];
        int n = std::snprintf(buf, sizeof(buf),
                              "page %lu %s %s %d\n",
                              static_cast<unsigned long>(leaf.pagenum),
                              FormatNumber(leaf.width).c_str(),
                              FormatNumber(leaf.height).c_str(),
                              leaf.rotation);
        if (n > 0 && static_cast<size_t>(n) < sizeof(buf)) {
            result.append(buf, n);
        }
    }
    return result;
}

namespace {

std::optional<std::pair<std::uint32_t, std::uint16_t>> GetRefFromValue(const Value& v) {
    const Ref* ref = std::get_if<Ref>(&v.v);
    if (ref) {
        return std::make_pair(ref->id, ref->generation);
    }
    return std::nullopt;
}

// Resolve an indirect reference to the concrete value it names. Follows a
// short ref->ref chain (bounded, cycle-guarded). Returns &v when v is not a
// reference, or nullptr when a hop names an object absent from the dump.
const Value* Deref(const Value& v, const ObjIndex& index) {
    const Value* cur = &v;
    std::map<std::pair<std::uint32_t, std::uint16_t>, bool> seen;
    for (int hops = 0; hops < 8; ++hops) {
        auto ref = GetRefFromValue(*cur);
        if (!ref) return cur;                 // concrete value
        if (seen.count(*ref)) return nullptr; // cycle
        seen[*ref] = true;
        auto it = index.find(ObjKey(ref->first, ref->second));
        if (it == index.end()) return nullptr; // dangling reference
        cur = &it->second->value;
    }
    return nullptr;                           // chain too long
}

void ResolvePagesNode(const Value& node_value,
                      std::uint32_t node_id,
                      std::uint64_t& next_pagenum,
                      std::vector<foundation::pages_tree::LeafPage>& leaves,
                      double inherited_media[4],
                      std::int32_t inherited_rotation,
                      const ObjIndex& index,
                      std::map<std::pair<std::uint32_t, std::uint16_t>, bool>& visited) {
    const Dict* node_dict = std::get_if<Dict>(&node_value.v);
    if (!node_dict) {
        throw std::runtime_error("Page tree node is not a dictionary");
    }

    // Check for /Type
    const Value* type_value = nullptr;
    for (const auto& entry : node_dict->entries) {
        if (entry.first == "Type") {
            type_value = &entry.second;
            break;
        }
    }

    // Check for /Kids
    const Value* kids_value = nullptr;
    for (const auto& entry : node_dict->entries) {
        if (entry.first == "Kids") {
            kids_value = &entry.second;
            break;
        }
    }

    // Node type: prefer a valid /Type, but many PDFs omit it (or set it
    // wrong) — infer from structure, a node with /Kids being an
    // intermediate /Pages node and otherwise a leaf /Page (§7.7.3).
    std::string node_type;
    if (type_value) {
        const std::string* type_str = std::get_if<std::string>(&type_value->v);
        if (type_str && (*type_str == "Pages" || *type_str == "Page")) {
            node_type = *type_str;
        }
    }
    if (node_type.empty()) {
        node_type = kids_value ? "Pages" : "Page";
    }

    if (node_type == "Pages") {
        // Page tree node: recurse into kids. /Kids is frequently an
        // indirect reference to the array (e.g. `/Kids 4 0 R`), so
        // resolve it through the object map before type-checking — the
        // same dereference the v3 node applies to /MediaBox and /Rotate.
        if (!kids_value) {
            throw std::runtime_error("/Pages node missing /Kids");
        }
        const Value* kids_resolved = Deref(*kids_value, index);
        const Array* kids_array = kids_resolved
            ? std::get_if<Array>(&kids_resolved->v)
            : nullptr;
        if (!kids_array) {
            throw std::runtime_error("/Kids is not an array");
        }

        // Inherit /MediaBox and /Rotate if present
        double local_media[4];
        std::int32_t local_rotation = inherited_rotation;

        // Copy inherited media
        for (int i = 0; i < 4; ++i) {
            local_media[i] = inherited_media[i];
        }

        // Check for /MediaBox override
        const Value* media_box_value = nullptr;
        for (const auto& entry : node_dict->entries) {
            if (entry.first == "MediaBox") {
                media_box_value = &entry.second;
                break;
            }
        }

        if (media_box_value) {
            const Value* mb = Deref(*media_box_value, index);
            if (mb) {                          // dangling ref -> keep inherited
                double new_media[4];
                ParseMediaBox(*mb, new_media);
                for (int i = 0; i < 4; ++i) {
                    local_media[i] = new_media[i];
                }
            }
        }

        // Check for /Rotate override
        const Value* rotate_value = nullptr;
        for (const auto& entry : node_dict->entries) {
            if (entry.first == "Rotate") {
                rotate_value = &entry.second;
                break;
            }
        }

        if (rotate_value) {
            const Value* rv = Deref(*rotate_value, index);
            const std::int64_t* rot_int =
                rv ? std::get_if<std::int64_t>(&rv->v) : nullptr;
            if (rot_int) {
                local_rotation = NormalizeRotation(*rot_int);
            }
        }

        // Recurse into each kid
        for (const auto& kid : kids_array->items) {
            auto kid_ref_opt = GetRefFromValue(kid);
            if (!kid_ref_opt) {
                throw std::runtime_error("/Kids entry is not an indirect reference");
            }

            auto key = *kid_ref_opt;

            // Cycle detection
            if (visited.count(key)) {
                throw std::runtime_error("Cycle detected in page tree");
            }

            // Find the kid object
            const IndirectObject* kid_obj = nullptr;
            if (auto it = index.find(ObjKey(key.first, key.second));
                it != index.end()) {
                kid_obj = it->second;
            }

            if (!kid_obj) {
                throw std::runtime_error("Page tree kid object not found");
            }

            // Mark visited
            visited[key] = true;

            // Recurse
            ResolvePagesNode(kid_obj->value, key.first, next_pagenum, leaves,
                             local_media, local_rotation, index, visited);

            // Unmark visited
            visited.erase(key);
        }
    } else if (node_type == "Page") {
        // Leaf page: emit a LeafPage
        // Check for /MediaBox
        const Value* media_box_value = nullptr;
        for (const auto& entry : node_dict->entries) {
            if (entry.first == "MediaBox") {
                media_box_value = &entry.second;
                break;
            }
        }

        double final_media[4];
        bool has_media = false;

        const Value* mb =
            media_box_value ? Deref(*media_box_value, index) : nullptr;
        if (mb) {
            ParseMediaBox(*mb, final_media);
            has_media = true;
        } else {
            // No own MediaBox (absent, or a dangling ref): use inherited.
            for (int i = 0; i < 4; ++i) {
                final_media[i] = inherited_media[i];
            }
            has_media = (inherited_media[0] != 0.0 || inherited_media[1] != 0.0 ||
                         inherited_media[2] != 0.0 || inherited_media[3] != 0.0);
        }

        if (!has_media) {
            throw std::runtime_error("Leaf page has no /MediaBox in inheritance chain");
        }

        // Compute width and height as the ABSOLUTE span of the box. A
        // /MediaBox defines a rectangle by two diagonal corners in any
        // order (PDF 32000-1 §7.9.5), so a box authored upper-left first
        // (e.g. [0 792 612 0]) is legal; the signed difference would give a
        // negative extent that collapses the raster to 1px.
        double width = std::abs(final_media[2] - final_media[0]);
        double height = std::abs(final_media[3] - final_media[1]);

        // Normalize rotation. A leaf's OWN /Rotate overrides the
        // inherited value (an earlier version checked /Rotate
        // only in the /Pages branch; this adds the symmetric
        // check on the /Page leaf so a leaf-declared /Rotate wins).
        std::int32_t rotation = inherited_rotation;
        for (const auto& entry : node_dict->entries) {
            if (entry.first == "Rotate") {
                const Value* rv = Deref(entry.second, index);
                const std::int64_t* rot_int =
                    rv ? std::get_if<std::int64_t>(&rv->v) : nullptr;
                if (rot_int) {
                    rotation = NormalizeRotation(*rot_int);
                }
                break;
            }
        }

        // Create LeafPage
        LeafPage leaf;
        leaf.pagenum = next_pagenum++;

        // The object id is the ref by which this node was reached —
        // threaded down the walk, so no scan of the dump is needed.
        leaf.id = node_id;

        leaf.width = width;
        leaf.height = height;
        leaf.rotation = rotation;

        leaves.push_back(leaf);
    } else {
        throw std::runtime_error("Unknown page tree node type: " + node_type);
    }
}

void ParseMediaBox(const Value& media_box_value, double out[4]) {
    const Array* media_box_array = std::get_if<Array>(&media_box_value.v);
    if (!media_box_array || media_box_array->items.size() != 4) {
        throw std::runtime_error("/MediaBox is not a 4-element array");
    }

    for (int i = 0; i < 4; ++i) {
        const Value& item = media_box_array->items[i];
        if (const std::int64_t* int_val = std::get_if<std::int64_t>(&item.v)) {
            out[i] = static_cast<double>(*int_val);
        } else if (const double* double_val = std::get_if<double>(&item.v)) {
            out[i] = *double_val;
        } else {
            throw std::runtime_error("/MediaBox element is not numeric");
        }
    }
}

std::int32_t NormalizeRotation(std::int64_t raw_rotation) {
    // Normalize to multiple of 90 and reduce to [0, 360)
    // Per §7.7.3.4: "The value shall be a numeric object representing the number
    // of degrees by which the page should be rotated clockwise when displayed or printed."
    // Non-multiples are snapped to nearest multiple of 90.

    // Round to nearest multiple of 90
    double normalized = std::round(static_cast<double>(raw_rotation) / 90.0) * 90.0;

    // Reduce to [0, 360)
    normalized = std::fmod(normalized, 360.0);
    if (normalized < 0) {
        normalized += 360.0;
    }

    return static_cast<std::int32_t>(normalized);
}

std::string FormatNumber(double value) {
    // Integer when exact-integer-valued; otherwise "%g" format.
    if (value == static_cast<double>(static_cast<std::int64_t>(value))) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(value));
        return std::string(buf);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%g", value);
        return std::string(buf);
    }
}

} // namespace

} // namespace foundation::pages_tree
