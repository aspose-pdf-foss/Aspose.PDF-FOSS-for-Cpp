#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Rectangle — a rectangle shape (PDF vector graphics).
// Mirrors canonical Aspose.Pdf.Drawing.Rectangle (Aspose.PDF 26.4.0). Distinct
// from Aspose::Pdf::Rectangle (the page-geometry value type).
// =============================================================================

#include <aspose/pdf/drawing/shape.hpp>

namespace Aspose::Pdf::Drawing {

class Rectangle : public Shape {
public:
    Rectangle(float left, float bottom, float width, float height);

    // Canonical corner rounding.
    double RoundedCornerRadius() const noexcept;
    void RoundedCornerRadius(double value);

    // Canonical placement / size.
    double Left() const noexcept;
    void Left(double value);
    double Bottom() const noexcept;
    void Bottom(double value);
    double Width() const noexcept;
    void Width(double value);
    double Height() const noexcept;
    void Height(double value);

    bool CheckBounds(double containerWidth,
                     double containerHeight) const override;

private:
    double left_;
    double bottom_;
    double width_;
    double height_;
    double rounded_corner_radius_ = 0.0;
};

}  // namespace Aspose::Pdf::Drawing
