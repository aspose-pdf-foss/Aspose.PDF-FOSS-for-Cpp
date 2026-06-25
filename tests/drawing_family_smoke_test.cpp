// =============================================================================
// drawing_family_smoke_test — beat A8 of the annotations cluster.
// CircleAnnotation + SquareAnnotation (both extend
// CommonFigureAnnotation) + LineAnnotation (extends MarkupAnnotation
// directly). Exercises visitor dispatch + per-class properties.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/caption_position.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/common_figure_annotation.hpp>
#include <aspose/pdf/annotations/line_annotation.hpp>
#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/line_intent.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int circle = 0, square = 0, line = 0;
    void Visit(CircleAnnotation& c) override { ++circle; (void)c; }
    void Visit(SquareAnnotation& s) override { ++square; (void)s; }
    void Visit(LineAnnotation& l) override   { ++line;   (void)l; }
};

}  // namespace

TEST(DrawingFamilySmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(CaptionPosition::Inline),   0);
    EXPECT_EQ(static_cast<int>(CaptionPosition::Top),      1);
    EXPECT_EQ(static_cast<int>(LineIntent::Undefined),     0);
    EXPECT_EQ(static_cast<int>(LineIntent::LineArrow),     1);
    EXPECT_EQ(static_cast<int>(LineIntent::LineDimension), 2);
}

TEST(DrawingFamilySmoke, CircleConstructAndVisit) {
    Document doc;
    CircleAnnotation c{doc};
    EXPECT_EQ(c.AnnotationType(), AnnotationType::Circle);

    CountingSelector s;
    c.Accept(s);
    EXPECT_EQ(s.circle, 1);
    EXPECT_EQ(s.square, 0);
}

TEST(DrawingFamilySmoke, SquareConstructAndVisit) {
    Document doc;
    SquareAnnotation a{doc};
    EXPECT_EQ(a.AnnotationType(), AnnotationType::Square);

    CountingSelector s;
    a.Accept(s);
    EXPECT_EQ(s.square, 1);
    EXPECT_EQ(s.circle, 0);
}

TEST(DrawingFamilySmoke, CommonFigureAccessors) {
    Document doc;
    CircleAnnotation c{doc};
    c.InteriorColor(Color::Red());
    EXPECT_NEAR(c.InteriorColor().A(), 1.0, 1e-9);

    Rectangle r{1.0, 2.0, 10.0, 20.0, false};
    c.Frame(r);
    EXPECT_NEAR(c.Frame().LLX(), 1.0,  1e-9);
    EXPECT_NEAR(c.Frame().URY(), 20.0, 1e-9);
}

TEST(DrawingFamilySmoke, LineConstructAndAccessors) {
    Document doc;
    Point start{0.0, 0.0};
    Point end{100.0, 50.0};
    LineAnnotation l{doc, start, end};

    EXPECT_EQ(l.AnnotationType(), AnnotationType::Line);
    EXPECT_NEAR(l.Starting().X(), 0.0,   1e-9);
    EXPECT_NEAR(l.Ending().X(),   100.0, 1e-9);
    EXPECT_NEAR(l.Ending().Y(),   50.0,  1e-9);

    EXPECT_EQ(l.StartingStyle(), LineEnding::None);
    EXPECT_EQ(l.EndingStyle(),   LineEnding::None);
    EXPECT_FALSE(l.ShowCaption());
    EXPECT_EQ(l.CaptionPosition(), CaptionPosition::Inline);
    EXPECT_EQ(l.Intent(),          LineIntent::Undefined);

    l.StartingStyle(LineEnding::Diamond);
    l.EndingStyle(LineEnding::OpenArrow);
    l.LeaderLine(5.0);
    l.LeaderLineExtension(3.0);
    l.LeaderLineOffset(1.0);
    l.ShowCaption(true);
    l.CaptionOffset(Point{2.0, 4.0});
    l.CaptionPosition(CaptionPosition::Top);
    l.Intent(LineIntent::LineArrow);

    EXPECT_EQ(l.StartingStyle(),     LineEnding::Diamond);
    EXPECT_EQ(l.EndingStyle(),       LineEnding::OpenArrow);
    EXPECT_DOUBLE_EQ(l.LeaderLine(), 5.0);
    EXPECT_DOUBLE_EQ(l.LeaderLineExtension(), 3.0);
    EXPECT_DOUBLE_EQ(l.LeaderLineOffset(),    1.0);
    EXPECT_TRUE(l.ShowCaption());
    EXPECT_NEAR(l.CaptionOffset().Y(), 4.0, 1e-9);
    EXPECT_EQ(l.CaptionPosition(), CaptionPosition::Top);
    EXPECT_EQ(l.Intent(),          LineIntent::LineArrow);
}

TEST(DrawingFamilySmoke, LineDispatchesToLineVisit) {
    Document doc;
    LineAnnotation l{doc, Point{0,0}, Point{1,1}};
    CountingSelector s;
    l.Accept(s);
    EXPECT_EQ(s.line, 1);
}
