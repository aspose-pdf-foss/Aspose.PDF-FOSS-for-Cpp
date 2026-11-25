// =============================================================================
// annotation_flatten_smoke_test — Annotation::Flatten(), Field::Flatten() and
// PdfAnnotationEditor::FlatteningAnnotations(): the selected annotations'
// appearances are drawn into the page content and the annotations removed.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;

namespace {

void SaveWithSquare(const std::string& path) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    Annotations::SquareAnnotation sq{page, Rect(200, 200, 260, 260)};
    page.Annotations().Add(sq);
    doc.Save(path);
}

Forms::Field* FieldByName(Document& doc, const std::string& name) {
    for (Forms::Field* f : doc.Form().Fields())
        if (f != nullptr && f->PartialName() == name) return f;
    return nullptr;
}

// Two text fields on page 1: "a" at y 400-420, "b" at y 300-320.
void SaveTwoFields(const std::string& path) {
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField a{doc, Rect(100, 400, 300, 420)};
    a.Value("AAAA");
    Forms::TextBoxField b{doc, Rect(100, 300, 300, 320)};
    b.Value("BBBB");
    doc.Form().Add(a, "a", 1);
    doc.Form().Add(b, "b", 1);
    doc.Save(path);
}

// After flattening field "a" of SaveTwoFields' layout: one widget and one
// burned appearance on page 1, field "b" still interactive, "a" drawn.
void ExpectOnlyFieldAFlattened(const std::string& out) {
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Widget"), 1);
    EXPECT_EQ(FlattenedCount(p), 1);
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_NE(FieldByName(re, "b"), nullptr);
    EXPECT_EQ(FieldByName(re, "a"), nullptr);
    EXPECT_GT(DarkPixels(Render(re, 1), 100, 400, 300, 420), 20);
}

}  // namespace

TEST(AnnotationFlatten, NewSquareFlattenBurnsAndRemoves) {
    const std::string out = TmpPath("ann_new_square");
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        Annotations::SquareAnnotation sq{page, Rect(200, 200, 260, 260)};
        page.Annotations().Add(sq);
        sq.Flatten();
        EXPECT_EQ(page.Annotations().Count(), 0);
        doc.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Square"), 0);
    EXPECT_EQ(FlattenedCount(p), 1);
    Document re{out};
    EXPECT_GT(RedPixels(Render(re, 1), 200, 200, 260, 260), 20);
    std::filesystem::remove(out);
}

TEST(AnnotationFlatten, LoadedSquareFlattenBurns) {
    const std::string src = TmpPath("ann_loaded_src");
    const std::string out = TmpPath("ann_loaded_out");
    SaveWithSquare(src);
    const std::uint32_t ap_id = FirstApId(ParsePage(ReadFile(src), 0), "Square");
    ASSERT_NE(ap_id, 0u);
    {
        Document re{src};
        auto page = re.Pages()[1];
        ASSERT_EQ(page.Annotations().Count(), 1);
        page.Annotations()[0].Flatten();
        EXPECT_EQ(page.Annotations().Count(), 0);
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Square"), 0);
    const auto xo = XObjects(p);
    ASSERT_EQ(xo.count("AspFl1"), 1u);
    EXPECT_EQ(xo.at("AspFl1"), ap_id);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(AnnotationFlatten, LoadedLinkFlattenRemoves) {
    const std::string src = TmpPath("ann_link_src");
    const std::string out = TmpPath("ann_link_out");
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        Annotations::LinkAnnotation link{page, Rect(100, 100, 200, 120)};
        page.Annotations().Add(link);
        doc.Save(src);
    }
    ASSERT_EQ(CountSubtype(ParsePage(ReadFile(src), 0), "Link"), 1);
    {
        Document re{src};
        auto page = re.Pages()[1];
        ASSERT_EQ(page.Annotations().Count(), 1);
        page.Annotations()[0].Flatten();
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Link"), 0);
    EXPECT_EQ(FlattenedCount(p), 0);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(AnnotationFlatten, FlattenRemovesItsPopup) {
    const std::string src = TmpPath("ann_popup_src");
    const std::string out = TmpPath("ann_popup_out");
    WritePdf(src,
             {"<< /Type /Catalog /Pages 2 0 R >>",
              "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
              "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
              "/Annots [4 0 R 5 0 R] >>",
              "<< /Type /Annot /Subtype /Square /Rect [200 200 240 240] "
              "/AP << /N 6 0 R >> /Popup 5 0 R >>",
              "<< /Type /Annot /Subtype /Popup /Parent 4 0 R "
              "/Rect [300 300 400 400] >>",
              "<< /Type /XObject /Subtype /Form /BBox [0 0 40 40] "
              "/Length 24 >>"},
             {{6, "1 0 0 RG 0 0 40 40 re S"}});
    {
        Document re{src};
        auto page = re.Pages()[1];
        ASSERT_EQ(page.Annotations().Count(), 1);
        page.Annotations()[0].Flatten();
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Square"), 0);
    EXPECT_EQ(CountSubtype(p, "Popup"), 0);
    const auto xo = XObjects(p);
    ASSERT_EQ(xo.count("AspFl1"), 1u);
    EXPECT_EQ(xo.at("AspFl1"), 6u);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(AnnotationFlatten, FlattenUnownedAnnotationIsNoop) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    Annotations::SquareAnnotation sq{page, Rect(200, 200, 260, 260)};
    EXPECT_NO_THROW(sq.Flatten());
    EXPECT_EQ(page.Annotations().Count(), 0);
}

TEST(FieldFlatten, NewFieldFlattenBurnsOnlyThatField) {
    const std::string out = TmpPath("field_new");
    {
        Document doc{HelloWorldPdf()};
        Forms::TextBoxField a{doc, Rect(100, 400, 300, 420)};
        a.Value("AAAA");
        Forms::TextBoxField b{doc, Rect(100, 300, 300, 320)};
        b.Value("BBBB");
        doc.Form().Add(a, "a", 1);
        doc.Form().Add(b, "b", 1);
        a.Flatten();
        EXPECT_EQ(doc.Form().Count(), 1);
        doc.Save(out);
    }
    ExpectOnlyFieldAFlattened(out);
    std::filesystem::remove(out);
}

TEST(FieldFlatten, LoadedFieldFlatten) {
    const std::string src = TmpPath("field_loaded_src");
    const std::string out = TmpPath("field_loaded_out");
    SaveTwoFields(src);
    {
        Document re{src};
        Forms::Field* a = FieldByName(re, "a");
        ASSERT_NE(a, nullptr);
        a->Flatten();
        EXPECT_EQ(re.Form().Count(), 1);
        re.Save(out);
    }
    ExpectOnlyFieldAFlattened(out);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FieldFlatten, RadioFieldFlattenBurnsAllKids) {
    const std::string out = TmpPath("field_radio");
    {
        Document doc{HelloWorldPdf()};
        Forms::RadioButtonField rb{doc};
        rb.AddOption("x", Rect(100, 300, 120, 320));
        rb.AddOption("y", Rect(200, 300, 220, 320));
        rb.Value("y");
        Forms::TextBoxField t{doc, Rect(100, 400, 300, 420)};
        t.Value("Keep");
        doc.Form().Add(rb, "color", 1);
        doc.Form().Add(t, "t", 1);
        rb.Flatten();
        doc.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(FlattenedCount(p), 2);
    EXPECT_EQ(CountSubtype(p, "Widget"), 1);
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_NE(FieldByName(re, "t"), nullptr);
    std::filesystem::remove(out);
}
