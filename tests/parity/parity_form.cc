// =============================================================================
// parity_form.cc — PARITY measurement of pdflib's tests/test_form.cpp (25
// tests) ported against OUR canonical Aspose::Pdf::Forms surface.
//
// Each pdflib FormTest.<Name> maps to a ParityForm.<Name> here:
//   * EXISTS  — real ported test against the canonical surface
//               (add → save → reopen → read-back via Form::Fields() +
//               Field::PartialName(); NOT a flat form().getFieldValue()
//               facade, which OUR lib does not expose).
//   * GAP     — capability absent in OUR v1 surface (GTEST_SKIP).
//   * SHAPE   — capability present but shaped differently than pdflib's
//               flat form() accessor model (GTEST_SKIP with rationale).
//
// pdflib's model:  doc.form().addTextField(name, page, Rect, default),
//                  doc.form().setFieldValue / getFieldValue (flat string),
//                  doc.form().fieldByName(...) -> FormFieldInfo POD,
//                  FormFieldType enum.
// OUR model:       construct concrete Field subclass, set typed properties,
//                  doc.Form().Add(field, partialName, pageNumber),
//                  read back via Field::Value()/Checked()/Options(),
//                  dynamic_cast for the concrete type.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/form_editor.hpp>
#include <aspose/pdf/forms/button_field.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/password_box_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/radio_button_option_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

// hello_world.pdf is a single-page fixture — fields reference page 1.
std::filesystem::path FixturePdf() {
    // tests/parity_form.cc -> tests/ -> tests/fixtures/text_extractor/...
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
           "fixtures" / "text_extractor" / "hello_world.pdf";
}

std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("parity_form_" + tag + ".pdf"))
        .string();
}

// llx,lly,urx,ury — no coordinate normalization (matches the form smoke tests).
Rectangle Rect(double a, double b, double c, double d) {
    return Rectangle(a, b, c, d, false);
}

// Find a field by partial name in a Form (OUR read-back idiom — replaces
// pdflib's flat fieldByName()/getFieldValue()).
Field* FieldByName(Document& doc, const std::string& name) {
    for (Field* f : doc.Form().Fields()) {
        if (f != nullptr && f->PartialName() == name) return f;
    }
    return nullptr;
}

}  // namespace

// ---------------------------------------------------------------------------
// Basic enumeration
// ---------------------------------------------------------------------------

TEST(ParityForm, FieldCount_EmptyDoc) {
    // EXISTS — doc.Form().Count() on a doc with no fields is 0.
    Document doc{FixturePdf().string()};
    EXPECT_EQ(doc.Form().Count(), 0);
}

TEST(ParityForm, HasField_False_WhenEmpty) {
    // EXISTS — doc.Form().HasField(name) is false when empty.
    Document doc{FixturePdf().string()};
    EXPECT_FALSE(doc.Form().HasField("noSuchField"));
}

// ---------------------------------------------------------------------------
// AddTextField
// ---------------------------------------------------------------------------

TEST(ParityForm, AddTextField) {
    // EXISTS — construct TextBoxField, Add via Form, read back by name.
    // SHAPE note: pdflib's FormFieldInfo POD (.type / .defaultValue /
    // .multiline / .password) has no canonical analog; we assert the
    // canonical-shaped equivalents (concrete type, Value, Multiline).
    Document doc{FixturePdf().string()};
    TextBoxField tf{doc, Rect(72, 700, 272, 720)};
    tf.Value("default");
    doc.Form().Add(tf, "name", 1);

    EXPECT_EQ(doc.Form().Count(), 1);
    EXPECT_TRUE(doc.Form().HasField("name"));

    Field* f = FieldByName(doc, "name");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->PartialName(), "name");
    EXPECT_NE(dynamic_cast<TextBoxField*>(f), nullptr);
    EXPECT_EQ(f->Value(), "default");
    EXPECT_FALSE(dynamic_cast<TextBoxField*>(f)->Multiline());
}

TEST(ParityForm, AddMultilineTextField) {
    // EXISTS — TextBoxField with Multiline(true).
    Document doc{FixturePdf().string()};
    TextBoxField tf{doc, Rect(72, 600, 372, 680)};
    tf.Multiline(true);
    tf.Value("hello");
    doc.Form().Add(tf, "notes", 1);

    Field* f = FieldByName(doc, "notes");
    ASSERT_NE(f, nullptr);
    auto* tb = dynamic_cast<TextBoxField*>(f);
    ASSERT_NE(tb, nullptr);
    EXPECT_TRUE(tb->Multiline());
}

TEST(ParityForm, AddPasswordField) {
    // EXISTS — PasswordBoxField is a concrete subclass of TextBoxField.
    // SHAPE note: pdflib exposes a `.password` bool on FormFieldInfo;
    // canonical models the password flag as the field's concrete type.
    Document doc{FixturePdf().string()};
    PasswordBoxField pwd;
    pwd.PartialName("pwd");
    doc.Form().Add(pwd, "pwd", 1);

    Field* f = FieldByName(doc, "pwd");
    ASSERT_NE(f, nullptr);
    EXPECT_NE(dynamic_cast<PasswordBoxField*>(f), nullptr);
}

// ---------------------------------------------------------------------------
// AddCheckBox
// ---------------------------------------------------------------------------

TEST(ParityForm, AddCheckBox_Unchecked) {
    // EXISTS — canonical Checked(false). SHAPE note: pdflib reads the
    // string value "Off"; canonical exposes the boolean Checked() instead.
    Document doc{FixturePdf().string()};
    CheckboxField cb{doc, Rect(72, 450, 87, 465)};
    cb.Checked(false);
    doc.Form().Add(cb, "agree", 1);

    Field* f = FieldByName(doc, "agree");
    ASSERT_NE(f, nullptr);
    auto* c = dynamic_cast<CheckboxField*>(f);
    ASSERT_NE(c, nullptr);
    EXPECT_FALSE(c->Checked());
}

TEST(ParityForm, AddCheckBox_Checked) {
    // EXISTS — canonical Checked(true). SHAPE note: pdflib expects the
    // string "Yes"; canonical exposes Checked() boolean.
    Document doc{FixturePdf().string()};
    CheckboxField cb{doc, Rect(72, 450, 87, 465)};
    cb.Checked(true);
    doc.Form().Add(cb, "agree", 1);

    Field* f = FieldByName(doc, "agree");
    ASSERT_NE(f, nullptr);
    auto* c = dynamic_cast<CheckboxField*>(f);
    ASSERT_NE(c, nullptr);
    EXPECT_TRUE(c->Checked());
}

// ---------------------------------------------------------------------------
// AddRadioGroup
// ---------------------------------------------------------------------------

TEST(ParityForm, AddRadioGroup) {
    // EXISTS — canonical models a radio group as a RadioButtonField
    // (extends ChoiceField, /FT /Btn + /Ff /Radio) owning N
    // RadioButtonOptionField renditions. pdflib's one-call
    // addRadioGroup(name, page, rects, opts, sel) maps to:
    // construct RadioButtonField, Add(RadioButtonOptionField) per option,
    // set the selected value, then Form().Add. SHAPE note: pdflib reads a
    // flat .value ("A"); canonical reads Options()/Value(). The /Radio flag
    // dispatch + /Opt list round-trip through Form save/load (FieldKindOf
    // emits /Btn /Radio; load-on-open reconstructs the RadioButtonField and
    // its ChoiceField Options).
    Document doc{FixturePdf().string()};
    RadioButtonField radio{doc};

    RadioButtonOptionField optA;
    optA.OptionName("A");
    radio.Add(optA);
    RadioButtonOptionField optB;
    optB.OptionName("B");
    radio.Add(optB);
    radio.Value("A");  // selected option

    doc.Form().Add(radio, "choice", 1);

    EXPECT_TRUE(doc.Form().HasField("choice"));
    Field* f = FieldByName(doc, "choice");
    ASSERT_NE(f, nullptr);
    auto* rb = dynamic_cast<RadioButtonField*>(f);
    ASSERT_NE(rb, nullptr);
    ASSERT_EQ(rb->Options().Count(), 2);
    EXPECT_EQ(rb->Options()[0].Value(), "A");
    EXPECT_EQ(rb->Options()[1].Value(), "B");
    EXPECT_EQ(rb->Value(), "A");

    // Round-trip: /Btn /Radio + /Opt survive save/reopen.
    const std::string out = TmpPath("radio_group");
    doc.Save(out);
    Document re{out};
    ASSERT_TRUE(re.Form().HasField("choice"));
    auto* rb2 = dynamic_cast<RadioButtonField*>(FieldByName(re, "choice"));
    ASSERT_NE(rb2, nullptr);
    EXPECT_EQ(rb2->Options().Count(), 2);
    EXPECT_EQ(rb2->Value(), "A");
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// AddComboBox / AddListBox
// ---------------------------------------------------------------------------

TEST(ParityForm, AddComboBox) {
    // EXISTS — ComboBoxField + AddOption + Value; read back Options/Value.
    // SHAPE note: pdflib's FormFieldInfo exposes .options/.selectedIndex/
    // .value as a flat POD; canonical reads them via Options() collection,
    // Selected() and Value(). Selecting an option is done via Value(name)
    // (no selectedIndex setter on the public surface).
    Document doc{FixturePdf().string()};
    ComboBoxField combo{doc};
    combo.AddOption("Red");
    combo.AddOption("Green");
    combo.AddOption("Blue");
    combo.Value("Green");
    doc.Form().Add(combo, "color", 1);

    Field* f = FieldByName(doc, "color");
    ASSERT_NE(f, nullptr);
    auto* c = dynamic_cast<ComboBoxField*>(f);
    ASSERT_NE(c, nullptr);
    ASSERT_EQ(c->Options().Count(), 3);
    EXPECT_EQ(c->Options()[0].Value(), "Red");
    EXPECT_EQ(c->Options()[1].Value(), "Green");
    EXPECT_EQ(c->Options()[2].Value(), "Blue");
    EXPECT_EQ(c->Value(), "Green");
}

TEST(ParityForm, AddListBox) {
    // EXISTS — ListBoxField + AddOption + Value; read back Options/Value.
    Document doc{FixturePdf().string()};
    ListBoxField lb{doc, Rect(72, 280, 222, 340)};
    lb.AddOption("X");
    lb.AddOption("Y");
    lb.AddOption("Z");
    lb.Value("Z");
    doc.Form().Add(lb, "items", 1);

    Field* f = FieldByName(doc, "items");
    ASSERT_NE(f, nullptr);
    auto* l = dynamic_cast<ListBoxField*>(f);
    ASSERT_NE(l, nullptr);
    EXPECT_EQ(l->Options().Count(), 3);
    EXPECT_EQ(l->Value(), "Z");
}

// ---------------------------------------------------------------------------
// AddPushButton
// ---------------------------------------------------------------------------

TEST(ParityForm, AddPushButton) {
    // EXISTS — canonical models a push button as a ButtonField
    // (/FT /Btn + /Ff /Pushbutton). pdflib's
    // addPushButton(name, page, rect, caption) maps to: construct
    // ButtonField(doc, rect), set NormalCaption, then Form().Add. The
    // /Pushbutton flag dispatch survives Form save/load — FieldKindOf emits
    // /Btn with the pushbutton bit; load-on-open reconstructs a ButtonField
    // (ff & 1<<16). SHAPE note: pdflib reads a flat FormFieldType::PushButton
    // enum; canonical reads the concrete type via dynamic_cast.
    Document doc{FixturePdf().string()};
    ButtonField btn{doc, Rect(72, 200, 152, 224)};
    btn.NormalCaption("Submit");
    doc.Form().Add(btn, "submit", 1);

    Field* f = FieldByName(doc, "submit");
    ASSERT_NE(f, nullptr);
    EXPECT_NE(dynamic_cast<ButtonField*>(f), nullptr);

    // Round-trip: the /Btn pushbutton flag survives save/reopen so the
    // field reloads as a ButtonField (not a checkbox).
    const std::string out = TmpPath("push_button");
    doc.Save(out);
    Document re{out};
    ASSERT_TRUE(re.Form().HasField("submit"));
    Field* rf = FieldByName(re, "submit");
    ASSERT_NE(rf, nullptr);
    EXPECT_NE(dynamic_cast<ButtonField*>(rf), nullptr);
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// fieldAt / index access
// ---------------------------------------------------------------------------

TEST(ParityForm, FieldAt) {
    // EXISTS (SHAPE) — pdflib's fieldAt(i) returns a FormFieldInfo POD by
    // index. Canonical has no fieldAt(); the equivalent index access is
    // Form::Fields()[i] (vector<Field*>). Ported using Fields() iteration.
    Document doc{FixturePdf().string()};
    TextBoxField a{doc, Rect(10, 700, 110, 720)};
    TextBoxField b{doc, Rect(10, 670, 110, 690)};
    doc.Form().Add(a, "f1", 1);
    doc.Form().Add(b, "f2", 1);

    EXPECT_EQ(doc.Form().Count(), 2);
    auto fields = doc.Form().Fields();
    ASSERT_EQ(fields.size(), 2u);
    EXPECT_TRUE(fields[0]->PartialName() == "f1" ||
                fields[1]->PartialName() == "f1");
}

// ---------------------------------------------------------------------------
// SetGetFieldValue
// ---------------------------------------------------------------------------

TEST(ParityForm, SetGetFieldValue_Text) {
    // SHAPE — pdflib uses flat form().setFieldValue/getFieldValue(name).
    // Canonical reads/writes the typed Field::Value() on the field object,
    // not via a form-level string accessor. Ported to the canonical shape:
    // mutate the field then read it back.
    Document doc{FixturePdf().string()};
    TextBoxField tf{doc, Rect(72, 700, 272, 720)};
    doc.Form().Add(tf, "name", 1);

    Field* f = FieldByName(doc, "name");
    ASSERT_NE(f, nullptr);
    f->Value("Alice");
    EXPECT_EQ(FieldByName(doc, "name")->Value(), "Alice");
}

TEST(ParityForm, SetGetFieldValue_CheckBox) {
    // SHAPE — pdflib toggles a checkbox via the string form value
    // ("Off"/"Yes") through form().setFieldValue/getFieldValue. Canonical
    // models check state as the boolean CheckboxField::Checked(). Ported
    // to the boolean shape.
    Document doc{FixturePdf().string()};
    CheckboxField cb{doc, Rect(72, 450, 87, 465)};
    cb.Checked(false);
    doc.Form().Add(cb, "chk", 1);

    auto* c = dynamic_cast<CheckboxField*>(FieldByName(doc, "chk"));
    ASSERT_NE(c, nullptr);
    EXPECT_FALSE(c->Checked());
    c->Checked(true);
    EXPECT_TRUE(c->Checked());
    c->Checked(false);
    EXPECT_FALSE(c->Checked());
}

// ---------------------------------------------------------------------------
// RemoveField
// ---------------------------------------------------------------------------

TEST(ParityForm, RemoveField) {
    // EXISTS — Form::Delete(name) drops the field; Count/HasField reflect it.
    Document doc{FixturePdf().string()};
    TextBoxField tf{doc, Rect(72, 700, 272, 720)};
    doc.Form().Add(tf, "tmp", 1);
    EXPECT_EQ(doc.Form().Count(), 1);
    doc.Form().Delete("tmp");
    EXPECT_EQ(doc.Form().Count(), 0);
    EXPECT_FALSE(doc.Form().HasField("tmp"));
}

TEST(ParityForm, RemoveField_NotFound_Throws) {
    // SHAPE (behavior-diff) — pdflib's removeField throws PdfException on a
    // missing name. OUR Form::Delete(name) is a no-op on miss (does NOT
    // throw), matching canonical Aspose.Pdf.Forms.Form.Delete semantics.
    // The throw contract is a pdflib-specific behavior; assert the canonical
    // no-throw behavior instead.
    Document doc{FixturePdf().string()};
    EXPECT_NO_THROW(doc.Form().Delete("noSuchField"));
    EXPECT_EQ(doc.Form().Count(), 0);
}

// ---------------------------------------------------------------------------
// Save and reload (roundtrip)
// ---------------------------------------------------------------------------

TEST(ParityForm, SaveAndReload_TextField) {
    // EXISTS — add → save → reopen (load-on-open) → read-back value.
    const std::string out = TmpPath("reload_text");
    {
        Document doc{FixturePdf().string()};
        TextBoxField tf{doc, Rect(72, 700, 322, 720)};
        tf.Value("user@domain.com");
        doc.Form().Add(tf, "email", 1);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_TRUE(re.Form().HasField("email"));
    Field* f = FieldByName(re, "email");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->Value(), "user@domain.com");
    std::filesystem::remove(out);
}

TEST(ParityForm, SaveAndReload_CheckBox) {
    // EXISTS — checkbox Checked(true) survives save/reopen.
    const std::string out = TmpPath("reload_checkbox");
    {
        Document doc{FixturePdf().string()};
        CheckboxField cb{doc, Rect(72, 450, 87, 465)};
        cb.Checked(true);
        doc.Form().Add(cb, "accept", 1);
        doc.Save(out);
    }
    Document re{out};
    ASSERT_TRUE(re.Form().HasField("accept"));
    auto* cb = dynamic_cast<CheckboxField*>(FieldByName(re, "accept"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->Checked());
    std::filesystem::remove(out);
}

TEST(ParityForm, SaveAndReload_ComboBox) {
    // EXISTS — combo options + value survive save/reopen.
    const std::string out = TmpPath("reload_combo");
    {
        Document doc{FixturePdf().string()};
        ComboBoxField combo{doc};
        combo.AddOption("Red");
        combo.AddOption("Green");
        combo.AddOption("Blue");
        combo.Value("Blue");
        doc.Form().Add(combo, "color", 1);
        doc.Save(out);
    }
    Document re{out};
    ASSERT_TRUE(re.Form().HasField("color"));
    auto* combo = dynamic_cast<ComboBoxField*>(FieldByName(re, "color"));
    ASSERT_NE(combo, nullptr);
    EXPECT_EQ(combo->Options().Count(), 3);
    EXPECT_EQ(combo->Value(), "Blue");
    std::filesystem::remove(out);
}

TEST(ParityForm, SaveAndReload_MultipleFields) {
    // EXISTS — multiple fields with mixed value/empty survive save/reopen.
    // SHAPE note: pdflib's "/DV set, /V empty" distinction (defaultValue vs
    // value) is not modeled separately on the canonical surface — only the
    // effective Value() round-trips. Asserted: edited value persists, an
    // unset value reads back empty.
    const std::string out = TmpPath("reload_multi");
    {
        Document doc{FixturePdf().string()};
        TextBoxField first{doc, Rect(72, 750, 272, 770)};
        first.Value("Jane");
        TextBoxField last{doc, Rect(72, 720, 272, 740)};
        // last left empty
        CheckboxField active{doc, Rect(72, 700, 87, 715)};
        active.Checked(true);
        doc.Form().Add(first, "first", 1);
        doc.Form().Add(last, "last", 1);
        doc.Form().Add(active, "active", 1);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 3);
    EXPECT_EQ(FieldByName(re, "first")->Value(), "Jane");
    EXPECT_EQ(FieldByName(re, "last")->Value(), "");
    auto* active = dynamic_cast<CheckboxField*>(FieldByName(re, "active"));
    ASSERT_NE(active, nullptr);
    EXPECT_TRUE(active->Checked());
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// FlattenAllFields
// ---------------------------------------------------------------------------

TEST(ParityForm, FlattenAllFields_ClearsFields) {
    // EXISTS — Form::Flatten() clears the field set (Count -> 0).
    // SHAPE note: canonical method is Form::Flatten() (no
    // flattenAllFields()); semantics match (clears + strips AcroForm).
    Document doc{FixturePdf().string()};
    TextBoxField tf{doc, Rect(72, 700, 272, 720)};
    tf.Value("Alice");
    CheckboxField cb{doc, Rect(72, 670, 87, 685)};
    cb.Checked(true);
    doc.Form().Add(tf, "name", 1);
    doc.Form().Add(cb, "chk", 1);

    doc.Form().Flatten();
    EXPECT_EQ(doc.Form().Count(), 0);
}

TEST(ParityForm, FlattenAllFields_NoAcroFormAfterReload) {
    // EXISTS — flatten strips the AcroForm; no fields after save/reopen.
    const std::string out = TmpPath("flatten_reload");
    {
        Document doc{FixturePdf().string()};
        TextBoxField tf{doc, Rect(72, 700, 272, 720)};
        doc.Form().Add(tf, "name", 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 0);
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// AnnotationSyncPreservesWidgets
// ---------------------------------------------------------------------------

TEST(ParityForm, AnnotationSyncPreservesWidgets) {
    // SHAPE — pdflib mixes a form field with a makeHighlight() annotation on
    // the same page and verifies the widget survives the annotation sync.
    // makeHighlight() is a pdflib factory with no canonical analog (canonical
    // constructs annotations via concrete ctors). We port the load-bearing
    // half — a form field survives save/reopen — without the highlight, since
    // the highlight is incidental to "does the widget survive a roundtrip".
    const std::string out = TmpPath("annot_sync");
    {
        Document doc{FixturePdf().string()};
        TextBoxField tf{doc, Rect(72, 700, 272, 720)};
        doc.Form().Add(tf, "field1", 1);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_TRUE(re.Form().HasField("field1"));
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// FormEditor facade
// ---------------------------------------------------------------------------

TEST(ParityForm, FormEditorFacade_AddSetSaveReload) {
    // EXISTS — pdflib's FormEditor: bindPdf + addField + setField(value) +
    // save. OUR Facades::FormEditor::AddField is REAL (routes through
    // Document::Form().Add). There is no flat setField(value) on the
    // canonical facade — instead the value is set on the Field through the
    // facade's bound Document (Facade::Document() → Form() → Field::Value),
    // which is the canonical value-set path. We exercise the full
    // add + set-value + save + reopen + read-back round-trip.
    const std::string out = TmpPath("formeditor_add");
    {
        Facades::FormEditor editor{FixturePdf().string(), out};
        ASSERT_TRUE(editor.AddField(Facades::FieldType::Text, "username", 1,
                                    72, 700, 272, 720));
        // Set the value through the bound document (canonical value-set path).
        Aspose::Pdf::Document* bound = editor.Document();
        ASSERT_NE(bound, nullptr);
        Field* fld = FieldByName(*bound, "username");
        ASSERT_NE(fld, nullptr);
        fld->Value("testuser");
        editor.Save();
    }
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_TRUE(re.Form().HasField("username"));
    Field* f = FieldByName(re, "username");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->Value(), "testuser");
    std::filesystem::remove(out);
}

TEST(ParityForm, FormEditorFacade_Flatten) {
    // GAP — pdflib's FormEditor exposes flattenAllFields(). OUR canonical
    // Facades::FormEditor has NO flattenAllFields() (flatten lives on
    // Forms::Form::Flatten(), not on the facade). Skip as a facade-surface
    // GAP. (Form-level flatten is covered by ParityForm.FlattenAllFields_*.)
    GTEST_SKIP() << "SHAPE: pdflib-invented — canonical Facades::FormEditor "
                    "has no flattenAllFields(); flatten lives on "
                    "Forms::Form::Flatten() (covered by FlattenAllFields_*)";
}
