// =============================================================================
// drawing_smoke_test — beat D-1 of the drawing cluster. The Aspose::Pdf::Drawing
// shape family: Shape (base), Graph (container), Line, Rectangle, Circle,
// Ellipse. Covers construction, accessors and CheckBounds. (Render-on-save is
// D-2.)
// =============================================================================

#include <memory>
#include <vector>

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/drawing/shape.hpp>
#include <aspose/pdf/graph_info.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Drawing;

}  // namespace

TEST(DrawingShapesSmoke, ConstructAndBounds) {
    Line line{std::vector<float>{0.0f, 0.0f, 50.0f, 50.0f}};
    ASSERT_EQ(line.PositionArray().size(), 4u);
    EXPECT_FLOAT_EQ(line.PositionArray()[2], 50.0f);
    EXPECT_TRUE(line.CheckBounds(100.0, 100.0));
    EXPECT_FALSE(line.CheckBounds(10.0, 10.0));  // point (50,50) out of bounds

    Rectangle rect{5.0f, 5.0f, 40.0f, 30.0f};
    EXPECT_DOUBLE_EQ(rect.Width(), 40.0);
    rect.RoundedCornerRadius(3.0);
    EXPECT_DOUBLE_EQ(rect.RoundedCornerRadius(), 3.0);
    EXPECT_TRUE(rect.CheckBounds(100.0, 100.0));

    Circle circle{50.0f, 50.0f, 20.0f};
    EXPECT_DOUBLE_EQ(circle.Radius(), 20.0);
    EXPECT_TRUE(circle.CheckBounds(100.0, 100.0));
    EXPECT_FALSE(circle.CheckBounds(60.0, 60.0));  // 50+20 > 60

    Ellipse ell{10.0, 10.0, 30.0, 20.0};
    EXPECT_DOUBLE_EQ(ell.Height(), 20.0);
}

TEST(DrawingGraphSmoke, OwnsShapesPolymorphically) {
    Graph graph{200.0, 150.0};
    EXPECT_DOUBLE_EQ(graph.Width(), 200.0);

    graph.Shapes().push_back(
        std::make_unique<Line>(std::vector<float>{0.0f, 0.0f, 10.0f, 10.0f}));
    graph.Shapes().push_back(std::make_unique<Rectangle>(0.0f, 0.0f, 50.0f,
                                                         50.0f));
    graph.Shapes().push_back(std::make_unique<Circle>(25.0f, 25.0f, 10.0f));
    ASSERT_EQ(graph.Shapes().size(), 3u);

    // Polymorphic access.
    EXPECT_NE(dynamic_cast<Rectangle*>(graph.Shapes()[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<Circle*>(graph.Shapes()[2].get()), nullptr);

    // Graph is a BaseParagraph.
    BaseParagraph& bp = graph;
    bp.ZIndex(5);
    EXPECT_EQ(bp.ZIndex(), 5);

    // Shape GraphInfo styling.
    GraphInfo gi;
    gi.LineWidth(2.0f);
    graph.Shapes()[0]->GraphInfo(gi);
    EXPECT_FLOAT_EQ(graph.Shapes()[0]->GraphInfo().LineWidth(), 2.0f);
}
