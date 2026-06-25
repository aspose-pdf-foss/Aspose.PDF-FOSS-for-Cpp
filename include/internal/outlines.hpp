#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::outlines {

// One outline item in preorder traversal of the outline
// tree. depth is 0 for top-level items, 1 for first-level
// children, and so on.
struct OutlineItem {
    std::uint32_t depth;
    std::string   title;
};

// Outline tree as a flat preorder list. Empty when the
// PDF has no /Outlines entry on its Catalog.
struct Outline {
    std::vector<OutlineItem> items;
};

Outline Parse(std::span<const std::byte> input);
std::string ToCanonical(const Outline& outline);

}
