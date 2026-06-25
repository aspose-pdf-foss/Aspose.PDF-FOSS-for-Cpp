// =============================================================================
// page_renderer_dimensions_smoke_test — the device raster must (a) be sized by
// ceiling (not rounding) the page extent so it fully covers a fractional-point
// MediaBox, and (b) swap its width/height for a /Rotate 90|270 page so the
// displayed (landscape) page fills the raster. Both match the reference / the
// poppler oracle the validation tool diffs against (task_06 B4): the old
// std::lround under-covered fractional pages by 1px (e.g. A4 595.276pt high →
// FOSS 1240 vs poppler 1241) and a portrait raster was emitted for rotated
// pages (foss 1275x1650 vs poppler 1650x1275), both yielding large pixel-diffs.
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

// A 100 x 100.4-pt page at 72 dpi (scale 1) must produce a 100 x 101 raster:
// ceil(100.4) = 101 fully covers the page; std::lround would truncate to 100
// and leave the top user-space row off the raster (poppler renders 101).
TEST(PageRendererDimensions, CeilCoversFractionalPage) {
    const std::string cs = "0 0 1 rg\n0 0 100 100.4 re\nf\n";
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Count 1 /Kids [3 0 R] >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100.4] "
        "/Contents 4 0 R >>",
        "<< /Length " + std::to_string(cs.size()) + " >>\nstream\n" + cs +
            "endstream",
    });

    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(pdf.data(), pdf.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    EXPECT_EQ(out.width, 100u);
    EXPECT_EQ(out.height, 101u);
}

// A 200 x 100-pt page with /Rotate 90 at 72 dpi must produce a 100 x 200
// raster — the displayed page is rotated to portrait, so the device width is
// the page's y-extent and the device height its x-extent. The old code emitted
// 200 x 100 (the unrotated extents) and mis-scaled the content.
TEST(PageRendererDimensions, Rotate90SwapsDims) {
    const std::string cs = "0 1 0 rg\n0 0 200 100 re\nf\n";
    const auto pdf = Pdf({
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Count 1 /Kids [3 0 R] >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] "
        "/Rotate 90 /Contents 4 0 R >>",
        "<< /Length " + std::to_string(cs.size()) + " >>\nstream\n" + cs +
            "endstream",
    });

    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(pdf.data(), pdf.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 200u);

    // The full-page green fill must reach the raster centre — proves the CTM
    // maps content onto the swapped raster rather than off-canvas.
    const std::size_t o = (100u * out.width + 50u) * 4u;
    EXPECT_LT(out.pixels[o + 0], 60u);   // R low
    EXPECT_GT(out.pixels[o + 1], 200u);  // G high
    EXPECT_LT(out.pixels[o + 2], 60u);   // B low
}

}  // namespace
