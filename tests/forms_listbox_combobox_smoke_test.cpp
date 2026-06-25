// =============================================================================
// forms_listbox_combobox_smoke_test — beat F5 of the Forms cluster.
// ListBoxField + ComboBoxField, both extending ChoiceField. Fast
// beat; smoke exercises ctor + class-specific accessors + that
// the ChoiceField surface (Options collection, Selected, Value)
// inherits correctly.
// =============================================================================

#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsListBoxComboBoxSmoke, ListBoxCtorAndTopIndex) {
    Document doc;
    ListBoxField l{doc, Rectangle{0, 0, 100, 80, false}};
    EXPECT_NEAR(l.Rect().Height(), 80.0, 1e-9);
    EXPECT_EQ(l.TopIndex(), 0);

    l.TopIndex(3);
    EXPECT_EQ(l.TopIndex(), 3);
}

TEST(FormsListBoxComboBoxSmoke, ListBoxInheritsOptionsAndSelected) {
    Document doc;
    ListBoxField l{doc, Rectangle{0, 0, 100, 80, false}};
    l.AddOption("Red");
    l.AddOption("Green");
    l.AddOption("Blue");
    ASSERT_EQ(l.Options().Count(), 3);

    // Selected setter is canonical-shadowed; getter inherited
    // from ChoiceField via the `using` declaration.
    l.Selected(1);
    EXPECT_EQ(l.Selected(), 1);
}

TEST(FormsListBoxComboBoxSmoke, ComboBoxCtorsAndFlags) {
    Document doc;
    ComboBoxField c1;
    ComboBoxField c2{doc};
    ComboBoxField c3{doc, Rectangle{0, 0, 80, 20, false}};

    EXPECT_FALSE(c1.Editable());
    EXPECT_TRUE(c1.SpellCheck());

    c1.Editable(true);
    c1.SpellCheck(false);
    EXPECT_TRUE(c1.Editable());
    EXPECT_FALSE(c1.SpellCheck());

    EXPECT_NEAR(c3.Rect().Width(), 80.0, 1e-9);
}

TEST(FormsListBoxComboBoxSmoke, ComboBoxInheritsChoiceFieldSurface) {
    Document doc;
    ComboBoxField c{doc};
    c.AddOption("v1", "First");
    c.AddOption("v2", "Second");
    EXPECT_EQ(c.Options().Count(), 2);
    EXPECT_EQ(c.Options()[0].Value(), "v1");
    EXPECT_EQ(c.Options()[1].Name(),  "Second");

    c.Value("v2");
    EXPECT_EQ(c.Value(), "v2");
}
