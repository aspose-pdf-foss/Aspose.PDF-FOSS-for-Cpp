// =============================================================================
// font_smoke_test — beat T-2 of the text cluster. The Font enrichment:
// BaseFont, IsEmbedded and IsSubset (alongside the existing FontName), resolved
// through FontRepository::FindFont.
// =============================================================================

#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf::Text;

}  // namespace

TEST(FontSmoke, NameAndBaseFont) {
    Font f = FontRepository::FindFont("Helvetica");
    EXPECT_EQ(f.FontName(), "Helvetica");
    EXPECT_EQ(f.BaseFont(), "Helvetica");
}

TEST(FontSmoke, EmbeddedAndSubsetFlags) {
    Font f = FontRepository::FindFont("Times-Roman");
    // Name-resolved fonts are not embedded / subsetted by default.
    EXPECT_FALSE(f.IsEmbedded());
    EXPECT_FALSE(f.IsSubset());

    f.IsEmbedded(true);
    f.IsSubset(true);
    EXPECT_TRUE(f.IsEmbedded());
    EXPECT_TRUE(f.IsSubset());
}
