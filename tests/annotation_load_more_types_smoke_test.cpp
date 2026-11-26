// =============================================================================
// annotation_load_more_types_smoke_test — Page::Annotations() now reads Line,
// Ink, Polygon, PolyLine, FreeText and FileAttachment annotations from a
// file, with their geometry, and edits to them are saved.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/file_attachment_annotation.hpp>
#include <aspose/pdf/annotations/ink_annotation.hpp>
#include <aspose/pdf/annotations/line_annotation.hpp>
#include <aspose/pdf/annotations/polygon_annotation.hpp>
#include <aspose/pdf/annotations/polyline_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
using flatten_test::TmpPath;

std::string AnnotPdf(const std::string& tag) {
    const auto path = TmpPath(tag);
    flatten_test::WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Annots [4 0 R 5 0 R 6 0 R 7 0 R 8 0 R 9 0 R] >>",
        "<< /Type /Annot /Subtype /Line /Rect [10 10 110 60] /L [10 10 110 60] /Contents (line) >>",
        "<< /Type /Annot /Subtype /Ink /Rect [0 0 50 50] /InkList [[1 2 3 4] [5 6 7 8 9 10]] >>",
        "<< /Type /Annot /Subtype /Polygon /Rect [0 0 50 50] /Vertices [0 0 50 0 25 50] >>",
        "<< /Type /Annot /Subtype /PolyLine /Rect [0 0 50 50] /Vertices [0 0 50 50] >>",
        "<< /Type /Annot /Subtype /FreeText /Rect [100 100 200 130] /DA (/Helv 12 Tf 0 g) /Contents (free) >>",
        "<< /Type /Annot /Subtype /FileAttachment /Rect [300 300 320 320] "
        "/FS << /Type /Filespec /F (notes.txt) >> >>",
    });
    return path;
}

}  // namespace

TEST(AnnotationLoadMoreTypes, ReadsGeometry) {
    Document doc{AnnotPdf("load_more")};
    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 6);
    EXPECT_EQ(annots[0].AnnotationType(), AnnotationType::Line);
    EXPECT_EQ(annots[1].AnnotationType(), AnnotationType::Ink);
    EXPECT_EQ(annots[2].AnnotationType(), AnnotationType::Polygon);
    EXPECT_EQ(annots[3].AnnotationType(), AnnotationType::PolyLine);
    EXPECT_EQ(annots[4].AnnotationType(), AnnotationType::FreeText);
    EXPECT_EQ(annots[5].AnnotationType(), AnnotationType::FileAttachment);

    const auto& line = dynamic_cast<LineAnnotation&>(annots[0]);
    EXPECT_DOUBLE_EQ(line.Ending().X(), 110.0);
    EXPECT_EQ(line.Contents(), "line");
    const auto& ink = dynamic_cast<InkAnnotation&>(annots[1]);
    ASSERT_EQ(ink.InkList().size(), 2u);
    EXPECT_EQ(ink.InkList()[1].size(), 3u);
    EXPECT_EQ(dynamic_cast<PolygonAnnotation&>(annots[2]).Vertices().size(), 3u);
    EXPECT_EQ(dynamic_cast<FileAttachmentAnnotation&>(annots[5]).File().Name(), "notes.txt");
}

TEST(AnnotationLoadMoreTypes, EditsAreSavedLosslessly) {
    const auto out = TmpPath("load_more_out");
    {
        Document doc{AnnotPdf("load_more_edit")};
        doc.Pages()[1].Annotations()[0].Contents("edited line");
        doc.Save(out);
    }
    const std::string bytes = flatten_test::ReadFile(out);
    EXPECT_NE(bytes.find("(edited line)"), std::string::npos);
    EXPECT_NE(bytes.find("/L [10 10 110 60]"), std::string::npos);  // untouched entries stay
    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 6);
}
