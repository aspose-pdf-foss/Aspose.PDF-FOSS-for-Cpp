// =============================================================================
// forms_text_input_family_smoke_test — beat F3 of the Forms cluster.
// 5 concrete text-input field subclasses sharing TextBoxField:
//   PasswordBoxField + FileSelectBoxField — empty subclasses
//     (canonical /Ff flag handled at save time)
//   NumberField — adds AllowedChars
//   DateField — adds Init(Page) + DateFormat; DateTime Value
//     dropped via translations cascade
//   RichTextBoxField — adds Style / RichTextValue / FormattedValue
//     / shadowed Value + Justify
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/justification.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/date_field.hpp>
#include <aspose/pdf/forms/file_select_box_field.hpp>
#include <aspose/pdf/forms/number_field.hpp>
#include <aspose/pdf/forms/password_box_field.hpp>
#include <aspose/pdf/forms/rich_text_box_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsTextInputFamilySmoke, PasswordAndFileSelectInheritTextBoxSurface) {
    PasswordBoxField p;
    FileSelectBoxField f;
    // Inherited from TextBoxField (which inherits from Field).
    EXPECT_FALSE(p.Multiline());
    EXPECT_TRUE(p.SpellCheck());
    p.MaxLen(64);
    EXPECT_EQ(p.MaxLen(), 64);

    f.PartialName("Resume");          // inherited from Field
    EXPECT_EQ(f.PartialName(), "Resume");
}

TEST(FormsTextInputFamilySmoke, NumberFieldCtorsAndAllowedChars) {
    Document doc;
    NumberField n{doc, Rectangle{0, 0, 50, 20, false}};
    EXPECT_NEAR(n.Rect().Width(), 50.0, 1e-9);
    EXPECT_TRUE(n.AllowedChars().empty());
    n.AllowedChars("0123456789.,-");
    EXPECT_EQ(n.AllowedChars(), "0123456789.,-");
}

TEST(FormsTextInputFamilySmoke, DateFieldCtorsAndDateFormat) {
    Document doc;
    DateField d{doc};
    DateField d2{doc, Rectangle{0, 0, 100, 20, false}};
    EXPECT_NEAR(d2.Rect().Width(), 100.0, 1e-9);
    EXPECT_TRUE(d.DateFormat().empty());

    d.DateFormat("yyyy-mm-dd");
    EXPECT_EQ(d.DateFormat(), "yyyy-mm-dd");
}

TEST(FormsTextInputFamilySmoke, RichTextBoxAccessors) {
    RichTextBoxField r;
    EXPECT_TRUE(r.Style().empty());
    EXPECT_TRUE(r.RichTextValue().empty());
    EXPECT_TRUE(r.FormattedValue().empty());
    EXPECT_TRUE(r.Value().empty());
    EXPECT_EQ(r.Justify(),
              Annotations::Justification::LeftJustify);

    r.Style("font-weight:bold");
    r.RichTextValue("<b>Hello</b>");
    r.FormattedValue("Hello (bold)");
    r.Value("Hello");
    r.Justify(Annotations::Justification::Centered);

    EXPECT_EQ(r.Style(),          "font-weight:bold");
    EXPECT_EQ(r.RichTextValue(),  "<b>Hello</b>");
    EXPECT_EQ(r.FormattedValue(), "Hello (bold)");
    EXPECT_EQ(r.Value(),          "Hello");
    EXPECT_EQ(r.Justify(),
              Annotations::Justification::Centered);
}

TEST(FormsTextInputFamilySmoke, RichTextValueShadowsTextBoxValue) {
    // RichTextBoxField::Value chains through TextBoxField storage —
    // confirm by accessing via the TextBoxField base reference.
    RichTextBoxField r;
    r.Value("plain");
    TextBoxField& base = r;
    EXPECT_EQ(base.Value(), "plain");
}
