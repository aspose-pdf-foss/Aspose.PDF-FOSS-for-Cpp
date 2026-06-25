// =============================================================================
// facades_form_editor_smoke_test — beat Fa9 of the Facades cluster.
// FormEditor edits a document's AcroForm. AddField / RemoveField are
// REAL (construct the concrete Field for the FieldType and route through
// Document::Form().Add / Delete, persisting via AcroForm save-through);
// the remaining field-configuration ops are v1 stubs.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/form_editor.hpp>
#include <aspose/pdf/facades/property_flag.hpp>
#include <aspose/pdf/facades/submit_form_flag.hpp>
#include <aspose/pdf/forms/form.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_formeditor_" + name + ".pdf")).string();
}

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), {});
}

}  // namespace

TEST(FacadesFormEditorSmoke, AddFieldRealViaForm) {
    Document doc{HelloWorldPdf()};
    FormEditor editor{doc};

    EXPECT_TRUE(editor.AddField(FieldType::Text, "name1", 1,
                                100, 600, 300, 620));
    EXPECT_TRUE(doc.Form().HasField("name1"));
    EXPECT_GE(doc.Form().Count(), 1);

    EXPECT_TRUE(editor.AddField(FieldType::CheckBox, "agree", 1,
                                100, 560, 120, 580));
    EXPECT_TRUE(doc.Form().HasField("agree"));
}

TEST(FacadesFormEditorSmoke, AddFieldInvalidTypeReturnsFalse) {
    Document doc{HelloWorldPdf()};
    FormEditor editor{doc};
    EXPECT_FALSE(editor.AddField(FieldType::InvalidNameOrType, "x", 1,
                                 0, 0, 10, 10));
}

TEST(FacadesFormEditorSmoke, RemoveFieldReal) {
    Document doc{HelloWorldPdf()};
    FormEditor editor{doc};
    editor.AddField(FieldType::Text, "keep", 1, 100, 600, 300, 620);
    editor.AddField(FieldType::Text, "drop", 1, 100, 560, 300, 580);
    ASSERT_TRUE(doc.Form().HasField("drop"));

    editor.RemoveField("drop");
    EXPECT_FALSE(doc.Form().HasField("drop"));
    EXPECT_TRUE(doc.Form().HasField("keep"));
}

TEST(FacadesFormEditorSmoke, FileBasedAddFieldSavePersists) {
    const std::string out = TempPath("persist");
    {
        FormEditor editor{HelloWorldPdf(), out};
        ASSERT_TRUE(editor.AddField(FieldType::Text, "myfield", 1,
                                    100, 600, 300, 620));
        editor.Save();
    }
    ASSERT_TRUE(std::filesystem::exists(out));
    EXPECT_NE(ReadFile(out).find("myfield"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(FacadesFormEditorSmoke, UnboundAddFieldReturnsFalse) {
    FormEditor editor;  // no doc, no src
    EXPECT_FALSE(editor.AddField(FieldType::Text, "x", 1, 0, 0, 10, 10));
    editor.RemoveField("x");  // safe no-op
}

TEST(FacadesFormEditorSmoke, ConfigurationStubs) {
    Document doc{HelloWorldPdf()};
    FormEditor editor{doc};
    editor.AddField(FieldType::Text, "f", 1, 100, 600, 300, 620);

    EXPECT_FALSE(editor.SetFieldAttribute("f", PropertyFlag::ReadOnly));
    EXPECT_FALSE(editor.SetFieldLimit("f", 10));
    EXPECT_FALSE(editor.SetFieldScript("f", "app.alert('x')"));
    EXPECT_FALSE(editor.Single2Multiple("f"));
    EXPECT_FALSE(editor.MoveField("f", 0, 0, 10, 10));

    // void stubs must not throw.
    editor.RenameField("f", "g");
    editor.DecorateField("f");
    editor.AddListItem("f", "item");
    editor.AddListItem("f", std::vector<std::string>{"a", "b"});
    editor.ResetFacade();
    SUCCEED();
}

TEST(FacadesFormEditorSmoke, PropertyRoundtrip) {
    FormEditor editor;
    editor.SrcFileName("in.pdf");
    editor.DestFileName("out.pdf");
    editor.RadioGap(2.5f);
    editor.RadioHoriz(true);
    editor.RadioButtonItemSize(15.0);
    editor.SubmitFlag(SubmitFormFlag::Pdf);
    editor.Items(std::vector<std::string>{"x", "y"});

    EXPECT_EQ(editor.SrcFileName(), "in.pdf");
    EXPECT_EQ(editor.DestFileName(), "out.pdf");
    EXPECT_FLOAT_EQ(editor.RadioGap(), 2.5f);
    EXPECT_TRUE(editor.RadioHoriz());
    EXPECT_DOUBLE_EQ(editor.RadioButtonItemSize(), 15.0);
    EXPECT_EQ(editor.SubmitFlag(), SubmitFormFlag::Pdf);
    ASSERT_EQ(editor.Items().size(), 2u);
    EXPECT_EQ(editor.Items()[0], "x");
}
