// =============================================================================
// forms_button_family_smoke_test — beat F4 of the Forms cluster.
// ChoiceField intermediate base + ButtonField + CheckboxField +
// RadioButtonField + RadioButtonOptionField.
//
// ChoiceField is the abstract base for radio buttons (this beat)
// and listbox/combobox (F5). Ships its ctors + AddOption /
// DeleteOption / Options collection.
// =============================================================================

#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/box_style.hpp>
#include <aspose/pdf/forms/button_field.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/forms/icon_caption_position.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/radio_button_option_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsButtonFamilySmoke, ButtonFieldDefaultsAndCaptions) {
    Document doc;
    ButtonField b{doc, Rectangle{0, 0, 100, 30, false}};
    EXPECT_TRUE(b.NormalCaption().empty());
    EXPECT_EQ(b.ICPosition(), IconCaptionPosition::NoIcon);

    b.NormalCaption("OK");
    b.RolloverCaption("Click me");
    b.AlternateCaption("Pressed");
    b.ICPosition(IconCaptionPosition::CaptionToTheRight);

    EXPECT_EQ(b.NormalCaption(),     "OK");
    EXPECT_EQ(b.RolloverCaption(),   "Click me");
    EXPECT_EQ(b.AlternateCaption(),  "Pressed");
    EXPECT_EQ(b.ICPosition(), IconCaptionPosition::CaptionToTheRight);

    // IconFit accessor is read-only canonical but mutable through
    // the returned reference.
    b.IconFit().LeftoverLeft(0.75);
    EXPECT_DOUBLE_EQ(b.IconFit().LeftoverLeft(), 0.75);
}

TEST(FormsButtonFamilySmoke, CheckboxFieldDefaultsAndState) {
    Document doc;
    CheckboxField c{doc};
    EXPECT_EQ(c.Style(),        BoxStyle::Check);
    EXPECT_FALSE(c.Checked());
    EXPECT_EQ(c.ActiveState(),  "Yes");
    EXPECT_EQ(c.ExportValue(),  "Yes");

    c.Style(BoxStyle::Cross);
    c.Checked(true);
    c.ActiveState("On");
    c.ExportValue("True");
    c.Value("On");

    EXPECT_EQ(c.Style(),        BoxStyle::Cross);
    EXPECT_TRUE(c.Checked());
    EXPECT_EQ(c.ActiveState(),  "On");
    EXPECT_EQ(c.ExportValue(),  "True");
    EXPECT_EQ(c.Value(),        "On");

    const auto states = c.AllowedStates();
    ASSERT_EQ(states.size(), 2u);
    EXPECT_EQ(states[0], "Off");
    EXPECT_EQ(states[1], "On");

    // Clone returns nullptr (v1 stub).
    EXPECT_EQ(c.Clone(), nullptr);

    // AddOption variants are v1 no-ops; just exercise they don't
    // throw.
    c.AddOption("Off");
    c.AddOption("Yes", Rectangle{0, 0, 10, 10, false});
    c.AddOption("No", 1, Rectangle{0, 0, 10, 10, false});
    SUCCEED();
}

TEST(FormsButtonFamilySmoke, CheckboxValueShadowsField) {
    Document doc;
    CheckboxField c{doc};
    c.Value("Yes");
    // CheckboxField::Value chains through Field storage —
    // confirm via Field& base ref.
    Field& base = c;
    EXPECT_EQ(base.Value(), "Yes");
}

TEST(FormsButtonFamilySmoke, ChoiceFieldOptionsAddDelete) {
    Document doc;
    // ChoiceField is abstract intermediate; construct through
    // RadioButtonField which extends it.
    RadioButtonField r{doc};
    EXPECT_EQ(r.Options().Count(), 0);

    r.AddOption("Apple");
    r.AddOption("Banana");
    EXPECT_EQ(r.Options().Count(), 2);
    EXPECT_EQ(r.Options()[0].Name(),  "Apple");
    EXPECT_EQ(r.Options()[1].Name(),  "Banana");

    r.DeleteOption("Apple");
    EXPECT_EQ(r.Options().Count(), 1);
    EXPECT_EQ(r.Options()[0].Name(), "Banana");
}

TEST(FormsButtonFamilySmoke, ChoiceFieldExportNamePairAddOption) {
    Document doc;
    RadioButtonField r{doc};
    r.AddOption("export-1", "Friendly Name");
    ASSERT_EQ(r.Options().Count(), 1);
    EXPECT_EQ(r.Options()[0].Value(), "export-1");
    EXPECT_EQ(r.Options()[0].Name(),  "Friendly Name");
}

TEST(FormsButtonFamilySmoke, RadioButtonFieldSelectionAndStyle) {
    Document doc;
    RadioButtonField r{doc};
    EXPECT_EQ(r.Style(), BoxStyle::Circle);
    EXPECT_FALSE(r.NoToggleToOff());
    EXPECT_EQ(r.Selected(), -1);

    r.Style(BoxStyle::Diamond);
    r.NoToggleToOff(true);
    r.Selected(2);

    EXPECT_EQ(r.Style(),    BoxStyle::Diamond);
    EXPECT_TRUE(r.NoToggleToOff());
    EXPECT_EQ(r.Selected(), 2);
}

TEST(FormsButtonFamilySmoke, RadioButtonAddOptionField) {
    Document doc;
    RadioButtonField r{doc};
    RadioButtonOptionField a;
    a.OptionName("Yes");
    RadioButtonOptionField b;
    b.OptionName("No");

    r.Add(std::move(a));
    r.Add(std::move(b));
    EXPECT_EQ(r.Options().Count(), 2);
    EXPECT_EQ(r.Options()[0].Name(),  "Yes");
    EXPECT_EQ(r.Options()[1].Name(),  "No");
}

TEST(FormsButtonFamilySmoke, RadioButtonOptionFieldStyle) {
    RadioButtonOptionField o;
    EXPECT_EQ(o.Style(), BoxStyle::Circle);
    o.Style(BoxStyle::Cross);
    o.OptionName("Maybe");
    EXPECT_EQ(o.Style(),      BoxStyle::Cross);
    EXPECT_EQ(o.OptionName(), "Maybe");
}
