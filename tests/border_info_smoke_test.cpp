// =============================================================================
// border_info_smoke_test — beat C-1 of the tables cluster. The styling value
// types GraphInfo, BorderInfo and the BorderSide [Flags] enum.
// =============================================================================

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/graph_info.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

}  // namespace

TEST(GraphInfoSmoke, Properties) {
    GraphInfo g;
    EXPECT_FLOAT_EQ(g.LineWidth(), 1.0f);
    EXPECT_DOUBLE_EQ(g.ScalingRateX(), 1.0);
    EXPECT_FALSE(g.IsDoubled());
    EXPECT_TRUE(g.DashArray().empty());

    g.LineWidth(2.5f);
    g.Color(Color::FromRgb(1.0, 0.0, 0.0));
    g.DashArray({3, 2});
    g.DashPhase(1);
    g.IsDoubled(true);
    g.RotationAngle(45.0);

    EXPECT_FLOAT_EQ(g.LineWidth(), 2.5f);
    EXPECT_DOUBLE_EQ(g.Color().A(), 1.0);
    ASSERT_EQ(g.DashArray().size(), 2u);
    EXPECT_EQ(g.DashArray()[0], 3);
    EXPECT_EQ(g.DashPhase(), 1);
    EXPECT_TRUE(g.IsDoubled());
    EXPECT_DOUBLE_EQ(g.RotationAngle(), 45.0);
}

TEST(BorderSideSmoke, FlagsCombine) {
    EXPECT_EQ(static_cast<int>(BorderSide::All), 15);
    EXPECT_EQ(static_cast<int>(BorderSide::Box), 15);
    const BorderSide lr = BorderSide::Left | BorderSide::Right;
    EXPECT_TRUE(HasSide(lr, BorderSide::Left));
    EXPECT_TRUE(HasSide(lr, BorderSide::Right));
    EXPECT_FALSE(HasSide(lr, BorderSide::Top));
}

TEST(BorderInfoSmoke, DefaultHasNoEdges) {
    BorderInfo b;
    EXPECT_FLOAT_EQ(b.Left().LineWidth(), 0.0f);
    EXPECT_FLOAT_EQ(b.Top().LineWidth(), 0.0f);
    EXPECT_DOUBLE_EQ(b.RoundedBorderRadius(), 0.0);
}

TEST(BorderInfoSmoke, ConstructorsConfigureNamedEdges) {
    // All edges, explicit width + colour.
    BorderInfo all{BorderSide::All, 2.0f, Color::FromRgb(0.0, 0.0, 1.0)};
    EXPECT_FLOAT_EQ(all.Left().LineWidth(), 2.0f);
    EXPECT_FLOAT_EQ(all.Bottom().LineWidth(), 2.0f);
    EXPECT_DOUBLE_EQ(all.Right().Color().A(), 1.0);

    // Only the named sides are configured; the rest stay at width 0.
    BorderInfo lr{BorderSide::Left | BorderSide::Right, 3.0f};
    EXPECT_FLOAT_EQ(lr.Left().LineWidth(), 3.0f);
    EXPECT_FLOAT_EQ(lr.Right().LineWidth(), 3.0f);
    EXPECT_FLOAT_EQ(lr.Top().LineWidth(), 0.0f);
    EXPECT_FLOAT_EQ(lr.Bottom().LineWidth(), 0.0f);

    // GraphInfo-valued constructor.
    GraphInfo g;
    g.LineWidth(5.0f);
    BorderInfo top{BorderSide::Top, g};
    EXPECT_FLOAT_EQ(top.Top().LineWidth(), 5.0f);
    EXPECT_FLOAT_EQ(top.Left().LineWidth(), 0.0f);

    top.RoundedBorderRadius(4.0);
    EXPECT_DOUBLE_EQ(top.RoundedBorderRadius(), 4.0);
}
