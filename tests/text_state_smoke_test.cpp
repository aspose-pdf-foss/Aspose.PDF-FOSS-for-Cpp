// =============================================================================
// text_state_smoke_test — beat T-3 of the text cluster. The rich TextState
// styling properties (colours, decoration / position flags, spacing / scaling,
// horizontal alignment) added alongside the existing Font / FontSize.
// =============================================================================

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/text_state.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Text;

}  // namespace

TEST(TextStateSmoke, Defaults) {
    TextState ts;
    EXPECT_FALSE(ts.Underline());
    EXPECT_FALSE(ts.StrikeOut());
    EXPECT_FALSE(ts.Invisible());
    EXPECT_FLOAT_EQ(ts.CharacterSpacing(), 0.0f);
    EXPECT_FLOAT_EQ(ts.HorizontalScaling(), 100.0f);
    EXPECT_EQ(ts.HorizontalAlignment(), HorizontalAlignment::None);
}

TEST(TextStateSmoke, ColourProperties) {
    TextState ts;
    // Default colours are transparent (alpha 0); a set colour round-trips.
    EXPECT_DOUBLE_EQ(ts.ForegroundColor().A(), 0.0);
    ts.ForegroundColor(Color::FromRgb(1.0, 0.0, 0.0));  // opaque
    EXPECT_DOUBLE_EQ(ts.ForegroundColor().A(), 1.0);
    ts.StrokingColor(Color::FromRgb(0.0, 1.0, 0.0));
    EXPECT_DOUBLE_EQ(ts.StrokingColor().A(), 1.0);
    ts.BackgroundColor(Color::Transparent());
    EXPECT_DOUBLE_EQ(ts.BackgroundColor().A(), 0.0);
}

TEST(TextStateSmoke, FlagsAndSpacing) {
    TextState ts;
    ts.Underline(true);
    ts.StrikeOut(true);
    ts.Subscript(true);
    ts.Superscript(true);
    ts.Invisible(true);
    ts.CharacterSpacing(2.5f);
    ts.WordSpacing(1.5f);
    ts.LineSpacing(14.0f);
    ts.HorizontalScaling(80.0f);
    ts.HorizontalAlignment(HorizontalAlignment::Center);

    EXPECT_TRUE(ts.Underline());
    EXPECT_TRUE(ts.StrikeOut());
    EXPECT_TRUE(ts.Subscript());
    EXPECT_TRUE(ts.Superscript());
    EXPECT_TRUE(ts.Invisible());
    EXPECT_FLOAT_EQ(ts.CharacterSpacing(), 2.5f);
    EXPECT_FLOAT_EQ(ts.WordSpacing(), 1.5f);
    EXPECT_FLOAT_EQ(ts.LineSpacing(), 14.0f);
    EXPECT_FLOAT_EQ(ts.HorizontalScaling(), 80.0f);
    EXPECT_EQ(ts.HorizontalAlignment(), HorizontalAlignment::Center);
}

// MeasureString returns the AFM advance width for a Standard-14 font.
// "Hello" in Helvetica @ 12pt: H722 e556 l222 l222 o556 = 2278/1000 * 12 = 27.336.
TEST(TextStateSmoke, MeasureStringHelvetica) {
    TextState ts;
    ts.Font(FontRepository::FindFont("Helvetica"));
    ts.FontSize(12.0f);
    EXPECT_NEAR(ts.MeasureString("Hello"), 27.336, 0.05);
    // Empty string measures zero.
    EXPECT_NEAR(ts.MeasureString(""), 0.0, 1e-9);
    // Bold "Hello" is wider (H722 e556 l278 l278 o611 = 2445/1000*12 = 29.34).
    ts.Font(FontRepository::FindFont("Helvetica-Bold"));
    EXPECT_GT(ts.MeasureString("Hello"), 28.0);
}

// CharacterSpacing, WordSpacing and HorizontalScaling adjust the measured width.
TEST(TextStateSmoke, MeasureStringSpacingAndScaling) {
    TextState ts;
    ts.Font(FontRepository::FindFont("Helvetica"));
    ts.FontSize(12.0f);
    const double base = ts.MeasureString("a b");  // 2 glyphs + 1 space
    // CharacterSpacing adds per glyph (3 codepoints here: 'a', ' ', 'b').
    ts.CharacterSpacing(1.0f);
    EXPECT_NEAR(ts.MeasureString("a b"), base + 3.0, 1e-6);
    ts.CharacterSpacing(0.0f);
    // WordSpacing adds per single-byte space (1 here).
    ts.WordSpacing(2.0f);
    EXPECT_NEAR(ts.MeasureString("a b"), base + 2.0, 1e-6);
    ts.WordSpacing(0.0f);
    // HorizontalScaling scales the whole advance.
    ts.HorizontalScaling(50.0f);
    EXPECT_NEAR(ts.MeasureString("a b"), base * 0.5, 1e-6);
}
