#include "outlines.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <map>

#include "lexer.hpp"
#include "xref.hpp"
#include "objects.hpp"
#include "trailer.hpp"
#include "encoding_tables.hpp"
#include "agl.hpp"

namespace {

// Helper: decode a PDF text string (§7.9.2.2) to UTF-8.
std::string DecodeTitleString(const foundation::objects::String& str) {
    const auto& bytes = str.bytes;
    if (bytes.size() >= 2 &&
        static_cast<std::uint8_t>(bytes[0]) == 0xFE &&
        static_cast<std::uint8_t>(bytes[1]) == 0xFF) {
        // UTF-16BE BOM — decode rest as UTF-16BE → UTF-8
        std::string result;
        result.reserve((bytes.size() - 2) / 2);
        for (std::size_t i = 2; i + 1 < bytes.size(); i += 2) {
            std::uint16_t high = (static_cast<std::uint8_t>(bytes[i]) << 8) |
                                 static_cast<std::uint8_t>(bytes[i + 1]);
            if (high >= 0xD800 && high <= 0xDBFF) {
                // High surrogate — must have a low surrogate
                if (i + 3 >= bytes.size()) {
                    throw std::runtime_error("unpaired high surrogate in UTF-16BE string");
                }
                std::uint16_t low = (static_cast<std::uint8_t>(bytes[i + 2]) << 8) |
                                    static_cast<std::uint8_t>(bytes[i + 3]);
                if (low < 0xDC00 || low > 0xDFFF) {
                    throw std::runtime_error("unpaired high surrogate in UTF-16BE string");
                }
                std::uint32_t cp = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                i += 2;
            } else if (high >= 0xDC00 && high <= 0xDFFF) {
                throw std::runtime_error("unpaired low surrogate in UTF-16BE string");
            } else {
                if (high <= 0x7F) {
                    result.push_back(static_cast<char>(high));
                } else if (high <= 0x7FF) {
                    result.push_back(static_cast<char>(0xC0 | (high >> 6)));
                    result.push_back(static_cast<char>(0x80 | (high & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xE0 | (high >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((high >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (high & 0x3F)));
                }
            }
        }
        return result;
    }

    if (bytes.size() >= 3 &&
        static_cast<std::uint8_t>(bytes[0]) == 0xEF &&
        static_cast<std::uint8_t>(bytes[1]) == 0xBB &&
        static_cast<std::uint8_t>(bytes[2]) == 0xBF) {
        // UTF-8 BOM — strip and pass through
        return std::string(reinterpret_cast<const char*>(bytes.data() + 3),
                           bytes.size() - 3);
    }

    // PDFDocEncoding fallback
    std::string result;
    result.reserve(bytes.size());
    for (auto b : bytes) {
        std::string_view name = foundation::encoding_tables::ByteToGlyphName(
            foundation::encoding_tables::Encoding::PDFDocEncoding,
            static_cast<std::uint8_t>(b));
        if (name.empty()) {
            // Unmapped slot → Latin-1 fallback
            result.push_back(static_cast<char>(static_cast<std::uint8_t>(b)));
        } else {
            auto codepoints = foundation::agl::Lookup(name);
            for (std::uint32_t cp : codepoints) {
                if (cp <= 0x7F) {
                    result.push_back(static_cast<char>(cp));
                } else if (cp <= 0x7FF) {
                    result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else if (cp <= 0xFFFF) {
                    result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                    result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
            }
        }
    }
    return result;
}

// Resolve a Ref to an objects::Dict, throwing on failure.
foundation::objects::Dict ResolveDict(const foundation::objects::Ref& ref,
                                      const foundation::objects::Dump& dump) {
    for (const auto& obj : dump.objects) {
        if (obj.id == ref.id && obj.generation == ref.generation) {
            if (std::holds_alternative<foundation::objects::Dict>(obj.value.v)) {
                return std::get<foundation::objects::Dict>(obj.value.v);
            }
            throw std::runtime_error("outline item is not a dictionary");
        }
    }
    throw std::runtime_error("outline item reference not found");
}

// Visit an outline item and its siblings in preorder, respecting depth.
void VisitOutlineItem(const foundation::objects::Ref& item_ref,
                      std::uint32_t depth,
                      std::vector<foundation::outlines::OutlineItem>& items,
                      const foundation::objects::Dump& dump,
                      std::size_t depth_limit) {
    if (depth > depth_limit) {
        throw std::runtime_error("outline cycle detected or depth limit exceeded");
    }

    foundation::objects::Dict item = ResolveDict(item_ref, dump);

    // /Title is required for items (§12.3.3)
    std::string title;
    bool found_title = false;
    for (const auto& entry : item.entries) {
        if (entry.first == "Title") {
            if (std::holds_alternative<foundation::objects::String>(entry.second.v)) {
                title = DecodeTitleString(std::get<foundation::objects::String>(entry.second.v));
                found_title = true;
            } else if (std::holds_alternative<std::string>(entry.second.v)) {
                // PDFDocEncoding string (rare but allowed)
                const std::string& str_val = std::get<std::string>(entry.second.v);
                foundation::objects::String s;
                s.bytes.resize(str_val.size());
                for (size_t i = 0; i < str_val.size(); ++i) {
                    s.bytes[i] = static_cast<std::byte>(static_cast<unsigned char>(str_val[i]));
                }
                title = DecodeTitleString(s);
                found_title = true;
            } else {
                throw std::runtime_error("/Title must be a string");
            }
            break;
        }
    }
    if (!found_title) {
        throw std::runtime_error("/Title missing on outline item");
    }

    items.push_back({depth, title});

    // Recurse into children via /First
    foundation::objects::Ref first_ref{};
    bool has_first = false;
    for (const auto& entry : item.entries) {
        if (entry.first == "First") {
            if (std::holds_alternative<foundation::objects::Ref>(entry.second.v)) {
                first_ref = std::get<foundation::objects::Ref>(entry.second.v);
                has_first = true;
            }
            break;
        }
    }

    if (has_first) {
        VisitOutlineItem(first_ref, depth + 1, items, dump, depth_limit);
    }

    // Walk siblings via /Next
    foundation::objects::Ref next_ref{};
    bool has_next = false;
    for (const auto& entry : item.entries) {
        if (entry.first == "Next") {
            if (std::holds_alternative<foundation::objects::Ref>(entry.second.v)) {
                next_ref = std::get<foundation::objects::Ref>(entry.second.v);
                has_next = true;
            }
            break;
        }
    }

    if (has_next) {
        VisitOutlineItem(next_ref, depth, items, dump, depth_limit);
    }
}

} // namespace

namespace foundation::outlines {

Outline Parse(std::span<const std::byte> input) {
    // Parse the PDF objects
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(input);
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to parse PDF objects: " + std::string(e.what()));
    }

    // Parse the trailer to get the root reference
    foundation::trailer::Trailer trailer;
    try {
        trailer = foundation::trailer::Parse(input);
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to parse trailer: " + std::string(e.what()));
    }

    // Find the root dictionary
    foundation::objects::Dict root{};
    bool found_root = false;
    for (const auto& obj : dump.objects) {
        if (obj.id == trailer.root_id && obj.generation == 0) {
            if (std::holds_alternative<foundation::objects::Dict>(obj.value.v)) {
                root = std::get<foundation::objects::Dict>(obj.value.v);
                found_root = true;
            }
            break;
        }
    }
    if (!found_root) {
        throw std::runtime_error("catalog (root) object not found");
    }

    // Look for /Outlines in the root dictionary
    foundation::objects::Ref outlines_ref{};
    bool has_outlines = false;
    for (const auto& entry : root.entries) {
        if (entry.first == "Outlines") {
            if (std::holds_alternative<foundation::objects::Ref>(entry.second.v)) {
                outlines_ref = std::get<foundation::objects::Ref>(entry.second.v);
                has_outlines = true;
            } else if (std::holds_alternative<foundation::objects::Dict>(entry.second.v)) {
                // /Outlines may be a dict directly (older PDFs)
                // Treat as a root dict with /First and /Last
                foundation::objects::Dict outlines_dict = std::get<foundation::objects::Dict>(entry.second.v);
                foundation::objects::Ref first_ref{};
                bool has_first = false;
                for (const auto& e : outlines_dict.entries) {
                    if (e.first == "First") {
                        if (std::holds_alternative<foundation::objects::Ref>(e.second.v)) {
                            first_ref = std::get<foundation::objects::Ref>(e.second.v);
                            has_first = true;
                        }
                        break;
                    }
                }
                if (!has_first) {
                    return Outline{}; // No items
                }
                Outline result;
                VisitOutlineItem(first_ref, 0, result.items, dump, 1000);
                return result;
            } else {
                throw std::runtime_error("/Outlines must be a reference or dictionary");
            }
            break;
        }
    }

    // If /Outlines is absent, return empty outline (not an error)
    if (!has_outlines) {
        return Outline{};
    }

    // Resolve the /Outlines dictionary
    foundation::objects::Dict outlines_dict = ResolveDict(outlines_ref, dump);

    // Look for /First in the outlines dictionary
    foundation::objects::Ref first_ref{};
    bool has_first = false;
    for (const auto& entry : outlines_dict.entries) {
        if (entry.first == "First") {
            if (std::holds_alternative<foundation::objects::Ref>(entry.second.v)) {
                first_ref = std::get<foundation::objects::Ref>(entry.second.v);
                has_first = true;
            }
            break;
        }
    }

    // If /First is absent, there are no items
    if (!has_first) {
        return Outline{};
    }

    // Walk the outline tree
    Outline result;
    VisitOutlineItem(first_ref, 0, result.items, dump, 1000);
    return result;
}

std::string ToCanonical(const Outline& outline) {
    if (outline.items.empty()) {
        return "";
    }

    std::string result;
    for (const auto& item : outline.items) {
        result += std::to_string(item.depth) + "\t" + item.title + "\n";
    }
    return result;
}

} // namespace foundation::outlines
