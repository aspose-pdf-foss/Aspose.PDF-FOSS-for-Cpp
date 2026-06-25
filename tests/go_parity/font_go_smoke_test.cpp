// =============================================================================
// Go-parity: font_api_test.go → canonical C++ Text::FontRepository::FindFont
// + Text::Font.
//
// Ported (canonical): FindFont exact name → FontName / BaseFont, the
// Standard-14 name sweep, and Font.IsEmbedded default.
// Skipped: case-insensitive lookup and unknown-font error (this lib's FindFont
// is a by-name resolver that does not normalise or reject names), and
// LoadFont/OpenFont from file/stream (FontRepository.OpenFont is not in the v1
// cpp surface).
// =============================================================================

#include <string>

#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>

#include <gtest/gtest.h>

namespace {

using Aspose::Pdf::Text::Font;
using Aspose::Pdf::Text::FontRepository;

}  // namespace

// TestFindFontExact — resolve a Standard-14 font by exact name.
TEST(GoFont, FindFontExact) {
    Font f = FontRepository::FindFont("Helvetica");
    EXPECT_EQ(f.BaseFont(), "Helvetica");
    EXPECT_EQ(f.FontName(), "Helvetica");
}

// TestFindFontAllStandard14 — each Standard-14 name resolves.
TEST(GoFont, FindFontStandard14Sweep) {
    const char* names[] = {
        "Helvetica",  "Helvetica-Bold", "Times-Roman", "Times-Bold",
        "Courier",    "Courier-Bold",   "Symbol",      "ZapfDingbats"};
    for (const char* n : names) {
        Font f = FontRepository::FindFont(n);
        EXPECT_EQ(f.FontName(), n);
    }
}

// Font.IsEmbedded defaults to false for a freshly-resolved Standard-14 font.
TEST(GoFont, IsEmbeddedDefault) {
    Font f = FontRepository::FindFont("Helvetica");
    EXPECT_FALSE(f.IsEmbedded());
}
