#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::GradientRadialShading — a radial gradient between two
// circles (PDF radial shading, ShadingType 3). Mirrors canonical
// Aspose.Pdf.Drawing.GradientRadialShading (Aspose.PDF 26.4.0).
//
// Start / End are the circle centres and StartingRadius / EndingRadius their
// radii, measured from the lower-left corner of the area being filled. With
// the defaults (both radii 0) the gradient spreads from the area's centre
// (StartColor) out to its corners (EndColor).
// =============================================================================

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/drawing/pattern_color_space.hpp>
#include <aspose/pdf/point.hpp>

namespace Aspose::Pdf::Drawing {

class GradientRadialShading : public PatternColorSpace {
public:
    GradientRadialShading();
    GradientRadialShading(const Aspose::Pdf::Color& startColor,
                          const Aspose::Pdf::Color& endColor);

    const Aspose::Pdf::Color& StartColor() const noexcept;
    void StartColor(const Aspose::Pdf::Color& value);
    const Aspose::Pdf::Color& EndColor() const noexcept;
    void EndColor(const Aspose::Pdf::Color& value);

    const Aspose::Pdf::Point& Start() const noexcept;
    void Start(const Aspose::Pdf::Point& value);
    const Aspose::Pdf::Point& End() const noexcept;
    void End(const Aspose::Pdf::Point& value);

    double StartingRadius() const noexcept;
    void StartingRadius(double value);
    double EndingRadius() const noexcept;
    void EndingRadius(double value);

private:
    Aspose::Pdf::Color start_color_;
    Aspose::Pdf::Color end_color_;
    Aspose::Pdf::Point start_{0.0, 0.0};
    Aspose::Pdf::Point end_{0.0, 0.0};
    double starting_radius_ = 0.0;
    double ending_radius_ = 0.0;
};

}  // namespace Aspose::Pdf::Drawing
