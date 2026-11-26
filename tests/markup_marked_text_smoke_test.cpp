// =============================================================================
// markup_marked_text_smoke_test — TextMarkupAnnotation::GetMarkedText on a
// page with "Hello brave world" (12 pt, x = 100, y = 700) and "second line"
// (y = 680): a new highlight with QuadPoints, one loaded from the file, a
// two-line selection, and an annotation not on a page.
// =============================================================================

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/text_markup_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
using flatten_test::TmpPath;

// The 0.5 em estimate: 6 pt per character at 12 pt.
std::vector<Point> Quad(double x0, double y0, double x1, double y1) {
    return {Point(x0, y1), Point(x1, y1), Point(x0, y0), Point(x1, y0)};
}

std::string TextPdf(const std::string& tag, const std::string& extra_annots = "") {
    const auto path = TmpPath(tag);
    const std::string content =
        "BT /F1 12 Tf 100 700 Td (Hello brave world) Tj ET\n"
        "BT /F1 12 Tf 100 680 Td (second line) Tj ET";
    flatten_test::WritePdf(
        path,
        {"<< /Type /Catalog /Pages 2 0 R >>",
         "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
         "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R "
         "/Resources << /Font << /F1 5 0 R >> >>" +
             (extra_annots.empty() ? std::string() : " /Annots [6 0 R]") + " >>",
         "<< /Length " + std::to_string(content.size()) + " >>",
         "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
         extra_annots.empty() ? std::string("null") : extra_annots},
        {{4, content}});
    return path;
}

}  // namespace

TEST(MarkupMarkedText, NewHighlightOverOneWord) {
    Document doc{TextPdf("marked_new")};
    auto page = doc.Pages()[1];
    HighlightAnnotation h{page, Rectangle(135, 698, 167, 712, false)};
    h.QuadPoints(Quad(135, 698, 167, 712));  // "brave": x 136..166
    page.Annotations().Add(h);
    EXPECT_EQ(h.GetMarkedText(), "brave");
}

TEST(MarkupMarkedText, TwoLines) {
    Document doc{TextPdf("marked_lines")};
    auto page = doc.Pages()[1];
    HighlightAnnotation h{page, Rectangle(99, 678, 203, 712, false)};
    std::vector<Point> quads = Quad(99, 698, 203, 712);   // whole first line
    const auto second = Quad(99, 678, 137, 692);           // "second"
    quads.insert(quads.end(), second.begin(), second.end());
    h.QuadPoints(quads);
    page.Annotations().Add(h);
    EXPECT_EQ(h.GetMarkedText(), "Hello brave world\nsecond");
}

TEST(MarkupMarkedText, LoadedHighlight) {
    Document doc{TextPdf("marked_loaded",
                         "<< /Type /Annot /Subtype /Highlight /Rect [99 698 131 712] "
                         "/QuadPoints [99 712 131 712 99 698 131 698] >>")};
    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 1);
    auto* tm = dynamic_cast<TextMarkupAnnotation*>(&annots[0]);
    ASSERT_NE(tm, nullptr);
    EXPECT_EQ(tm->QuadPoints().size(), 4u);
    EXPECT_EQ(tm->GetMarkedText(), "Hello");
}

TEST(MarkupMarkedText, NotOnAPageThrows) {
    Document doc{TextPdf("marked_unbound")};
    auto page = doc.Pages()[1];
    HighlightAnnotation h{page, Rectangle(0, 0, 10, 10, false)};
    EXPECT_THROW(h.GetMarkedText(), std::logic_error);
}
