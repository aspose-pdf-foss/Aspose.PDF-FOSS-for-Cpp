// =============================================================================
// field_appearance_smoke_test — the form-field appearance-stream generators shared
// by the AcroForm save path and Form::Flatten (foundation::field_appearance).
// Each returns a content-stream body in widget-local space [0 0 w h].
// =============================================================================

#include <string>
#include <vector>

#include "field_appearance.hpp"

#include <gtest/gtest.h>

namespace fa = foundation::field_appearance;

namespace {

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

}  // namespace

TEST(FieldAppearance, TextBoxDrawsValue) {
    const std::string s = fa::TextBox(100, 20, "Hi (x)");
    EXPECT_TRUE(Has(s, "/Helv 12 Tf"));
    EXPECT_TRUE(Has(s, "(Hi \\(x\\)) Tj"));
    EXPECT_EQ(s.rfind("q\n", 0), 0u);
    ASSERT_GE(s.size(), 2u);
    EXPECT_EQ(s.substr(s.size() - 2), "Q\n");
}

TEST(FieldAppearance, TextBoxEmptyHasNoText) {
    EXPECT_FALSE(Has(fa::TextBox(100, 20, ""), "Tj"));
}

TEST(FieldAppearance, CheckBoxOnHasMarkOffHasNot) {
    EXPECT_TRUE(Has(fa::CheckBox(12, 12, true), "0 0.5 0 RG"));
    EXPECT_FALSE(Has(fa::CheckBox(12, 12, false), "0 0.5 0 RG"));
}

TEST(FieldAppearance, RadioButtonOnHasDot) {
    EXPECT_TRUE(Has(fa::RadioButton(12, 12, true), "\nf\n"));
    EXPECT_FALSE(Has(fa::RadioButton(12, 12, false), "\nf\n"));
}

TEST(FieldAppearance, ListBoxHighlightsSelected) {
    const std::vector<std::string> opts{"a", "b"};
    const std::string sel = fa::ListBox(80, 60, opts, 1);
    EXPECT_TRUE(Has(sel, "0.6 0.75 0.9 rg"));
    EXPECT_TRUE(Has(sel, "(a) Tj"));
    EXPECT_TRUE(Has(sel, "(b) Tj"));
    EXPECT_FALSE(Has(fa::ListBox(80, 60, opts, -1), "0.6 0.75 0.9 rg"));
}

TEST(FieldAppearance, PushButtonDefaultCaption) {
    EXPECT_TRUE(Has(fa::PushButton(60, 20, ""), "(Button) Tj"));
}

TEST(FieldAppearance, EscapeLiteral) {
    EXPECT_EQ(fa::EscapeLiteral("a(b)c\\"), "a\\(b\\)c\\\\");
}

// ---- Text layout: /DA, multi-line, quadding, comb ---------------------------

namespace {

int Count(const std::string& s, const std::string& needle) {
    int n = 0;
    for (std::size_t p = 0; (p = s.find(needle, p)) != std::string::npos;
         p += needle.size())
        ++n;
    return n;
}

fa::TextLayoutStyle Helv(double size) {
    fa::TextLayoutStyle st;
    st.font_key = "Helv";
    st.metrics.base_font = "Helvetica";
    st.size = size;
    return st;
}

}  // namespace

TEST(FieldAppearance, ParseDaReadsFontSizeAndColor) {
    const auto a = fa::ParseDa("/Helv 0 Tf 1 0 0 rg");
    EXPECT_EQ(a.font_key, "Helv");
    EXPECT_DOUBLE_EQ(a.size, 0.0);
    EXPECT_EQ(a.color, "1 0 0 rg");
    const auto b = fa::ParseDa("0.5 g /TiRo 10 Tf");
    EXPECT_EQ(b.font_key, "TiRo");
    EXPECT_DOUBLE_EQ(b.size, 10.0);
    EXPECT_EQ(b.color, "0.5 g");
    const auto c = fa::ParseDa("");
    EXPECT_TRUE(c.font_key.empty());
    EXPECT_EQ(c.color, "0 g");
}

TEST(FieldAppearance, MapFontNameToStandard14) {
    EXPECT_EQ(fa::MapFontName("Helv").base_font, "Helvetica");
    EXPECT_EQ(fa::MapFontName("Arial").base_font, "Helvetica");
    EXPECT_EQ(fa::MapFontName("Arial,Bold").base_font, "Helvetica-Bold");
    EXPECT_EQ(fa::MapFontName("Arial,Bold").key, "HeBo");
    EXPECT_EQ(fa::MapFontName("Times New Roman").base_font, "Times-Roman");
    EXPECT_EQ(fa::MapFontName("TiRo").key, "TiRo");
    EXPECT_EQ(fa::MapFontName("Times-BoldItalic").base_font, "Times-BoldItalic");
    EXPECT_EQ(fa::MapFontName("Cour").base_font, "Courier");
    EXPECT_EQ(fa::MapFontName("Courier-Oblique").key, "CoOb");
    EXPECT_EQ(fa::MapFontName("SomethingElse").base_font, "Helvetica");
}

TEST(FieldAppearance, TextWidthUsesMetrics) {
    fa::FontMetrics courier;
    courier.base_font = "Courier";
    EXPECT_DOUBLE_EQ(fa::TextWidth("abcd", 10, courier), 24.0);  // 600/1000
    fa::FontMetrics custom;
    custom.first_char = 65;
    custom.widths = {700, 800};  // 'A', 'B'
    custom.missing_width = 250;
    EXPECT_DOUBLE_EQ(fa::TextWidth("ABC", 10, custom), 7 + 8 + 2.5);
}

TEST(FieldAppearance, WrapBreaksAtWordsAndNewlines) {
    fa::FontMetrics courier;
    courier.base_font = "Courier";
    // Courier 10pt: 6pt per char; 40pt fits 6 chars.
    const auto lines = fa::WrapText("aaa bbb ccc\nd", 40, 10, courier);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "aaa");
    EXPECT_EQ(lines[1], "bbb");
    EXPECT_EQ(lines[2], "ccc");
    EXPECT_EQ(lines[3], "d");
    const auto split = fa::WrapText("abcdefghij", 40, 10, courier);
    ASSERT_EQ(split.size(), 2u);
    EXPECT_EQ(split[0], "abcdef");
    EXPECT_EQ(split[1], "ghij");
}

TEST(FieldAppearance, TextFieldMultilineEmitsOneTjPerLine) {
    auto st = Helv(10);
    st.multiline = true;
    const std::string s =
        fa::TextField(60, 80, "one two three four five six", st);
    EXPECT_GE(Count(s, ") Tj"), 3);
}

TEST(FieldAppearance, TextFieldAutoSizeShrinksToFit) {
    auto st = Helv(0);
    const std::string s = fa::TextField(
        60, 20, "a long value that cannot fit at full height", st);
    // The chosen size is printed as "/Helv <size> Tf"; it must be small.
    const auto p = s.find("/Helv ");
    ASSERT_NE(p, std::string::npos);
    const double size = std::stod(s.substr(p + 6));
    EXPECT_LT(size, 8.0);
    EXPECT_GE(size, 4.0);
}

TEST(FieldAppearance, TextFieldCombPlacesEachCharacter) {
    auto st = Helv(10);
    st.comb = true;
    st.max_len = 5;
    EXPECT_EQ(Count(fa::TextField(100, 20, "abc", st), ") Tj"), 3);
}

TEST(FieldAppearance, TextFieldQuaddingMovesText) {
    auto left = Helv(10);
    auto right = Helv(10);
    right.quadding = 2;
    const std::string l = fa::TextField(200, 20, "Hi", left);
    const std::string r = fa::TextField(200, 20, "Hi", right);
    auto x_of = [](const std::string& s) {
        const auto td = s.find(" Td");
        auto start = s.rfind('\n', td);
        return std::stod(s.substr(start + 1));
    };
    EXPECT_LT(x_of(l), 10.0);
    EXPECT_GT(x_of(r), 150.0);
}

TEST(FieldAppearance, TextFieldColorFromStyle) {
    auto st = Helv(10);
    st.color = "1 0 0 rg";
    EXPECT_NE(fa::TextField(100, 20, "x", st).find("1 0 0 rg"),
              std::string::npos);
}
