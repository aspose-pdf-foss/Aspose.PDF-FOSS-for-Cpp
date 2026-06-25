#pragma once

// =============================================================================
// Aspose::Pdf::GraphInfo — stroke / fill styling for a graph shape or a table
// border edge (PDF graphics state). Mirrors canonical Aspose.Pdf.GraphInfo
// (Aspose.PDF 26.4.0): line width, stroke / fill colour, dash pattern and an
// affine-transform descriptor. A copyable value type.
// =============================================================================

#include <vector>

#include <aspose/pdf/color.hpp>

namespace Aspose::Pdf {

class GraphInfo {
public:
    GraphInfo() = default;

    // Canonical GraphInfo.X / Y — the shape origin (read-only).
    double X() const noexcept;
    double Y() const noexcept;

    // Canonical GraphInfo.LineWidth — stroke width in points.
    float LineWidth() const noexcept;
    void LineWidth(float value);

    // Canonical GraphInfo.Color / FillColor — stroke and fill colours.
    const Aspose::Pdf::Color& Color() const noexcept;
    void Color(const Aspose::Pdf::Color& value);
    const Aspose::Pdf::Color& FillColor() const noexcept;
    void FillColor(const Aspose::Pdf::Color& value);

    // Canonical GraphInfo.DashArray / DashPhase — the dash pattern.
    const std::vector<int>& DashArray() const noexcept;
    void DashArray(std::vector<int> value);
    int DashPhase() const noexcept;
    void DashPhase(int value);

    // Canonical GraphInfo.IsDoubled — draw a doubled (twin-line) stroke.
    bool IsDoubled() const noexcept;
    void IsDoubled(bool value);

    // Canonical affine-transform descriptors.
    double SkewAngleX() const noexcept;
    void SkewAngleX(double value);
    double SkewAngleY() const noexcept;
    void SkewAngleY(double value);
    double ScalingRateX() const noexcept;
    void ScalingRateX(double value);
    double ScalingRateY() const noexcept;
    void ScalingRateY(double value);
    double RotationAngle() const noexcept;
    void RotationAngle(double value);

private:
    double x_ = 0.0;
    double y_ = 0.0;
    float line_width_ = 1.0f;
    Aspose::Pdf::Color color_;
    Aspose::Pdf::Color fill_color_;
    std::vector<int> dash_array_;
    int dash_phase_ = 0;
    bool is_doubled_ = false;
    double skew_angle_x_ = 0.0;
    double skew_angle_y_ = 0.0;
    double scaling_rate_x_ = 1.0;
    double scaling_rate_y_ = 1.0;
    double rotation_angle_ = 0.0;
};

}  // namespace Aspose::Pdf
