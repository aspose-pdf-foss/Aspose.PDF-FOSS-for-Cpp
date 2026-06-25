// =============================================================================
// Go-parity: form_test.go → canonical C++ Document::Form + field hierarchy.
//
// Ported (canonical): Form present + empty on a plain doc, add fields + Count,
// TextBoxField value, ComboBoxField options + value, and a save-through that
// persists the field count.
// Skipped: Go-specific SetValue-validation error returns, NeedAppearances
// toggling internals, Cyrillic AP-regeneration, and read-from-fixture tests
// that assert a specific Go form fixture's field layout.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_form_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestDocumentFormNonNilOnPlainPDF — a plain doc has an empty form.
TEST(GoForm, EmptyFormOnPlainDoc) {
    Document doc;
    EXPECT_EQ(doc.Form().Count(), 0);
}

// TestFormFieldsCount — adding fields grows the form.
TEST(GoForm, AddFieldsCount) {
    Document doc;
    TextBoxField name{doc};
    name.PartialName("FullName");
    doc.Form().Add(name);
    CheckboxField subscribe{doc};
    subscribe.PartialName("subscribe");
    doc.Form().Add(subscribe);
    EXPECT_EQ(doc.Form().Count(), 2);
}

// TestTextBoxFieldRoundTrip — text value set/get.
TEST(GoForm, TextBoxValue) {
    Document doc;
    TextBoxField name{doc};
    name.PartialName("FullName");
    name.Value("Jane Doe");
    doc.Form().Add(name);
    EXPECT_EQ(name.Value(), "Jane Doe");
}

// TestComboBoxFieldRoundTrip — choice options + selected value.
TEST(GoForm, ComboBoxOptionsAndValue) {
    Document doc;
    ComboBoxField combo{doc};
    combo.AddOption("v1");
    combo.AddOption("v2");
    ASSERT_GE(combo.Options().Count(), 2);
    combo.Value("v2");
    EXPECT_EQ(combo.Value(), "v2");
}

// TestFormFillIntegration — added fields persist through Save into /AcroForm.
TEST(GoForm, FieldsPersistThroughSave) {
    const std::string out = TmpOut("persist");
    {
        Document doc;
        TextBoxField name{doc};
        name.PartialName("FullName");
        name.Value("Jane");
        doc.Form().Add(name);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Form().Count(), 1);
    std::filesystem::remove(out);
}
