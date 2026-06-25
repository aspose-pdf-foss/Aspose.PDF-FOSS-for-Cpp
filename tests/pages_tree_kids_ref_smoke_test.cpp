// =============================================================================
// pages_tree_kids_ref_smoke_test — a /Pages node whose /Kids is an indirect
// reference (`/Kids 4 0 R`) resolves, rather than being rejected with
// "/Kids is not an array" (task_06 A4, reproducer 44825.pdf). Mirrors the v3
// dereference already applied to /MediaBox and /Rotate.
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

TEST(PagesTreeKidsRefSmoke, IndirectKidsArrayResolves) {
    // Object 2 (Pages) points /Kids at object 4, the array literal.
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",                       // 1
        "<< /Type /Pages /Count 1 /Kids 4 0 R >>",                 // 2
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] >>", // 3
        "[ 3 0 R ]",                                               // 4: /Kids
    });

    const auto tree = foundation::pages_tree::Parse(
        std::span<const std::byte>(pdf.data(), pdf.size()));

    ASSERT_EQ(tree.leaves.size(), 1u);
    EXPECT_EQ(tree.leaves[0].id, 3u);
}

}  // namespace
