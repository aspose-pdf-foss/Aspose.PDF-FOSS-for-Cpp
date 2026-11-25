// =============================================================================
// forms_flatten_smoke_test — Form::Flatten() + Save() burn every visible
// widget's appearance into its page content (no values lost), and value-edited
// loaded fields get a fresh /AP on a plain Save.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Forms::CheckboxField;
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
            ("asp_form_flatten_" + tag + ".pdf"))
        .string();
}

Rectangle Rect(double a, double b, double c, double d) {
    return Rectangle(a, b, c, d, false);
}

std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

void WriteFile(const std::string& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::string ReplaceAll(std::string s, const std::string& from,
                       const std::string& to) {
    for (std::size_t pos = 0;
         (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
        s.replace(pos, from.size(), to);
    return s;
}

Field* FieldByName(Document& doc, const std::string& name) {
    for (Field* f : doc.Form().Fields()) {
        if (f != nullptr && f->PartialName() == name) return f;
    }
    return nullptr;
}

}  // namespace

TEST(FormsFlatten, FlattenEmptiesFormForUser) {
    Document doc{HelloWorldPdf()};
    TextBoxField tb{doc, Rect(100, 700, 300, 720)};
    tb.Value("Hello");
    doc.Form().Add(tb, 1);
    ASSERT_EQ(doc.Form().Count(), 1);
    doc.Form().Flatten();
    EXPECT_EQ(doc.Form().Count(), 0);
}

TEST(FormsFlatten, FlattenedNewFieldIsWrittenBeforeRemoval) {
    const std::string out = TmpPath("newfield");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 700, 300, 720)};
        tb.Value("Hello");
        doc.Form().Add(tb, 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    EXPECT_TRUE(Has(ReadFile(out), "(Hello) Tj"));
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 0);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, EditedLoadedTextFieldGetsNewAppearance) {
    const std::string out = TmpPath("edit1");
    const std::string out2 = TmpPath("edit2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 700, 300, 720)};
        tb.Value("Old");
        doc.Form().Add(tb, "name", 1);
        doc.Save(out);
    }
    {
        Document re{out};
        Field* f = FieldByName(re, "name");
        ASSERT_NE(f, nullptr);
        f->Value("New");
        re.Save(out2);
    }
    EXPECT_TRUE(Has(ReadFile(out2), "(New) Tj"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsFlatten, EditedLoadedCheckboxKeepsItsOnStateName) {
    const std::string out = TmpPath("chk1");
    const std::string out2 = TmpPath("chk2");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField cb{doc, Rect(100, 560, 120, 580)};
        doc.Form().Add(cb, "agree", 1);
        doc.Save(out);
    }
    // Rename the on-state from /Yes to /Chk (same length keeps the xref
    // offsets valid), as a file authored by another tool might.
    const std::string original = ReadFile(out);
    ASSERT_TRUE(Has(original, "/Yes"));
    WriteFile(out, ReplaceAll(original, "/Yes", "/Chk"));
    {
        Document re{out};
        auto* cb = dynamic_cast<CheckboxField*>(FieldByName(re, "agree"));
        ASSERT_NE(cb, nullptr);
        cb->Checked(true);
        re.Save(out2);
    }
    const std::string saved = ReadFile(out2);
    EXPECT_TRUE(Has(saved, "/AS /Chk"));
    EXPECT_FALSE(Has(saved, "/AS /Yes"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}
