// =============================================================================
// text_field_layout_smoke_test — text field appearances follow the field's
// /DA (font, size, colour), multi-line wrapping, quadding (/Q) and comb
// layout, for new fields and fields loaded from a file; FreeText wraps.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/password_box_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;

namespace {

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

// Save hello_world.pdf with one text field configured by `setup`; returns
// the saved file path (caller removes it).
template <class Setup>
std::string SaveField(const std::string& tag, Rectangle rect, Setup setup) {
    const std::string out = TmpPath("layout_" + tag);
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField f{doc, rect};
    setup(f);
    doc.Form().Add(f, "f", 1);
    doc.Save(out);
    return out;
}

}  // namespace

TEST(TextFieldLayout, NewFieldUsesDefaultAppearanceFont) {
    const std::string out =
        SaveField("da_font", Rect(100, 400, 300, 430), [](auto& f) {
            Annotations::DefaultAppearance da;
            da.FontName("Times-Roman");
            da.FontSize(18);
            f.DefaultAppearance(da);
            f.Value("Serif");
        });
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "/BaseFont /Times-Roman"));
    EXPECT_TRUE(Has(bytes, "/TiRo 18 Tf"));
    EXPECT_TRUE(Has(bytes, "/DA (/TiRo 18 Tf 0 g)"));
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, NewMultilineFieldWraps) {
    const std::string out =
        SaveField("multiline", Rect(100, 300, 250, 400), [](auto& f) {
            f.Multiline(true);
            // About six lines at 12pt in a 146pt-wide box.
            f.Value("The quick brown fox jumps over the lazy dog. The quick "
                    "brown fox jumps over the lazy dog. The quick brown fox "
                    "jumps over the lazy dog.");
        });
    Document re{out};
    const auto img = Render(re, 1);
    EXPECT_GT(DarkPixels(img, 100, 370, 250, 400), 20);  // first lines
    EXPECT_GT(DarkPixels(img, 100, 300, 250, 350), 10);  // later lines
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, CenterAlignment) {
    const std::string out =
        SaveField("center", Rect(100, 400, 400, 420), [](auto& f) {
            f.TextHorizontalAlignment(HorizontalAlignment::Center);
            f.Value("Hi");
        });
    EXPECT_TRUE(Has(ReadFile(out), "/Q 1"));
    Document re{out};
    const auto img = Render(re, 1);
    EXPECT_EQ(DarkPixels(img, 100, 400, 175, 420), 0);
    EXPECT_GT(DarkPixels(img, 225, 400, 275, 420), 5);
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, CombField) {
    const std::string out =
        SaveField("comb", Rect(100, 400, 300, 420), [](auto& f) {
            f.ForceCombs(true);
            f.MaxLen(2);
            f.Value("ab");
        });
    EXPECT_TRUE(Has(ReadFile(out), "/MaxLen 2"));
    Document re{out};
    // "b" sits centred in the right-hand cell.
    EXPECT_GT(DarkPixels(Render(re, 1), 200, 400, 300, 420), 5);
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, PasswordFieldShowsAsterisks) {
    const std::string out = TmpPath("layout_password");
    {
        Document doc{HelloWorldPdf()};
        Forms::PasswordBoxField p;
        p.Rect(Rect(100, 400, 300, 420));
        p.Value("secret");
        doc.Form().Add(p, "pwd", 1);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "(******) Tj"));
    EXPECT_FALSE(Has(bytes, "(secret) Tj"));
    std::filesystem::remove(out);
}
