// =============================================================================
// pages_tree_reversed_mediabox_smoke_test — a /MediaBox whose two diagonal
// corners are given in reversed order (upper-left then lower-right, e.g.
// [0 792 612 0]) is legal: PDF 32000-1 §7.9.5 / 14.11.2 define a rectangle by
// any two diagonal points, not necessarily lower-left then upper-right. The
// leaf extent must be the absolute span (|x2-x0|, |y3-y1|), not the signed
// difference — otherwise a reversed box yields a negative height that collapses
// the raster to 1px (task_06 B4; reproducers 20163 / 23237_input_XFA /
// 32503 / 30979 / 41930 rendered 1275x1 instead of 1275x1650).
// =============================================================================

#include "pages_tree.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> Pdf(const std::vector<std::string>& bodies) {
    std::string s = "%PDF-1.5\n";
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        s += std::to_string(i + 1) + " 0 obj\n" + bodies[i] + "\nendobj\n";
    }
    s += "trailer\n<</Root 1 0 R>>\nstartxref\n0\n%%EOF\n";
    std::vector<std::byte> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        out[i] = static_cast<std::byte>(s[i]);
    return out;
}

TEST(PagesTreeReversedMediaBox, ReversedCornersYieldPositiveExtent) {
    // MediaBox [0 792 612 0] — upper-left corner first, lower-right second.
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",                         // 1
        "<< /Type /Pages /Count 1 /Kids [3 0 R] >>",                 // 2
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 792 612 0] >>",   // 3
    });

    const auto tree = foundation::pages_tree::Parse(
        std::span<const std::byte>(pdf.data(), pdf.size()));

    ASSERT_EQ(tree.leaves.size(), 1u);
    EXPECT_DOUBLE_EQ(tree.leaves[0].width, 612.0);
    EXPECT_DOUBLE_EQ(tree.leaves[0].height, 792.0);
}

}  // namespace
