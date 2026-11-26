// =============================================================================
// forms_section3_smoke_test — Form::GetFieldsInRect,
// Form::MakeFormAnnotationsIndependent, CheckboxField::AddOption / Clone.
// =============================================================================

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;
using flatten_test::ReadFile;
using flatten_test::TmpPath;
using flatten_test::WritePdf;

std::vector<std::string> Names(Document& doc) {
    std::vector<std::string> out;
    for (Field* f : doc.Form().Fields()) out.push_back(f->PartialName());
    std::sort(out.begin(), out.end());
    return out;
}

// Two pages; text fields "a" (page 1, 50..150 x 700..720) and "b" (page 1,
// 300..400 x 100..120); "shared" has one widget on each page.
std::string FormPdf(const std::string& tag) {
    const auto path = TmpPath(tag);
    WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R /AcroForm << /Fields [5 0 R 6 0 R 7 0 R] >> >>",
        "<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Annots [5 0 R 6 0 R 8 0 R] >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Annots [9 0 R] >>",
        "<< /FT /Tx /T (a) /Type /Annot /Subtype /Widget /Rect [50 700 150 720] /P 3 0 R >>",
        "<< /FT /Tx /T (b) /Type /Annot /Subtype /Widget /Rect [300 100 400 120] /P 3 0 R >>",
        "<< /FT /Tx /T (shared) /V (same) /Kids [8 0 R 9 0 R] >>",
        "<< /Type /Annot /Subtype /Widget /Parent 7 0 R /Rect [50 500 150 520] /P 3 0 R >>",
        "<< /Type /Annot /Subtype /Widget /Parent 7 0 R /Rect [50 500 150 520] /P 4 0 R >>",
    });
    return path;
}

}  // namespace

TEST(FormsSection3, GetFieldsInRect) {
    Document doc{FormPdf("gfir")};
    auto inside = doc.Form().GetFieldsInRect(Rectangle(0, 650, 200, 750, false));
    ASSERT_EQ(inside.size(), 1u);
    EXPECT_EQ(inside[0]->PartialName(), "a");
    EXPECT_TRUE(doc.Form().GetFieldsInRect(Rectangle(60, 705, 100, 715, false)).empty());
    EXPECT_GE(doc.Form().GetFieldsInRect(Rectangle(0, 0, 612, 792, false)).size(), 2u);
}

TEST(FormsSection3, MakeFormAnnotationsIndependent) {
    const auto out = TmpPath("independent_out");
    {
        Document doc{FormPdf("independent")};
        auto page2 = doc.Pages()[2];
        doc.Form().MakeFormAnnotationsIndependent(page2);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(Names(doc), (std::vector<std::string>{"a", "b", "shared", "shared0"}));
    // Each copy keeps the value and has exactly one widget.
    const std::string bytes = ReadFile(out);
    EXPECT_NE(bytes.find("(shared0)"), std::string::npos);
}

TEST(FormsSection3, CheckboxOptionsOnNewField) {
    const auto out = TmpPath("checkbox_options");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        CheckboxField c{doc, Rectangle(100, 600, 115, 615, false)};
        c.PartialName("choice");
        c.ExportValue("Yes");
        c.AddOption("Maybe");
        c.AddOption("No", 1, Rectangle(200, 600, 215, 615, false));
        const auto states = c.AllowedStates();
        EXPECT_NE(std::find(states.begin(), states.end(), "Maybe"), states.end());
        c.Value("Maybe");
        doc.Form().Add(c, 1);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_NE(bytes.find("/V /Maybe"), std::string::npos);
    EXPECT_NE(bytes.find("/AS /Maybe"), std::string::npos);
    EXPECT_NE(bytes.find("/AS /Off"), std::string::npos);
    const auto page = flatten_test::ParsePage(bytes, 0);
    EXPECT_EQ(flatten_test::CountSubtype(page, "Widget"), 3);
}

TEST(FormsSection3, CheckboxOptionsOnLoadedFieldThrow) {
    const auto path = TmpPath("checkbox_loaded");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        CheckboxField c{doc, Rectangle(100, 600, 115, 615, false)};
        c.PartialName("cb");
        doc.Form().Add(c, 1);
        doc.Save(path);
    }
    Document doc{path};
    for (Field* f : doc.Form().Fields())
        if (auto* cb = dynamic_cast<CheckboxField*>(f))
            EXPECT_THROW(cb->AddOption("Extra"), std::logic_error);
}

TEST(FormsSection3, CheckboxClone) {
    Document doc;
    CheckboxField c{doc, Rectangle(10, 10, 20, 20, false)};
    c.PartialName("orig");
    c.ExportValue("On");
    c.Checked(true);
    auto* copy = static_cast<CheckboxField*>(c.Clone());
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->PartialName(), "orig");
    EXPECT_EQ(copy->ExportValue(), "On");
    EXPECT_TRUE(copy->Checked());
    EXPECT_DOUBLE_EQ(copy->Rect().URX(), 20.0);
    delete copy;
}
