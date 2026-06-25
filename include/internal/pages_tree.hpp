#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::pages_tree {

// One leaf page with geometry resolved down the /Pages
// tree (inheritance from /Pages parents applied).
struct LeafPage {
    std::uint32_t id;            // /Page leaf indirect-object id
    std::uint64_t pagenum;       // 1-based preorder position
    double width;                // /MediaBox (urx - llx)
    double height;               // /MediaBox (ury - lly)
    std::int32_t rotation;       // {0, 90, 180, 270}
};

// Preorder-sorted list of every leaf reachable from the
// page-tree root (looked up via Catalog.Pages).
struct Tree {
    std::vector<LeafPage> leaves;
};

// Parse `input` end-to-end: find the Catalog via
// foundation::trailer, read its /Pages entry, walk the
// resulting tree. Returns every leaf with resolved
// geometry. Throws std::runtime_error on the error
// conditions enumerated in the shared anchor.
Tree Parse(std::span<const std::byte> input);

// Freeze-gate comparison form — one line per leaf,
// preorder, newline-terminated:
//     "page <pagenum> <width> <height> <rotation>\n"
// width / height: integer when exact-integer-valued,
//                 otherwise "%g" format.
// rotation: always 0 / 90 / 180 / 270.
std::string ToCanonical(const Tree& tree);

}
