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
