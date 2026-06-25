#pragma once

// =============================================================================
// Aspose::Pdf::Rectangle — axis-aligned bounding box keyed on the
// PDF user-space convention (LLX, LLY) = lower-left corner, (URX,
// URY) = upper-right. Consumed by Annotation::Rect, Page::Rect,
// and most geometric APIs across the canonical surface. Mirrors
// canonical Aspose.Pdf.Rectangle (Aspose.PDF 26.4.0).
//
// Implements System.ICloneable via copy construction (the
// canonical Clone() returning System.Object is dropped per the
// cpp drops.yaml — value-type cloning is idiomatic in C++).
// =============================================================================

#include <vector>

#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rotation.hpp>

namespace Aspose::Pdf {

class Rectangle {
public:
    // Canonical ctor. When `normalizeCoordinates` is true, the
    // ctor swaps lly/ury and llx/urx so that LLX <= URX and
    // LLY <= URY regardless of caller convention.
    Rectangle(double llx, double lly, double urx, double ury,
              bool normalizeCoordinates) noexcept;

    // Equality + near-equality.
    bool Equals(const Rectangle& other) const noexcept;
    bool NearEquals(const Rectangle& other,
                    double delta) const noexcept;

    // Set algebra over rectangles.
    Rectangle Intersect(const Rectangle& otherRect) const noexcept;
    Rectangle Join(const Rectangle& otherRect) const noexcept;
    bool IsIntersect(const Rectangle& otherRect) const noexcept;

    // Point / line containment.
    bool Contains(const Point& point,
                  bool inclusive) const noexcept;
    bool ContainsPoint(double x, double y) const noexcept;
    bool ContainsLine(double x1, double y1,
                      double x2, double y2) const noexcept;

    // Geometric ops.
    Point Center() const noexcept;
    void Rotate(Rotation angle) noexcept;
    void Rotate(int angle) noexcept;
    std::vector<Point> ToPoints() const;
    void MoveBy(double dx, double dy) noexcept;

    // Read-only geometry (computed: URX - LLX, URY - LLY).
    double Width() const noexcept;
    double Height() const noexcept;

    // Lower-left / upper-right corner coordinates.
    double LLX() const noexcept;
    void LLX(double value) noexcept;

    double LLY() const noexcept;
    void LLY(double value) noexcept;

    double URX() const noexcept;
    void URX(double value) noexcept;

    double URY() const noexcept;
    void URY(double value) noexcept;

    // Special static instances.
    static Rectangle Empty();
    static Rectangle Trivial();

    // Predicates over the corners.
    bool IsTrivial() const noexcept;
    bool IsEmpty() const noexcept;
    bool IsPoint() const noexcept;

private:
    double llx_;
    double lly_;
    double urx_;
    double ury_;
};

}  // namespace Aspose::Pdf
