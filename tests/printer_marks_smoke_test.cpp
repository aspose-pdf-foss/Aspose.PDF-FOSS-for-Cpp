// =============================================================================
// printer_marks_smoke_test — PrinterMarkAnnotation::AddPrinterMarks on a
// 300 x 300 page whose trim box is inset by 36 pt: trim, bleed and
// registration marks, colour bars and page information are written as
// /PrinterMark annotations with appearances drawn in the margins.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/printer_mark_annotation.hpp>
#include <aspose/pdf/annotations/printer_marks_kind.hpp>
#include <aspose/pdf/annotations/trim_mark_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
using flatten_test::DarkPixels;
using flatten_test::ReadFile;
using flatten_test::TmpPath;

std::string InsetPage(const std::string& tag) {
    const auto path = TmpPath(tag);
    flatten_test::WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 300] /TrimBox [36 36 264 264] "
        "/BleedBox [30 30 270 270] >>",
    });
    return path;
}

std::size_t Count(const std::string& hay, const std::string& needle) {
    std::size_t n = 0;
    for (auto p = hay.find(needle); p != std::string::npos; p = hay.find(needle, p + 1)) ++n;
    return n;
}

}  // namespace

TEST(PrinterMarks, TrimMarkGeometry) {
    Document doc{InsetPage("marks_geom")};
    auto page = doc.Pages()[1];
    TrimMarkAnnotation tl{page, PrinterMarkCornerPosition::TopLeft};
    EXPECT_DOUBLE_EQ(tl.Rect().LLX(), 12.0);   // 36 - 24
    EXPECT_DOUBLE_EQ(tl.Rect().LLY(), 264.0);
    EXPECT_DOUBLE_EQ(tl.Rect().URX(), 36.0);
    EXPECT_DOUBLE_EQ(tl.Rect().URY(), 288.0);
}

TEST(PrinterMarks, AddAllMarks) {
    const auto out = TmpPath("marks_all");
    {
        Document doc{InsetPage("marks_src")};
        PrinterMarkAnnotation::AddPrinterMarks(doc, PrinterMarksKind::All);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    // 4 trim + 4 bleed + 4 registration + 4 colour bars + 1 page information.
    EXPECT_EQ(Count(bytes, "/Subtype /PrinterMark"), 17u);
    EXPECT_EQ(Count(bytes, "/MN /TrimMarks"), 4u);
    EXPECT_EQ(Count(bytes, "/MN /RegistrationTarget"), 4u);
    EXPECT_EQ(Count(bytes, "/MN /ColorBar"), 4u);
    EXPECT_EQ(Count(bytes, "/MN /PageInformation"), 1u);

    Document doc{out};
    const auto img = flatten_test::Render(doc, 1);
    EXPECT_GT(DarkPixels(img, 0, 264, 36, 300), 0);    // top-left corner marks
    EXPECT_GT(DarkPixels(img, 134, 268, 166, 300), 0);  // top registration target
    EXPECT_GT(DarkPixels(img, 0, 0, 300, 36), 0);       // page information text
    EXPECT_EQ(DarkPixels(img, 60, 60, 240, 240), 0);    // nothing inside the trim box
}

TEST(PrinterMarks, NoneAddsNothing) {
    Document doc{InsetPage("marks_none")};
    PrinterMarkAnnotation::AddPrinterMarks(doc, PrinterMarksKind::None);
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 0);
}
