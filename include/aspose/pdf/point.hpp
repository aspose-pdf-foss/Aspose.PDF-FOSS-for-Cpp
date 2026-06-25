#pragma once

// =============================================================================
// Aspose::Pdf::Point — 2D coordinate primitive consumed by
// Rectangle::Contains / Center / ToPoints + annotation quad-point
// arrays. Mirrors canonical Aspose.Pdf.Point (Aspose.PDF 26.4.0).
// =============================================================================

namespace Aspose::Pdf {

class Point {
public:
    Point(double x, double y) noexcept;

    double X() const noexcept;
    void X(double value) noexcept;

    double Y() const noexcept;
    void Y(double value) noexcept;

    // Euclidean distance between two Point values.
    static double Distance(const Point& point1,
                           const Point& point2) noexcept;

    // Canonical "trivial" Point — the origin (0, 0).
    static Point Trivial();

private:
    double x_;
    double y_;
};

}  // namespace Aspose::Pdf
