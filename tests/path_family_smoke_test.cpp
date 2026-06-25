// =============================================================================
// path_family_smoke_test — beat A9 of the annotations cluster.
// PolygonAnnotation + PolylineAnnotation extend the new
// PolyAnnotation intermediate base (vertices + interior color +
// line-end styles + intent).
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/poly_annotation.hpp>
#include <aspose/pdf/annotations/poly_intent.hpp>
#include <aspose/pdf/annotations/polygon_annotation.hpp>
#include <aspose/pdf/annotations/polyline_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/point.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int polygon = 0, polyline = 0;
    void Visit(PolygonAnnotation& p)  override { ++polygon;  (void)p; }
    void Visit(PolylineAnnotation& p) override { ++polyline; (void)p; }
};

}  // namespace

TEST(PathFamilySmoke, PolyIntentEnumValues) {
    EXPECT_EQ(static_cast<int>(PolyIntent::Undefined),         0);
    EXPECT_EQ(static_cast<int>(PolyIntent::PolygonCloud),      1);
    EXPECT_EQ(static_cast<int>(PolyIntent::PolyLineDimension), 2);
    EXPECT_EQ(static_cast<int>(PolyIntent::PolygonDimension),  3);
}

TEST(PathFamilySmoke, PolygonConstructAndDispatch) {
    Document doc;
    std::vector<Point> verts{
        Point{0, 0}, Point{10, 0}, Point{10, 10}, Point{0, 10}};

    PolygonAnnotation p{doc, verts};
    EXPECT_EQ(p.AnnotationType(), AnnotationType::Polygon);
    EXPECT_EQ(p.Vertices().size(), 4u);

    CountingSelector s;
    p.Accept(s);
    EXPECT_EQ(s.polygon, 1);
    EXPECT_EQ(s.polyline, 0);
}

TEST(PathFamilySmoke, PolyAnnotationAccessorsInherited) {
    Document doc;
    std::vector<Point> verts{Point{0, 0}, Point{1, 1}, Point{2, 0}};
    PolygonAnnotation p{doc, verts};

    p.InteriorColor(Color::Blue());
    p.StartingStyle(LineEnding::OpenArrow);
    p.EndingStyle(LineEnding::ClosedArrow);
    p.Intent(PolyIntent::PolygonCloud);

    EXPECT_NEAR(p.InteriorColor().A(), 1.0, 1e-9);
    EXPECT_EQ(p.StartingStyle(), LineEnding::OpenArrow);
    EXPECT_EQ(p.EndingStyle(),   LineEnding::ClosedArrow);
    EXPECT_EQ(p.Intent(),        PolyIntent::PolygonCloud);
}

TEST(PathFamilySmoke, VerticesRoundtrip) {
    Document doc;
    std::vector<Point> initial{Point{0, 0}, Point{1, 1}};
    PolygonAnnotation p{doc, initial};

    std::vector<Point> updated{
        Point{5, 5}, Point{6, 7}, Point{8, 4}, Point{9, 2}};
    p.Vertices(updated);
    EXPECT_EQ(p.Vertices().size(), 4u);
    EXPECT_NEAR(p.Vertices()[2].X(), 8.0, 1e-9);
}
