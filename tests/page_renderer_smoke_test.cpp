// =============================================================================
// page_renderer_smoke_test — exercises foundation::page_renderer::Render
// against the existing fixture corpus. Asserts the function produces a
// non-empty RGBA8 buffer of the expected dimensions for each fixture
// PDF.
//
// This is NOT the freeze gate — full PSNR-tolerant comparison against
// mutool draw + pdftocairo lands in a follow-up beat once canonical
// sidecars are generated. This smoke confirms the recovered body
// compiles + runs end-to-end without crashing on the corpus.
// =============================================================================

#include "page_renderer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <utility>
#include <fstream>
#include <span>
#include <vector>
#include <stdexcept>

namespace {

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "page_renderer";
}

std::vector<std::byte> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = f.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

}  // namespace

// ---------------------------------------------------------------------------
// blank.pdf renders to a non-empty buffer of the expected pixel size.
// At 100 user-units × 100 user-units × 72 DPI the output is 100×100 px,
// 4 bytes per pixel = 40 000 bytes.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, Blank_RendersExpectedDimensions) {
    const auto pdf = FixtureRoot() / "blank.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    ASSERT_FALSE(bytes.empty());

    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0,
        /*target_dpi=*/72.0);

    EXPECT_EQ(out.width, 100u);
    EXPECT_EQ(out.height, 100u);
    EXPECT_EQ(out.pixels.size(),
              static_cast<std::size_t>(out.width) * out.height * 4u);
}

// ---------------------------------------------------------------------------
// red_rect.pdf: same dimensions as blank, but the centre pixel ought
// to be red (the 50×50 painted rect is centred at (50, 50)).
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, RedRect_CentrePixelIsRed) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0,
        /*target_dpi=*/72.0);

    ASSERT_GT(out.pixels.size(), 0u);
    const std::size_t cx = out.width / 2;
    const std::size_t cy = out.height / 2;
    const std::size_t off = (cy * out.width + cx) * 4u;
    ASSERT_LT(off + 3u, out.pixels.size());

    // RGBA8 red — exact (255, 0, 0, 255) since rasterizer.Fill emits
    // the fill colour at full opacity for non-edge pixels.
    EXPECT_GT(out.pixels[off], 200u)     << "R channel";
    EXPECT_LT(out.pixels[off + 1], 50u)  << "G channel";
    EXPECT_LT(out.pixels[off + 2], 50u)  << "B channel";
    EXPECT_GT(out.pixels[off + 3], 200u) << "A channel";
}

// ---------------------------------------------------------------------------
// two_pages.pdf: page_index 0 and 1 both render; out-of-range throws.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, TwoPages_PageIndexBoundary) {
    const auto pdf = FixtureRoot() / "two_pages.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;
    const auto bytes = ReadFile(pdf);

    const auto p0 = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        0, 72.0);
    const auto p1 = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        1, 72.0);
    EXPECT_GT(p0.pixels.size(), 0u);
    EXPECT_GT(p1.pixels.size(), 0u);

    EXPECT_THROW(
        foundation::page_renderer::Render(
            std::span<const std::byte>(bytes.data(), bytes.size()),
            2, 72.0),
        std::out_of_range);
}

// ---------------------------------------------------------------------------
// Argument validation: target_dpi must be positive.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, RejectsZeroDpi) {
    const auto pdf = FixtureRoot() / "blank.pdf";
    const auto bytes = ReadFile(pdf);
    EXPECT_THROW(
        foundation::page_renderer::Render(
            std::span<const std::byte>(bytes.data(), bytes.size()),
            0, 0.0),
        std::invalid_argument);
}

// ---------------------------------------------------------------------------
// form_xobject.pdf: a /Subtype /Form XObject filled green over its
// BBox [0 0 40 40], invoked once via `q 1 0 0 1 30 30 cm /Fm0 Do Q`.
// The green square lands at PDF (30,30)-(70,70) → image rows/cols
// [30,70] after the y-flip. A renderer that skips Form XObjects
// leaves the page blank; this asserts the form content is executed
// and positioned by formMatrix∘CTM.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, FormXObject_DoExecutesAndPositions) {
    const auto pdf = FixtureRoot() / "form_xobject.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Centre of the form square — green (0, 0.7, 0.2) ≈ (0,178,51).
    const auto c = px(50, 50);
    EXPECT_LT(c[0], 60u)  << "centre R should be low (green)";
    EXPECT_GT(c[1], 120u) << "centre G should be high (green)";
    EXPECT_LT(c[2], 100u) << "centre B should be low (green)";

    // Corners outside the 30..70 square stay white.
    for (auto [x, y] : {std::pair{10u, 10u}, std::pair{90u, 90u}}) {
        const auto w = px(x, y);
        EXPECT_GT(w[0], 230u) << "corner R should be white";
        EXPECT_GT(w[1], 230u) << "corner G should be white";
        EXPECT_GT(w[2], 230u) << "corner B should be white";
    }
}

// ---------------------------------------------------------------------------
// annotation_ap.pdf: a page with NO content stream and a /Square
// annotation whose /AP /N appearance fills its BBox [0 0 20 20] blue;
// the annotation /Rect is [25 25 75 75], so §12.5.5 scales the BBox to
// cover the Rect → a blue square at image rows/cols [25,70]. Asserts
// the annotation appearance stream is rendered and BBox→Rect mapped.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, AnnotationAP_RendersAppearanceIntoRect) {
    const auto pdf = FixtureRoot() / "annotation_ap.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Centre of the annotation Rect — blue (0, 0, 255).
    const auto c = px(50, 50);
    EXPECT_LT(c[0], 60u)  << "centre R should be low (blue)";
    EXPECT_LT(c[1], 60u)  << "centre G should be low (blue)";
    EXPECT_GT(c[2], 200u) << "centre B should be high (blue)";

    // Outside the Rect stays white.
    const auto w = px(10, 10);
    EXPECT_GT(w[0], 230u);
    EXPECT_GT(w[1], 230u);
    EXPECT_GT(w[2], 230u);
}

// ---------------------------------------------------------------------------
// shading_pattern.pdf: a rect filled with a PatternType 2 shading
// (ShadingType 3 radial), red at centre (50,50) fading to blue at
// radius 30. Asserts the radial gradient is evaluated per pixel:
// centre is red, and a point further out is markedly bluer.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ShadingPattern_RadialGradient) {
    const auto pdf = FixtureRoot() / "shading_pattern.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Centre of the gradient — red (C0).
    const auto c = px(50, 50);
    EXPECT_GT(c[0], 150u) << "centre R should be high (red)";
    EXPECT_LT(c[2], 100u) << "centre B should be low (red)";

    // ~25px out toward the edge: markedly bluer than the centre.
    const auto o = px(50, 25);
    EXPECT_GT(static_cast<int>(o[2]), static_cast<int>(c[2]) + 40)
        << "outer pixel should shift toward blue (C1)";
}

// ---------------------------------------------------------------------------
// dash_pattern.pdf: an 8pt black horizontal line at y=50 from x=10
// to x=90 dashed `[16 16] 0 d`. The dash starts "on" at x=10, so the
// painted spans are x∈[10,26], [42,58], [74,90] and the gaps are
// [26,42], [58,74] (device col = x, device row = 100−y). Asserts the
// `d` operator is honoured: painted-dash pixels are black and gap
// pixels stay white — a renderer ignoring `d` would paint the whole
// line solid (the gaps would be black).
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, DashPattern_OnOffSpans) {
    const auto pdf = FixtureRoot() / "dash_pattern.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Painted dash spans — black on the line row.
    for (auto x : {15u, 50u, 82u}) {
        const auto p = px(x, 50);
        EXPECT_LT(p[0], 60u) << "dash span x=" << x << " should be black";
        EXPECT_LT(p[1], 60u);
        EXPECT_LT(p[2], 60u);
    }

    // Gap spans — white on the same row (the `d` operator left them
    // unpainted).
    for (auto x : {34u, 66u}) {
        const auto g = px(x, 50);
        EXPECT_GT(g[0], 230u) << "dash gap x=" << x << " should be white";
        EXPECT_GT(g[1], 230u);
        EXPECT_GT(g[2], 230u);
    }

    // Well off the line stays white.
    const auto w = px(50, 20);
    EXPECT_GT(w[0], 230u);
    EXPECT_GT(w[1], 230u);
    EXPECT_GT(w[2], 230u);
}

// ---------------------------------------------------------------------------
// constant_alpha.pdf: an opaque red square (20,20)-(80,80), then a
// blue square (40,40)-(80,80) painted after `/GS0 gs` (ExtGState
// /ca 0.5). The overlap composites to 0.5·blue + 0.5·red = (128,0,128)
// purple; a renderer that ignores `gs` paints it fully opaque blue.
// Asserts the constant-alpha blend: overlap purple, red-only red.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ConstantAlpha_GsBlendsFill) {
    const auto pdf = FixtureRoot() / "constant_alpha.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Overlap centre — 50% blue over red ≈ (128, 0, 128) purple. The
    // discriminator: red AND blue are both substantial (not pure blue
    // as an opaque paint would give).
    const auto c = px(60, 40);
    EXPECT_GT(c[0], 90u)  << "overlap R (red showing through)";
    EXPECT_LT(c[0], 165u);
    EXPECT_LT(c[1], 60u)  << "overlap G low";
    EXPECT_GT(c[2], 90u)  << "overlap B (the 50% blue)";
    EXPECT_LT(c[2], 165u);

    // Red-only region stays saturated red.
    const auto r = px(30, 70);
    EXPECT_GT(r[0], 200u) << "red-only R";
    EXPECT_LT(r[1], 50u);
    EXPECT_LT(r[2], 50u);
}

// ---------------------------------------------------------------------------
// clip_rect.pdf: a whole-page red fill restricted by `20 40 60 20 re
// W n` to the rect (20,40)-(80,60). Only image rows [40,60] x cols
// [20,80] paint red; everything outside stays white. A renderer that
// ignores `W` paints the whole page red. Asserts the rectangular clip.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ClipRect_RestrictsFill) {
    const auto pdf = FixtureRoot() / "clip_rect.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Inside the clip rect — red.
    const auto in = px(50, 50);
    EXPECT_GT(in[0], 200u) << "inside-clip R";
    EXPECT_LT(in[1], 50u);
    EXPECT_LT(in[2], 50u);

    // Outside the clip on every side (the fill covers these, the clip
    // must mask them) — white.
    for (auto [x, y] : {std::pair{50u, 20u}, std::pair{50u, 80u},
                        std::pair{10u, 50u}, std::pair{90u, 50u}}) {
        const auto o = px(x, y);
        EXPECT_GT(o[0], 230u) << "outside-clip R at " << x << "," << y;
        EXPECT_GT(o[1], 230u) << "outside-clip G at " << x << "," << y;
        EXPECT_GT(o[2], 230u) << "outside-clip B at " << x << "," << y;
    }
}

// ---------------------------------------------------------------------------
// image_smask.pdf: a 2x1 RGB image (left red, right green) with a 2x1
// DeviceGray /SMask (left 255 opaque, right 0 transparent), drawn over
// a blue page. The left half shows red; the right half is transparent
// so blue shows through and the green pixel must NEVER appear. A
// renderer that ignores /SMask paints the right half opaque green.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageSMask_AppliesSoftMaskAlpha) {
    const auto pdf = FixtureRoot() / "image_smask.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Left half — the opaque red image pixel.
    const auto l = px(30, 50);
    EXPECT_GT(l[0], 200u) << "left R (red)";
    EXPECT_LT(l[1], 60u);
    EXPECT_LT(l[2], 60u);

    // Right half — transparent, so the blue page shows through. The
    // green source pixel is fully masked and must not appear.
    const auto r = px(70, 50);
    EXPECT_LT(r[1], 80u) << "right must NOT be green (SMask ignored)";
    EXPECT_GT(r[2], 180u) << "right B (blue background through mask)";
}

// ---------------------------------------------------------------------------
// image_mask_stencil.pdf: a 2x1 RGB image (left red, right green) carrying
// a 2x1 1-bpc /ImageMask stencil via /Mask (left sample 0 = paint, right
// sample 1 = mask), drawn over a blue page. Explicit masking (§8.9.6.3):
// the left half shows red; the right half is masked so blue shows through
// and the green pixel must NEVER appear. A renderer that ignores /Mask
// paints the right half opaque green — the mixed-raster "dark block buries
// the page" defect (field reproducer 30179.pdf).
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageMaskStencil_AppliesExplicitMask) {
    const auto pdf = FixtureRoot() / "image_mask_stencil.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Left half — the unmasked (sample 0) red image pixel paints.
    const auto l = px(30, 50);
    EXPECT_GT(l[0], 200u) << "left R (red)";
    EXPECT_LT(l[1], 60u);
    EXPECT_LT(l[2], 60u);

    // Right half — masked (sample 1), so the blue page shows through. The
    // green source pixel is masked and must not appear.
    const auto r = px(70, 50);
    EXPECT_LT(r[1], 80u) << "right must NOT be green (/Mask ignored)";
    EXPECT_GT(r[2], 180u) << "right B (blue background through mask)";
}

// ---------------------------------------------------------------------------
// image_mask_jbig2.pdf: a 16x16 solid-red image with a 16x16 /ImageMask
// whose data is a JBIG2Decode stream decoding to all-black (every bit = 1).
// JBIG2's 1 = black (§7.4.7) marks the PAINTED region — the inverse of the
// raw-stencil convention (sample 0 = paint) — so the red image paints over
// the blue page. A renderer that applies the raw default polarity to a JBIG2
// mask masks the whole image and leaves the page blue (the 30179.pdf MRC
// defect). Discriminator: image centre is red, not blue.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageMaskJbig2_UsesInversePolarity) {
    const auto pdf = FixtureRoot() / "image_mask_jbig2.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Centre — the all-black JBIG2 mask paints the red image (JBIG2 1=paint).
    const auto c = px(50, 50);
    EXPECT_GT(c[0], 200u) << "centre R (red painted via JBIG2 black mask)";
    EXPECT_LT(c[2], 80u)  << "centre must NOT be blue (raw polarity misapplied)";
}

// ---------------------------------------------------------------------------
// image_calrgb.pdf: a 2x1 image (left black, right white) in a [/CalRGB …]
// colour space (§8.6.5.3) drawn over a white page. Colour management is out
// of scope, so the renderer maps CalRGB to its DeviceRGB device fallback; a
// renderer that treats CalRGB as Unsupported drops the image and the page
// stays white (field reproducers 33306_whitepoint*). Discriminator: the left
// half is black (image rendered), not white (image dropped).
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageCalRgb_MapsToDeviceRgb) {
    const auto pdf = FixtureRoot() / "image_calrgb.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Left half — the black image pixel (image rendered, not dropped).
    const auto l = px(30, 50);
    EXPECT_LT(l[0], 40u) << "left R (black — CalRGB image must render)";
    EXPECT_LT(l[1], 40u);
    EXPECT_LT(l[2], 40u);
}

// ---------------------------------------------------------------------------
// image_smask_predictor.pdf: a 4x4 solid-green image whose DeviceGray
// /SMask is stored with a TIFF Predictor 2 (/DecodeParms) and is fully
// opaque post-prediction. Reversing the predictor yields alpha 0xFF
// everywhere, so the whole page is green. A renderer that ignores the
// predictor reads the stored horizontal differences ([0xFF,0,0,0] per
// row) as alpha, so only the leftmost image column stays opaque and the
// page interior (e.g. x=75) renders white. The right-interior pixel is
// the discriminator. (Regression guard for §7.4.4.4 predictors on image
// / SMask FlateDecode streams; field reproducer 30758.pdf.)
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageSMaskPredictor_ReversesTiffPredictor) {
    const auto pdf = FixtureRoot() / "image_smask_predictor.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Right-interior pixel: green only when the predictor is reversed
    // (the opaque mask makes the green image show); white otherwise.
    const auto r = px(75, 50);
    EXPECT_LT(r[0], 80u)  << "right R (green)";
    EXPECT_GT(r[1], 150u) << "right G — opaque green via reversed predictor";
    EXPECT_LT(r[2], 80u)  << "right B (green)";

    // Centre is green for the same reason.
    const auto c = px(50, 50);
    EXPECT_LT(c[0], 80u);
    EXPECT_GT(c[1], 150u) << "centre G (green)";
    EXPECT_LT(c[2], 80u);
}

// ---------------------------------------------------------------------------
// image_indexed.pdf: a 2x1 1-bit /Indexed image over a [/CalRGB …] base
// (palette index 0 = black, index 1 = white), FlateDecode'd with a PNG
// predictor. Left half = index 0 = black, right half = index 1 = white.
// Three independent gaps each break this (field reproducer 34026.pdf, a
// full-page 1-bit Indexed/CalRGB scan that rendered fully blank):
//   * the Indexed base must resolve recursively (a "/DeviceRGB only"
//     check rejects the CalRGB base → image dropped → white page);
//   * sub-byte (1-bit) indices must be unpacked through the palette, not
//     routed to the 1=black bilevel stencil path (which inverts);
//   * the byte-wise PNG predictor stride must follow the 1-bit sample
//     depth (an 8-bit stride leaves the per-row filter byte in and
//     shears the image).
// Discriminators: left (30,50) black AND right (70,50) white.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageIndexedCalRgbBase_OneBitWithPredictor) {
    const auto pdf = FixtureRoot() / "image_indexed.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };

    // Left half — index 0 = black (image rendered, palette honoured,
    // not inverted, not sheared).
    const auto l = px(30, 50);
    EXPECT_LT(l[0], 40u) << "left R (index 0 = black)";
    EXPECT_LT(l[1], 40u);
    EXPECT_LT(l[2], 40u);
    // Right half — index 1 = white (palette polarity correct).
    const auto r = px(70, 50);
    EXPECT_GT(r[0], 215u) << "right R (index 1 = white)";
    EXPECT_GT(r[1], 215u);
    EXPECT_GT(r[2], 215u);
}

// ---------------------------------------------------------------------------
// content_ascii85_flate.pdf: a black 40x40 centre rectangle whose content
// stream uses a [/ASCII85Decode /FlateDecode] filter chain. A renderer that
// only implements FlateDecode decodes the content to nothing and the page
// stays white (field reproducer 33300-2.pdf, whose content streams are all
// ASCII85 + Flate). Discriminator: centre (50,50) is black.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ContentMultiFilter_Ascii85ThenFlate) {
    const auto pdf = FixtureRoot() / "content_ascii85_flate.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };
    // Centre — black only when the ASCII85→Flate chain decodes the
    // content stream (otherwise the page is blank white).
    const auto c = px(50, 50);
    EXPECT_LT(c[0], 40u) << "centre R (rect drawn → content chain decoded)";
    EXPECT_LT(c[1], 40u);
    EXPECT_LT(c[2], 40u);
    // A corner stays white.
    const auto corner = px(5, 5);
    EXPECT_GT(corner[0], 215u) << "corner R (white background)";
}

// ---------------------------------------------------------------------------
// image_ccitt_{nodecode,decode}.pdf: the SAME 32x16 CCITTFaxDecode (G4)
// bilevel image drawn full-page, one without /Decode and one with
// /Decode [1 0]. /Decode [1 0] inverts a 1-bpc image's polarity (§8.9.5.2),
// so the two renders must be exact pixelwise inverses. A renderer that
// ignores /Decode on a bilevel base image produces identical output and, on
// a full-page scan with /Decode [1 0], floods the page black (field
// reproducer 35063.pdf). The relative (inverse) check avoids depending on
// the absolute CCITT / BlackIs1 polarity.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ImageCcitt_DecodeArrayInvertsPolarity) {
    const auto load = [](const char* name) {
        const auto pdf = FixtureRoot() / name;
        EXPECT_TRUE(std::filesystem::exists(pdf)) << pdf;
        const auto bytes = ReadFile(pdf);
        return foundation::page_renderer::Render(
            std::span<const std::byte>(bytes.data(), bytes.size()),
            /*page_index=*/0, /*target_dpi=*/72.0);
    };
    const auto nd = load("image_ccitt_nodecode.pdf");
    const auto dc = load("image_ccitt_decode.pdf");
    ASSERT_EQ(nd.width, dc.width);
    ASSERT_EQ(nd.height, dc.height);
    ASSERT_GT(nd.width, 0u);

    // Sample the left and right halves (the image is left-black/right-white,
    // or its inverse). Avoid the centre seam and page edges.
    auto r = [](const foundation::page_renderer::RenderedPage& p,
                std::size_t x, std::size_t y) {
        return p.pixels[(y * p.width + x) * 4u];
    };
    const std::size_t y = nd.height / 2;
    const std::size_t lx = nd.width / 4;          // left half
    const std::size_t rx = nd.width * 3 / 4;      // right half

    // The image has structure (left half differs from right half).
    EXPECT_NE(static_cast<int>(r(nd, lx, y)), static_cast<int>(r(nd, rx, y)))
        << "no-decode render should not be flat (image has black+white halves)";

    // /Decode [1 0] must invert: each pixel of the decode render is the
    // complement of the no-decode render (≈ 255 - value).
    for (auto x : {lx, rx}) {
        const int a = r(nd, x, y);
        const int b = r(dc, x, y);
        EXPECT_LE(std::abs((255 - a) - b), 8)
            << "x=" << x << ": /Decode [1 0] must invert (a=" << a
            << " b=" << b << ")";
    }
}

// ---------------------------------------------------------------------------
// separation_fill.pdf: a 50x50 rect filled via a /Separation colour space
// (alternate /DeviceCMYK, Type-2 tint transform with C1 = [0 1 1 0] = red)
// at tint 1. The renderer must evaluate the tint transform into the
// alternate space → RGB; one that ignores /Separation treats `1 scn` as
// DeviceGray 1.0 and paints the rect white (field reproducer 33408.pdf,
// whose PANTONE banner renders grey). Discriminator: centre (50,50) is red.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, SeparationFill_AppliesTintTransform) {
    const auto pdf = FixtureRoot() / "separation_fill.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };
    // Centre is red (tint 1 → CMYK [0 1 1 0] → RGB ~255,0,0) only when the
    // Separation tint transform is applied; white/grey otherwise.
    const auto c = px(50, 50);
    EXPECT_GT(c[0], 200u) << "centre R (red spot colour)";
    EXPECT_LT(c[1], 60u)  << "centre G (red, not white/grey)";
    EXPECT_LT(c[2], 60u)  << "centre B";
}

// ---------------------------------------------------------------------------
// content_lzw_early0.pdf: a black centre rect whose content stream is
// LZWDecode-compressed with TIFF EarlyChange 0 but with NO /DecodeParms (so
// the PDF default EarlyChange 1 applies). A decoder that only tries
// EarlyChange 1 fails and the page stays blank (field reproducer 34203.pdf,
// whose LZW content streams are EarlyChange 0); a lenient decoder retries the
// other convention. Discriminator: centre (50,50) is black.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, ContentLzw_EarlyChangeFallback) {
    const auto pdf = FixtureRoot() / "content_lzw_early0.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };
    // Centre is black only when the EarlyChange-0 LZW content decodes via
    // the fallback (otherwise the page is blank white).
    const auto c = px(50, 50);
    EXPECT_LT(c[0], 40u) << "centre R (rect drawn → LZW decoded)";
    EXPECT_LT(c[1], 40u);
    EXPECT_LT(c[2], 40u);
    const auto corner = px(5, 5);
    EXPECT_GT(corner[0], 215u) << "corner R (white background)";
}

// ---------------------------------------------------------------------------
// pages_no_type.pdf: the /Pages tree root carries /Kids but NO /Type. /Type is
// required on /Pages nodes (§7.7.3.2) but real PDFs frequently omit it; the
// page-tree walk must infer "Pages" from /Kids rather than reject the document
// (field reproducers 32988-1 / 33204-1 / 33211, open-failed with "Page-tree
// root missing /Type"). Discriminator: a red rect renders (page opened).
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, PageTreeRoot_NoType_InferredFromKids) {
    const auto pdf = FixtureRoot() / "pages_no_type.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    auto px = [&](std::size_t x, std::size_t y) {
        const std::size_t o = (y * out.width + x) * 4u;
        return std::array<std::uint8_t, 4>{
            out.pixels[o], out.pixels[o + 1],
            out.pixels[o + 2], out.pixels[o + 3]};
    };
    // Centre red only when the /Type-less Pages root was accepted (the page
    // opened and its content drew); blank/throw otherwise.
    const auto c = px(50, 50);
    EXPECT_GT(c[0], 200u) << "centre R (page opened, rect drawn)";
    EXPECT_LT(c[1], 60u);
    EXPECT_LT(c[2], 60u);
}

// ---------------------------------------------------------------------------
// inline_image.pdf: a §8.9.7 inline image (BI / ID <samples> EI) — a 1x1
// DeviceRGB red pixel scaled by the CTM to fill the page. A renderer that
// drops inline-image samples (the prior behaviour) leaves the page white;
// honouring them paints it red (field reproducer 33433.pdf, a single
// page-filling inline image). Exercises abbreviation expansion (/W /H /CS
// /BPC and the /RGB colour-space abbreviation). Discriminator: centre red.
// ---------------------------------------------------------------------------

TEST(PageRendererSmoke, InlineImage_RendersSamples) {
    const auto pdf = FixtureRoot() / "inline_image.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    const auto bytes = ReadFile(pdf);
    const auto out = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);
    ASSERT_EQ(out.width, 100u);
    ASSERT_EQ(out.height, 100u);

    const std::size_t off = (50u * out.width + 50u) * 4u;
    ASSERT_LT(off + 3u, out.pixels.size());
    EXPECT_GT(out.pixels[off], 200u)     << "centre R (inline image drawn)";
    EXPECT_LT(out.pixels[off + 1], 60u)  << "centre G";
    EXPECT_LT(out.pixels[off + 2], 60u)  << "centre B";
    EXPECT_GT(out.pixels[off + 3], 200u) << "centre A";
}
