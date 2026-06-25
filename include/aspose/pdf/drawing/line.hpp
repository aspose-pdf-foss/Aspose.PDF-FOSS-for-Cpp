#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Line — a polyline shape (PDF vector graphics). Mirrors
// canonical Aspose.Pdf.Drawing.Line (Aspose.PDF 26.4.0): a flat [x0 y0 x1 y1 …]
// position array stroked with the shape's GraphInfo.
// =============================================================================

#include <vector>

#include <aspose/pdf/drawing/shape.hpp>

namespace Aspose::Pdf::Drawing {

class Line : public Shape {
public:
    explicit Line(std::vector<float> positionArray);

    // Canonical Line.PositionArray — flat [x0 y0 x1 y1 …] vertex coordinates.
    const std::vector<float>& PositionArray() const noexcept;
    void PositionArray(std::vector<float> value);

    bool CheckBounds(double containerWidth,
                     double containerHeight) const override;

private:
    std::vector<float> position_array_;
};

}  // namespace Aspose::Pdf::Drawing
