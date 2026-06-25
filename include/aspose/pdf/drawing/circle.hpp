#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Circle — a circle shape (PDF vector graphics). Mirrors
// canonical Aspose.Pdf.Drawing.Circle (Aspose.PDF 26.4.0): a centre point and
// radius stroked / filled with the shape's GraphInfo.
// =============================================================================

#include <aspose/pdf/drawing/shape.hpp>

namespace Aspose::Pdf::Drawing {

class Circle : public Shape {
public:
    Circle(float posX, float posY, float radius);

    double PosX() const noexcept;
    void PosX(double value);
    double PosY() const noexcept;
    void PosY(double value);
    double Radius() const noexcept;
    void Radius(double value);

    bool CheckBounds(double containerWidth,
                     double containerHeight) const override;

private:
    double pos_x_;
    double pos_y_;
    double radius_;
};

}  // namespace Aspose::Pdf::Drawing
