// =============================================================================
// forms_load_on_open_smoke_test — Document::Form() reads an existing
// /AcroForm /Fields back into the Form (load-on-open). Verifies the
// add → save → reopen → read-back round-trip across field types, plus
// remove-after-reopen and the lossless re-save of loaded fields.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Forms::CheckboxField;
using Aspose::Pdf::Forms::ComboBoxField;
using Aspose::Pdf::Forms::Field;
using Aspose::Pdf::Forms::TextBoxField;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("asp_form_loo_" + tag + ".pdf"))
        .string();
}

Rectangle Rect(double a, double b, double c, double d) {
    return Rectangle(a, b, c, d, false);
}

// Find a loaded field by partial name (Form::Fields() returns Field*).
Field* FieldByName(Document& doc, const std::string& name) {
    for (Field* f : doc.Form().Fields()) {
        if (f != nullptr && f->PartialName() == name) return f;
    }
    return nullptr;
}

}  // namespace

TEST(FormsLoadOnOpen, TextFieldRoundTrip) {
    const std::string out = TmpPath("text");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tf{doc, Rect(100, 600, 300, 620)};
        tf.Value("Jane Doe");
        doc.Form().Add(tf, "fullname", 1);
        doc.Save(out);
    }

    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_TRUE(re.Form().HasField("fullname"));

    Field* f = FieldByName(re, "fullname");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->Value(), "Jane Doe");
    EXPECT_NE(dynamic_cast<TextBoxField*>(f), nullptr);
    std::filesystem::remove(out);
}

TEST(FormsLoadOnOpen, CheckBoxRoundTrip) {
    const std::string out = TmpPath("checkbox");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField cb{doc, Rect(100, 560, 120, 580)};
        cb.Checked(true);
        doc.Form().Add(cb, "agree", 1);
        doc.Save(out);
    }

    Document re{out};
    EXPECT_TRUE(re.Form().HasField("agree"));
    Field* f = FieldByName(re, "agree");
    ASSERT_NE(f, nullptr);
    auto* cb = dynamic_cast<CheckboxField*>(f);
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->Checked());
    std::filesystem::remove(out);
}

TEST(FormsLoadOnOpen, ComboBoxOptionsRoundTrip) {
    const std::string out = TmpPath("combo");
    {
        Document doc{HelloWorldPdf()};
        ComboBoxField combo{doc};
        combo.AddOption("Red");
        combo.AddOption("Green");
        combo.AddOption("Blue");
        combo.Value("Green");
        doc.Form().Add(combo, "color", 1);
        doc.Save(out);
    }

    Document re{out};
    Field* f = FieldByName(re, "color");
    ASSERT_NE(f, nullptr);
    auto* combo = dynamic_cast<ComboBoxField*>(f);
    ASSERT_NE(combo, nullptr);
    EXPECT_EQ(combo->Options().Count(), 3);
    EXPECT_EQ(combo->Value(), "Green");
    EXPECT_EQ(combo->Selected(), 1);  // index of "Green"
    std::filesystem::remove(out);
}

TEST(FormsLoadOnOpen, MultipleFieldsRoundTrip) {
    const std::string out = TmpPath("multi");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField a{doc, Rect(100, 600, 300, 620)};
        a.Value("first");
        TextBoxField b{doc, Rect(100, 560, 300, 580)};
        b.Value("second");
        TextBoxField c{doc, Rect(100, 520, 300, 540)};
        // c left empty
        doc.Form().Add(a, "f1", 1);
        doc.Form().Add(b, "f2", 1);
        doc.Form().Add(c, "f3", 1);
        doc.Save(out);
    }

    Document re{out};
    EXPECT_EQ(re.Form().Count(), 3);
    EXPECT_EQ(FieldByName(re, "f1")->Value(), "first");
    EXPECT_EQ(FieldByName(re, "f2")->Value(), "second");
    EXPECT_EQ(FieldByName(re, "f3")->Value(), "");
    std::filesystem::remove(out);
}

TEST(FormsLoadOnOpen, RemoveAfterReopenDropsField) {
    const std::string out = TmpPath("remove1");
    const std::string out2 = TmpPath("remove2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField a{doc, Rect(100, 600, 300, 620)};
        TextBoxField b{doc, Rect(100, 560, 300, 580)};
        doc.Form().Add(a, "keep", 1);
        doc.Form().Add(b, "drop", 1);
        doc.Save(out);
    }

    // Reopen, remove one loaded field, re-save.
    {
        Document re{out};
        ASSERT_EQ(re.Form().Count(), 2);
        re.Form().Delete("drop");
        EXPECT_EQ(re.Form().Count(), 1);
        re.Save(out2);
    }

    // The survivor persists; the removed field is gone.
    Document re2{out2};
    EXPECT_EQ(re2.Form().Count(), 1);
    EXPECT_TRUE(re2.Form().HasField("keep"));
    EXPECT_FALSE(re2.Form().HasField("drop"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsLoadOnOpen, EditLoadedTextValuePersists) {
    const std::string out = TmpPath("edit1");
    const std::string out2 = TmpPath("edit2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tf{doc, Rect(100, 600, 300, 620)};
        tf.Value("before");
        doc.Form().Add(tf, "name", 1);
        doc.Save(out);
    }

    // Reopen, edit the loaded field's value, re-save.
    {
        Document re{out};
        Field* f = FieldByName(re, "name");
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(f->Value(), "before");
        f->Value("after");
        re.Save(out2);
    }

    Document re2{out2};
    EXPECT_EQ(re2.Form().Count(), 1);
    EXPECT_EQ(FieldByName(re2, "name")->Value(), "after");
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsLoadOnOpen, EditLoadedCheckBoxPersists) {
    const std::string out = TmpPath("cbedit1");
    const std::string out2 = TmpPath("cbedit2");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField cb{doc, Rect(100, 560, 120, 580)};
        cb.Checked(false);
        doc.Form().Add(cb, "agree", 1);
        doc.Save(out);
    }

    {
        Document re{out};
        auto* cb = dynamic_cast<CheckboxField*>(FieldByName(re, "agree"));
        ASSERT_NE(cb, nullptr);
        ASSERT_FALSE(cb->Checked());
        cb->Checked(true);
        re.Save(out2);
    }

    Document re2{out2};
    auto* cb2 = dynamic_cast<CheckboxField*>(FieldByName(re2, "agree"));
    ASSERT_NE(cb2, nullptr);
    EXPECT_TRUE(cb2->Checked());
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsLoadOnOpen, FlattenRemovesAcroFormAfterReload) {
    const std::string out = TmpPath("flat1");
    const std::string out2 = TmpPath("flat2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField a{doc, Rect(100, 600, 300, 620)};
        a.Value("x");
        TextBoxField b{doc, Rect(100, 560, 300, 580)};
        doc.Form().Add(a, "f1", 1);
        doc.Form().Add(b, "f2", 1);
        doc.Save(out);
    }

    // Reopen, flatten, re-save.
    {
        Document re{out};
        ASSERT_EQ(re.Form().Count(), 2);
        re.Form().Flatten();
        EXPECT_EQ(re.Form().Count(), 0);
        re.Save(out2);
    }

    Document re2{out2};
    EXPECT_EQ(re2.Form().Count(), 0);
    EXPECT_FALSE(re2.Form().HasField("f1"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsLoadOnOpen, ReadOnlyResaveIsLossless) {
    const std::string out = TmpPath("lossless1");
    const std::string out2 = TmpPath("lossless2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField a{doc, Rect(100, 600, 300, 620)};
        a.Value("payload");
        doc.Form().Add(a, "only", 1);
        doc.Save(out);
    }

    // Open (loads the field), then save without changes.
    {
        Document re{out};
        ASSERT_EQ(re.Form().Count(), 1);
        re.Save(out2);
    }

    Document re2{out2};
    EXPECT_EQ(re2.Form().Count(), 1);
    EXPECT_EQ(FieldByName(re2, "only")->Value(), "payload");
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}
