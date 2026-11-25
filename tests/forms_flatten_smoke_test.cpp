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
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
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
