// =============================================================================
// page_renderer_mediabox_origin_smoke_test — content drawn against a non-zero
// (here negative) MediaBox origin must map onto the raster, not off-canvas
// (task_06 B2; the 26944-* CMS-1500 forms use MediaBox [-90 -1008 702 216] and
// rendered almost entirely blank when the origin was dropped).
// =============================================================================

#include "page_renderer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> Pdf(const std::vector<std::string>& bodies) {
    std::string s = "%PDF-1.5\n";
    for (std::size_t i = 0; i < bodies.size(); ++i)
        s += std::to_string(i + 1) + " 0 obj\n" + bodies[i] + "\nendobj\n";
    s += "trailer\n<</Root 1 0 R>>\nstartxref\n0\n%%EOF\n";
    std::vector<std::byte> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        out[i] = static_cast<std::byte>(s[i]);
    return out;
}

// MediaBox [-50 -50 50 50] (100x100, origin -50,-50). The content fills the
// user-space rect (-50,-50)-(0,0) — the lower-left box quadrant — in red.
// With the origin honoured that quadrant lands at device x in [0,50],
// y in [50,100] (PDF y-up flipped); the opposite quadrant stays white. With
// the origin dropped the rect is pushed off-canvas and the page is blank.
TEST(PageRendererMediaBoxOrigin, NegativeOriginContentOnCanvas) {
    const std::string cs = "1 0 0 rg\n-50 -50 50 50 re\nf\n";
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Count 1 /Kids [3 0 R] >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [-50 -50 50 50] "
        "/Contents 4 0 R >>",
        "<< /Length " + std::to_string(cs.size()) + " >>\nstream\n" + cs +
            "endstream",
    });

    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(pdf.data(), pdf.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{out.pixels[o], out.pixels[o + 1],
                                           out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Lower-left user quadrant -> device top-left-ish (x~25, y~75): red.
    const auto in = px(25, 75);
    EXPECT_GT(in[0], 200u) << "filled quadrant R";
    EXPECT_LT(in[1], 60u)  << "filled quadrant G";
    EXPECT_LT(in[2], 60u)  << "filled quadrant B";
    // Opposite quadrant stays white.
    const auto out_q = px(75, 25);
    EXPECT_GT(out_q[0], 230u);
    EXPECT_GT(out_q[1], 230u);
    EXPECT_GT(out_q[2], 230u);
}

}  // namespace
