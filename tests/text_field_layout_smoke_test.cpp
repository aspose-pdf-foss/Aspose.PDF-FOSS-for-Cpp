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

// ---- Fields loaded from a file ----------------------------------------------

namespace {

// A one-page form: field "name" (no /AP) at (100,400)-(300,430) with the
// given /DA, /Ff and /V; /AcroForm /DR /Font holds Helv (obj 6, Helvetica)
// and F1 (obj 7, Courier with a /Widths table).
void WriteForm(const std::string& path, const std::string& da, int ff,
               const std::string& value) {
    WritePdf(path,
             {"<< /Type /Catalog /Pages 2 0 R /AcroForm << /Fields [4 0 R] "
              "/DR << /Font << /Helv 6 0 R /F1 7 0 R >> >> "
              "/DA (/Helv 0 Tf 0 g) >> >>",
              "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
              "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
              "/Annots [4 0 R] >>",
              "<< /Type /Annot /Subtype /Widget /FT /Tx /T (name) "
              "/Rect [100 400 300 430] /P 3 0 R /F 4 /Ff " +
                  std::to_string(ff) + " /DA (" + da + ") /V (" + value +
                  ") >>",
              "<< >>",
              "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
              "<< /Type /Font /Subtype /Type1 /BaseFont /Courier "
              "/FirstChar 32 /LastChar 126 /Widths [" +
                  [] {
                      std::string w;
                      for (int i = 32; i <= 126; ++i) w += "600 ";
                      return w;
                  }() +
                  "] >>"});
}

// The font resource `key` of the normal appearance of field "name".
// The object id the appearance's font resource `key` refers to; 0 when it
// is absent or not a reference. Returned by value: the dictionaries walked
// here are local copies.
std::uint32_t ApFontRefId(const Parsed& p, const std::string& key) {
    for (const auto& d : AnnotDicts(p)) {
        if (NameOf(Get(d, "Subtype")) != "Widget") continue;
        const fo::Dict ap = ResolveDict(p.dump, Get(d, "AP"));
        const fo::Dict n = ResolveDict(p.dump, Get(ap, "N"));
        const fo::Dict res = ResolveDict(p.dump, Get(n, "Resources"));
        const fo::Dict fonts = ResolveDict(p.dump, Get(res, "Font"));
        const fo::Value* v = Get(fonts, key);
        const auto* r = v ? std::get_if<fo::Ref>(&v->v) : nullptr;
        return r ? r->id : 0;
    }
    return 0;
}

int ColorPixels(const foundation::png_decoder::DecodedImage& img, bool red) {
    return CountPixels(img, 100, 400, 300, 430, 2.0, [&](int r, int g, int b) {
        return red ? (r > 150 && g < 100 && b < 100)
                   : (b > 150 && r < 100 && g < 100);
    });
}

}  // namespace

TEST(TextFieldLayout, LoadedFieldUsesItsDaColorAndDrFont) {
    const std::string src = TmpPath("loaded_da_src");
    const std::string out = TmpPath("loaded_da_out");
    WriteForm(src, "/Helv 0 Tf 1 0 0 rg", 0, "Old");
    {
        Document re{src};
        ASSERT_EQ(re.Form().Count(), 1);
        re.Form().Fields()[0]->Value("Red text");
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(ApFontRefId(p, "Helv"), 6u);  // the /DR font object itself
    Document re{out};
    EXPECT_GT(ColorPixels(Render(re, 1), true), 10);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, LoadedFieldUsesDrFontWithWidths) {
    const std::string src = TmpPath("loaded_widths_src");
    const std::string out = TmpPath("loaded_widths_out");
    WriteForm(src, "/F1 10 Tf 0 0 1 rg", 0, "Old");
    {
        Document re{src};
        re.Form().Fields()[0]->Value("Blue");
        re.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "/F1 10 Tf"));
    EXPECT_EQ(ApFontRefId(ParsePage(bytes, 0), "F1"), 7u);
    Document re{out};
    EXPECT_GT(ColorPixels(Render(re, 1), false), 10);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, LoadedMultilineFieldWraps) {
    const std::string src = TmpPath("loaded_ml_src");
    const std::string out = TmpPath("loaded_ml_out");
    WriteForm(src, "/Helv 8 Tf 0 g", 4096, "x");
    {
        Document re{src};
        re.Form().Fields()[0]->Value(
            "one two three four five six seven eight nine ten eleven twelve "
            "thirteen fourteen fifteen sixteen");
        re.Save(out);
    }
    const std::string bytes = ReadFile(out);
    int tj = 0;
    for (std::size_t p = 0; (p = bytes.find(") Tj", p)) != std::string::npos;
         p += 4)
        ++tj;
    EXPECT_GE(tj, 2);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(TextFieldLayout, FlattenFieldWithoutApUsesDa) {
    const std::string src = TmpPath("flat_noap_src");
    const std::string out = TmpPath("flat_noap_out");
    WriteForm(src, "/Helv 14 Tf 1 0 0 rg", 0, "Red");
    {
        Document re{src};
        re.Form().Flatten();
        re.Save(out);
    }
    EXPECT_TRUE(Has(ReadFile(out), "/Helv 14 Tf"));
    Document re{out};
    EXPECT_GT(ColorPixels(Render(re, 1), true), 10);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
