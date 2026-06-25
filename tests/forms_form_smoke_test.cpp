// =============================================================================
// forms_form_smoke_test — beat F7 of the Forms cluster. Form
// main class + nested FlattenSettings + nested
// SignDependentElementsRenderingModes enum + XFA wire-through +
// Document.Form accessor + WidgetAnnotation.Parent undrop.
//
// The Form is the entry-point that surfaces all the F2..F6
// per-class work to user-facing code.
// =============================================================================

#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/form_type.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/forms/xfa.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsFormSmoke, DocumentFormAccessorIsStable) {
    Document doc;
    Form& f1 = doc.Form();
    Form& f2 = doc.Form();
    EXPECT_EQ(&f1, &f2);
    EXPECT_EQ(f1.Count(), 0);
}

TEST(FormsFormSmoke, NestedSignDependentEnumValues) {
    EXPECT_EQ(static_cast<int>(
        Form::SignDependentElementsRenderingModes::RenderFormAsUnsigned), 0);
    EXPECT_EQ(static_cast<int>(
        Form::SignDependentElementsRenderingModes::RenderFormAsSigned), 1);
}

TEST(FormsFormSmoke, NestedFlattenSettingsDefaultsAndRoundtrip) {
    Form::FlattenSettings fs;
    EXPECT_TRUE(fs.UpdateAppearances());
    EXPECT_TRUE(fs.CallEvents());
    EXPECT_FALSE(fs.HideButtons());
    EXPECT_FALSE(fs.ApplyRedactions());

    fs.UpdateAppearances(false);
    fs.CallEvents(false);
    fs.HideButtons(true);
    fs.ApplyRedactions(true);
    EXPECT_FALSE(fs.UpdateAppearances());
    EXPECT_FALSE(fs.CallEvents());
    EXPECT_TRUE(fs.HideButtons());
    EXPECT_TRUE(fs.ApplyRedactions());
}

TEST(FormsFormSmoke, PublicFieldDefault) {
    Form f;
    EXPECT_EQ(f.SignDependentElementsRenderingModeWhenConverted,
              Form::SignDependentElementsRenderingModes::RenderFormAsUnsigned);
    f.SignDependentElementsRenderingModeWhenConverted =
        Form::SignDependentElementsRenderingModes::RenderFormAsSigned;
    EXPECT_EQ(f.SignDependentElementsRenderingModeWhenConverted,
              Form::SignDependentElementsRenderingModes::RenderFormAsSigned);
}

TEST(FormsFormSmoke, AddAndQueryFields) {
    Document doc;
    Form& form = doc.Form();

    TextBoxField name_field{doc};
    name_field.PartialName("FullName");
    form.Add(name_field, 1);

    TextBoxField email_field{doc};
    email_field.PartialName("Email");
    form.Add(email_field);

    CheckboxField subscribe{doc};
    subscribe.PartialName("Subscribe");
    Field* added = form.Add(subscribe, "Subscribe", 1);
    EXPECT_EQ(added, &subscribe);

    EXPECT_EQ(form.Count(), 3);
    EXPECT_TRUE(form.HasField(name_field));
    EXPECT_TRUE(form.HasField("Email"));
    EXPECT_TRUE(form.HasField("Subscribe", true));
    EXPECT_FALSE(form.HasField("DoesNotExist"));
    EXPECT_FALSE(form.HasField("DoesNotExist", true));

    // Indexer by name + by index.
    auto* by_name = form["Email"];
    ASSERT_NE(by_name, nullptr);
    EXPECT_EQ(by_name, static_cast<Annotations::WidgetAnnotation*>(
                           &email_field));
    auto* by_index = form[0];
    EXPECT_EQ(by_index, static_cast<Annotations::WidgetAnnotation*>(
                            &name_field));
    EXPECT_EQ(form["Missing"], nullptr);
    EXPECT_EQ(form[99], nullptr);
}

TEST(FormsFormSmoke, DeleteByFieldAndName) {
    Document doc;
    Form& form = doc.Form();

    TextBoxField a{doc};
    a.PartialName("A");
    form.Add(a);
    TextBoxField b{doc};
    b.PartialName("B");
    form.Add(b);
    EXPECT_EQ(form.Count(), 2);

    form.Delete(a);
    EXPECT_EQ(form.Count(), 1);
    EXPECT_FALSE(form.HasField(a));
    EXPECT_TRUE(form.HasField("B"));

    form.Delete("B");
    EXPECT_EQ(form.Count(), 0);
}

TEST(FormsFormSmoke, TypeAndAutoFlags) {
    Form f;
    EXPECT_EQ(f.Type(), FormType::Standard);
    f.Type(FormType::Static);
    EXPECT_EQ(f.Type(), FormType::Static);

    f.AutoRecalculate(true);
    f.AutoRestoreForm(true);
    f.IgnoreNeedsRendering(true);
    f.RemovePermission(true);
    f.EmulateRequierdGroups(true);
    f.SignaturesExist(true);
    f.SignaturesAppendOnly(true);
    EXPECT_TRUE(f.AutoRecalculate());
    EXPECT_TRUE(f.AutoRestoreForm());
    EXPECT_TRUE(f.IgnoreNeedsRendering());
    EXPECT_TRUE(f.RemovePermission());
    EXPECT_TRUE(f.EmulateRequierdGroups());
    EXPECT_TRUE(f.SignaturesExist());
    EXPECT_TRUE(f.SignaturesAppendOnly());

    EXPECT_FALSE(f.NeedsRendering());
}

TEST(FormsFormSmoke, XfaAccessorRoundtrip) {
    Document doc;
    Form& form = doc.Form();
    EXPECT_FALSE(form.HasXfa());
    EXPECT_TRUE(form.XFA().FieldNames().empty());
    EXPECT_EQ(form.XFA()["any-path"], "");
}

TEST(FormsFormSmoke, WidgetAnnotationParentRoundtrip) {
    Document doc;
    TextBoxField f{doc};
    Annotations::WidgetAnnotation& widget = f;
    EXPECT_EQ(widget.Parent(), nullptr);
    widget.Parent(&f);
    EXPECT_EQ(widget.Parent(), &f);
}

TEST(FormsFormSmoke, FieldsListAndCalculatedFields) {
    Document doc;
    Form& form = doc.Form();
    TextBoxField a{doc}; a.PartialName("A"); form.Add(a);
    TextBoxField b{doc}; b.PartialName("B"); form.Add(b);

    auto all = form.Fields();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], &a);
    EXPECT_EQ(all[1], &b);

    form.CalculatedFields({&b, &a});
    SUCCEED();
}
