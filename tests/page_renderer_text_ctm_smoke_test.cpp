// =============================================================================
// page_renderer_text_ctm_smoke_test — text must be positioned by the CURRENT
// graphics-state CTM (so a `cm` transform applies to glyphs), and a NEGATIVE
// font size must not blow the glyph up to raw font-unit scale (task_06 B3,
// reproducer 28353.pdf). Telerik/Aspose-generated PDFs set a page-level
// `cm 1 0 0 -1 0 H` y-flip and draw text with a negative font size + negative
// `Tz`; the renderer had (1) positioned text with the page default CTM,
// ignoring `cm`, so glyphs landed off their cells (clipped away → blank), and
// (2) bailed BuildGlyphCtm to identity on a non-positive font size, drawing a
// page-filling black blob.
// =============================================================================

#include "page_renderer.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> PdfWithHelvetica(const std::string& content) {
    std::string s = "%PDF-1.5\n";
    auto add = [&](int n, const std::string& body) {
        s += std::to_string(n) + " 0 obj\n" + body + "\nendobj\n";
    };
    add(1, "<< /Type /Catalog /Pages 2 0 R >>");
    add(2, "<< /Type /Pages /Count 1 /Kids [3 0 R] >>");
    add(3, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] "
           "/Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>");
    add(4, "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" +
           content + "endstream");
    add(5, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");
    s += "trailer\n<</Root 1 0 R>>\nstartxref\n0\n%%EOF\n";
    std::vector<std::byte> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        out[i] = static_cast<std::byte>(s[i]);
    return out;
}

// Count dark (near-black) pixels inside a device-pixel rectangle.
std::size_t DarkPixels(const foundation::page_renderer::RenderedPage& pg,
                       std::uint32_t x0, std::uint32_t y0,
                       std::uint32_t x1, std::uint32_t y1) {
    std::size_t n = 0;
    for (std::uint32_t y = y0; y < y1 && y < pg.height; ++y)
        for (std::uint32_t x = x0; x < x1 && x < pg.width; ++x) {
            const std::size_t o = (y * pg.width + x) * 4u;
            if (pg.pixels[o] < 100 && pg.pixels[o + 1] < 100 &&
                pg.pixels[o + 2] < 100)
                ++n;
        }
    return n;
}

// A `cm` translate must move the text: 'H' drawn at text origin (0,0) under
// `cm 1 0 0 1 130 130` lands near device (133,49)-(148,69) (PDF y-up flipped
// over the 200pt page at 72dpi), NOT at the untranslated bottom-left origin.
TEST(PageRendererTextCtm, CmTranslatePositionsGlyph) {
    const auto pdf = PdfWithHelvetica(
        "q\n1 0 0 1 130 130 cm\nBT\n/F1 30 Tf\n0 0 0 rg\n0 0 Td\n(H) Tj\nET\nQ\n");
    const auto pg = foundation::page_renderer::Render(
        std::span<const std::byte>(pdf.data(), pdf.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);

    // Ink lands in the cm-translated cell (upper-middle-right).
    EXPECT_GT(DarkPixels(pg, 120, 40, 160, 80), 30u)
        << "glyph not at the cm-translated position";
    // And NOT at the untranslated origin (device bottom-left).
    EXPECT_EQ(DarkPixels(pg, 0, 170, 40, 200), 0u)
        << "glyph drew at the page-default origin — cm ignored for text";
}

// A negative font size (with a y-flipping CTM) renders normal-sized upright
// text, not a page-filling black blob. Guards the BuildGlyphCtm identity bail.
TEST(PageRendererTextCtm, NegativeFontSizeIsNotGiantBlob) {
    // y-flip the page, then draw with -30 Tf and -100 Tz so the doubly-flipped
    // glyph reads upright — the 28353 idiom.
    const auto pdf = PdfWithHelvetica(
        "q\n1 0 0 -1 0 200 cm\n-100 Tz\nBT\n/F1 -30 Tf\n0 0 0 rg\n"
        "20 60 Td\n(HELLO) Tj\nET\nQ\n");
    const auto pg = foundation::page_renderer::Render(
        std::span<const std::byte>(pdf.data(), pdf.size()),
        /*page_index=*/0, /*target_dpi=*/72.0);

    // A single 30pt glyph covers well under 5% of a 200x200 page; the identity
    // bail filled a ~font-unit-sized (2048px) black region clamped to the page.
    const std::size_t total =
        static_cast<std::size_t>(pg.width) * pg.height;
    const std::size_t dark = DarkPixels(pg, 0, 0, pg.width, pg.height);
    EXPECT_LT(dark, total / 10)
        << "negative font size produced a giant black blob (" << dark
        << " / " << total << " px)";
    // But the glyph did render something.
    EXPECT_GT(dark, 20u) << "negative-font-size glyph drew nothing";
}

}  // namespace
