#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Ellipse — an ellipse shape (PDF vector graphics).
// Mirrors canonical Aspose.Pdf.Drawing.Ellipse (Aspose.PDF 26.4.0): a bounding
// box stroked / filled with the shape's GraphInfo.
// =============================================================================

#include <aspose/pdf/drawing/shape.hpp>

namespace Aspose::Pdf::Drawing {

class Ellipse : public Shape {
public:
    Ellipse(double left, double bottom, double width, double height);

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
};

}  // namespace Aspose::Pdf::Drawing
