// =============================================================================
// rectangle_smoke_test — beat A0 of the annotations cluster.
// Covers Rectangle (geometry primitives + set ops), Point, and
// the Rotation enum.
// =============================================================================

#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

#include <gtest/gtest.h>

#include <cmath>

using Aspose::Pdf::Point;
using Aspose::Pdf::Rectangle;
using Aspose::Pdf::Rotation;

constexpr double kEps = 1e-9;

TEST(RectangleSmoke, CtorWithoutNormalisation) {
    Rectangle r{10.0, 20.0, 30.0, 40.0, /*normalize=*/false};
    EXPECT_NEAR(r.LLX(), 10.0, kEps);
    EXPECT_NEAR(r.LLY(), 20.0, kEps);
    EXPECT_NEAR(r.URX(), 30.0, kEps);
    EXPECT_NEAR(r.URY(), 40.0, kEps);
    EXPECT_NEAR(r.Width(),  20.0, kEps);
    EXPECT_NEAR(r.Height(), 20.0, kEps);
}

TEST(RectangleSmoke, CtorWithNormalisationSwaps) {
    // urx < llx + ury < lly — normalise must swap both axes.
    Rectangle r{30.0, 40.0, 10.0, 20.0, /*normalize=*/true};
    EXPECT_NEAR(r.LLX(), 10.0, kEps);
    EXPECT_NEAR(r.LLY(), 20.0, kEps);
    EXPECT_NEAR(r.URX(), 30.0, kEps);
    EXPECT_NEAR(r.URY(), 40.0, kEps);
}

TEST(RectangleSmoke, EqualsAndNearEquals) {
    Rectangle a{1.0, 2.0, 3.0, 4.0, false};
    Rectangle b{1.0, 2.0, 3.0, 4.0, false};
    Rectangle c{1.001, 2.0, 3.0, 4.0, false};
    EXPECT_TRUE(a.Equals(b));
    EXPECT_FALSE(a.Equals(c));
    EXPECT_TRUE(a.NearEquals(c, 0.01));
    EXPECT_FALSE(a.NearEquals(c, 0.0001));
}

TEST(RectangleSmoke, IntersectAndJoin) {
    Rectangle a{0.0, 0.0, 10.0, 10.0, false};
    Rectangle b{5.0, 5.0, 15.0, 15.0, false};
    auto i = a.Intersect(b);
    EXPECT_NEAR(i.LLX(), 5.0,  kEps);
    EXPECT_NEAR(i.LLY(), 5.0,  kEps);
    EXPECT_NEAR(i.URX(), 10.0, kEps);
    EXPECT_NEAR(i.URY(), 10.0, kEps);

    auto j = a.Join(b);
    EXPECT_NEAR(j.LLX(), 0.0,  kEps);
    EXPECT_NEAR(j.LLY(), 0.0,  kEps);
    EXPECT_NEAR(j.URX(), 15.0, kEps);
    EXPECT_NEAR(j.URY(), 15.0, kEps);

    EXPECT_TRUE(a.IsIntersect(b));
    Rectangle far{100.0, 100.0, 200.0, 200.0, false};
    EXPECT_FALSE(a.IsIntersect(far));
}

TEST(RectangleSmoke, Contains) {
    Rectangle r{0.0, 0.0, 10.0, 10.0, false};

    Point inside{5.0, 5.0};
    EXPECT_TRUE(r.Contains(inside, true));
    EXPECT_TRUE(r.Contains(inside, false));

    Point onEdge{0.0, 5.0};
    EXPECT_TRUE(r.Contains(onEdge, true));     // inclusive
    EXPECT_FALSE(r.Contains(onEdge, false));   // exclusive

    EXPECT_TRUE(r.ContainsPoint(5.0, 5.0));
    EXPECT_TRUE(r.ContainsLine(2.0, 2.0, 8.0, 8.0));
    EXPECT_FALSE(r.ContainsLine(2.0, 2.0, 20.0, 8.0));
}

TEST(RectangleSmoke, CenterAndToPoints) {
    Rectangle r{0.0, 0.0, 10.0, 20.0, false};
    auto c = r.Center();
    EXPECT_NEAR(c.X(), 5.0,  kEps);
    EXPECT_NEAR(c.Y(), 10.0, kEps);

    auto pts = r.ToPoints();
    ASSERT_EQ(pts.size(), 4u);
    EXPECT_NEAR(pts[0].X(),  0.0, kEps);
    EXPECT_NEAR(pts[0].Y(),  0.0, kEps);
    EXPECT_NEAR(pts[2].X(), 10.0, kEps);
    EXPECT_NEAR(pts[2].Y(), 20.0, kEps);
}

TEST(RectangleSmoke, RotateAndMoveBy) {
    Rectangle r{0.0, 0.0, 10.0, 20.0, false};
    r.Rotate(Rotation::on90);
    // 90° rotation swaps width/height around the center (5, 10).
    EXPECT_NEAR(r.Width(),  20.0, kEps);
    EXPECT_NEAR(r.Height(), 10.0, kEps);

    Rectangle q{0.0, 0.0, 1.0, 1.0, false};
    q.MoveBy(3.0, 4.0);
    EXPECT_NEAR(q.LLX(), 3.0, kEps);
    EXPECT_NEAR(q.LLY(), 4.0, kEps);
    EXPECT_NEAR(q.URX(), 4.0, kEps);
    EXPECT_NEAR(q.URY(), 5.0, kEps);
}

TEST(RectangleSmoke, EmptyAndTrivialAndPoint) {
    auto empty = Rectangle::Empty();
    EXPECT_TRUE(empty.IsEmpty());

    auto triv = Rectangle::Trivial();
    EXPECT_TRUE(triv.IsTrivial());
    EXPECT_TRUE(triv.IsPoint());   // origin-anchored zero-area

    Rectangle pt{5.0, 5.0, 5.0, 5.0, false};
    EXPECT_TRUE(pt.IsPoint());
    EXPECT_FALSE(pt.IsTrivial());
}

TEST(PointSmoke, CtorAndAccessors) {
    Point p{3.0, 4.0};
    EXPECT_NEAR(p.X(), 3.0, kEps);
    EXPECT_NEAR(p.Y(), 4.0, kEps);

    p.X(7.0);
    p.Y(9.0);
    EXPECT_NEAR(p.X(), 7.0, kEps);
    EXPECT_NEAR(p.Y(), 9.0, kEps);
}

TEST(PointSmoke, Distance) {
    Point a{0.0, 0.0};
    Point b{3.0, 4.0};
    EXPECT_NEAR(Point::Distance(a, b), 5.0, kEps);
}

TEST(PointSmoke, Trivial) {
    auto t = Point::Trivial();
    EXPECT_NEAR(t.X(), 0.0, kEps);
    EXPECT_NEAR(t.Y(), 0.0, kEps);
}

TEST(RotationEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(Rotation::None),  0);
    EXPECT_EQ(static_cast<int>(Rotation::on90),  1);
    EXPECT_EQ(static_cast<int>(Rotation::on180), 2);
    EXPECT_EQ(static_cast<int>(Rotation::on270), 3);
}
