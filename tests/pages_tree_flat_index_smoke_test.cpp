// =============================================================================
// pages_tree_flat_index_smoke_test — a flat /Pages node resolves each leaf by
// keyed object lookup and records each leaf's own id from the ref it was
// reached by, NOT by a position scan of the object dump. Guards against the
// O(pages x objects) walk that made a 400k-page document (33712-5) hang.
//
// The /Kids list is deliberately NOT in object-number order and a decoy
// object is interleaved, so a correct walk must (a) find each kid by key
// regardless of dump position and (b) preserve preorder = /Kids order with the
// right per-leaf id.
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

TEST(PagesTreeFlatIndex, KidsOutOfOrderResolveByKey) {
    // obj2 = Pages; /Kids names the page objects out of object-number order,
    // with a decoy (obj7) sitting in the dump that must never be visited.
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",                        // 1
        "<< /Type /Pages /Count 4 /Kids [4 0 R 3 0 R 6 0 R 5 0 R] >>", // 2
        "<< /Type /Page /MediaBox [0 0 10 10] >>",                  // 3
        "<< /Type /Page /MediaBox [0 0 20 20] >>",                  // 4
        "<< /Type /Page /MediaBox [0 0 30 30] >>",                  // 5
        "<< /Type /Page /MediaBox [0 0 40 40] >>",                  // 6
        "<< /Type /Decoy >>",                                       // 7
    });

    const auto tree = foundation::pages_tree::Parse(
        std::span<const std::byte>(pdf.data(), pdf.size()));

    ASSERT_EQ(tree.leaves.size(), 4u);

    // Preorder follows /Kids order: 4, 3, 6, 5.
    EXPECT_EQ(tree.leaves[0].id, 4u);
    EXPECT_EQ(tree.leaves[1].id, 3u);
    EXPECT_EQ(tree.leaves[2].id, 6u);
    EXPECT_EQ(tree.leaves[3].id, 5u);

    // 1-based pagenums in preorder.
    EXPECT_EQ(tree.leaves[0].pagenum, 1u);
    EXPECT_EQ(tree.leaves[3].pagenum, 4u);

    // Geometry tracks the kid each leaf id resolved to (id N -> width).
    EXPECT_DOUBLE_EQ(tree.leaves[0].width, 20.0);  // obj4
    EXPECT_DOUBLE_EQ(tree.leaves[1].width, 10.0);  // obj3
    EXPECT_DOUBLE_EQ(tree.leaves[2].width, 40.0);  // obj6
    EXPECT_DOUBLE_EQ(tree.leaves[3].width, 30.0);  // obj5
}

}  // namespace
