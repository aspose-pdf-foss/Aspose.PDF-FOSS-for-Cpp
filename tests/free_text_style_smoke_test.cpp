// =============================================================================
// free_text_style_smoke_test — FreeTextAnnotation::SetTextStyle(from, to,
// styles): styled runs render with the bold / italic face of the
// DefaultAppearance font family and are written to /RC; /DA is always
// written and its font size honoured.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/free_text_annotation.hpp>
#include <aspose/pdf/annotations/rich_text_font_styles.hpp>
#include <aspose/pdf/document.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;
using Annotations::RichTextFontStyles;

namespace {

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

// Save hello_world.pdf with one FreeText at (100,400)-(400,440) holding
// `text`, after `style` is applied to it; returns the saved bytes.
template <class Style>
std::string SaveFreeText(const std::string& tag, const std::string& text,
                         double font_size, Style style) {
    const std::string out = TmpPath("freetext_" + tag);
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        Annotations::DefaultAppearance da;
        da.FontName("Helvetica");
        da.FontSize(font_size);
        Annotations::FreeTextAnnotation ft{page, Rect(100, 400, 400, 440), da};
        ft.Contents(text);
        style(ft);
        page.Annotations().Add(ft);
        doc.Save(out);
    }
    std::string bytes = ReadFile(out);
    std::filesystem::remove(out);
    return bytes;
}

int DarkInFreeText(const std::string& bytes, const std::string& tag) {
    const std::string path = TmpPath("freetext_render_" + tag);
    {
        std::ofstream o(path, std::ios::binary);
        o.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    int n = 0;
    {
        Document doc{path};
        n = DarkPixels(Render(doc, 1), 100, 400, 400, 440);
    }
    std::filesystem::remove(path);
    return n;
}

}  // namespace

TEST(FreeTextStyle, BoldRunRendersDarker) {
    const std::string plain = SaveFreeText(
        "plain", "WWWWWWWWWW", 20, [](Annotations::FreeTextAnnotation&) {});
    const std::string bold = SaveFreeText(
        "bold", "WWWWWWWWWW", 20, [](Annotations::FreeTextAnnotation& f) {
            f.SetTextStyle(0, 10, RichTextFontStyles::Bold);
        });
    EXPECT_GT(DarkInFreeText(bold, "b"), DarkInFreeText(plain, "p") + 30);
}

TEST(FreeTextStyle, StyleRunsWrittenToRichText) {
    const std::string s = SaveFreeText(
        "rc", "Hello World", 12, [](Annotations::FreeTextAnnotation& f) {
            f.SetTextStyle(0, 5, RichTextFontStyles::Bold);
        });
    EXPECT_TRUE(Has(s, "/RC"));
    EXPECT_TRUE(Has(s, "<span style=\"font-weight:bold\">Hello</span> World"));
    EXPECT_TRUE(Has(s, "/Helvetica-Bold"));
}

TEST(FreeTextStyle, FontSizeFromDefaultAppearance) {
    const std::string s = SaveFreeText(
        "size", "Big", 20, [](Annotations::FreeTextAnnotation&) {});
    EXPECT_TRUE(Has(s, " 20 Tf"));
}

TEST(FreeTextStyle, DefaultAppearanceWritten) {
    const std::string s = SaveFreeText(
        "da", "Text", 12, [](Annotations::FreeTextAnnotation&) {});
    EXPECT_TRUE(Has(s, "/DA ("));
    EXPECT_FALSE(Has(s, "/RC"));  // no styled runs, no rich text
}

TEST(FreeTextStyle, OutOfRangeStyleIsClamped) {
    std::string s;
    EXPECT_NO_THROW(s = SaveFreeText(
        "clamp", "Slanted", 12, [](Annotations::FreeTextAnnotation& f) {
            f.SetTextStyle(-5, 100, RichTextFontStyles::Italic);
            f.SetTextStyle(4, 2, RichTextFontStyles::Bold);  // empty: ignored
        }));
    EXPECT_TRUE(Has(s, "<span style=\"font-style:italic\">Slanted</span>"));
    EXPECT_TRUE(Has(s, "/Helvetica-Oblique"));
    EXPECT_FALSE(Has(s, "font-weight:bold"));
}

TEST(FreeTextStyle, LaterStyleOverrides) {
    const std::string s = SaveFreeText(
        "override", "Hello World", 12, [](Annotations::FreeTextAnnotation& f) {
            f.SetTextStyle(0, 11, RichTextFontStyles::Bold);
            f.SetTextStyle(2, 4, RichTextFontStyles::Plain);
        });
    EXPECT_TRUE(Has(s, "<span style=\"font-weight:bold\">He</span>ll"
                       "<span style=\"font-weight:bold\">o World</span>"));
}
