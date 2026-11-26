#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::GradientAxialShading — a linear gradient between two
// colours (PDF axial shading, ShadingType 2). Mirrors canonical
// Aspose.Pdf.Drawing.GradientAxialShading (Aspose.PDF 26.4.0).
//
// Use it as a fill:
//     Color fill;
//     fill.PatternColorSpace(std::make_shared<GradientAxialShading>(from, to));
//     shape.GraphInfo().FillColor(fill);   // or Page::Background(fill), ...
//
// Start / End are measured from the lower-left corner of the area being
// filled (a shape's bounding box, a table cell, the page). When both are left
// at (0, 0) the gradient runs across the area from left to right. Beyond
// Start and End the end colours extend.
// =============================================================================

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/drawing/pattern_color_space.hpp>
#include <aspose/pdf/point.hpp>

namespace Aspose::Pdf::Drawing {

class GradientAxialShading : public PatternColorSpace {
public:
    GradientAxialShading();
    GradientAxialShading(const Aspose::Pdf::Color& startColor,
                         const Aspose::Pdf::Color& endColor);

    // Canonical StartColor / EndColor — the gradient's end colours.
    const Aspose::Pdf::Color& StartColor() const noexcept;
    void StartColor(const Aspose::Pdf::Color& value);
    const Aspose::Pdf::Color& EndColor() const noexcept;
    void EndColor(const Aspose::Pdf::Color& value);

    // Canonical Start / End — the gradient axis (see the header note).
    const Aspose::Pdf::Point& Start() const noexcept;
    void Start(const Aspose::Pdf::Point& value);
    const Aspose::Pdf::Point& End() const noexcept;
    void End(const Aspose::Pdf::Point& value);

private:
    Aspose::Pdf::Color start_color_;
    Aspose::Pdf::Color end_color_;
    Aspose::Pdf::Point start_{0.0, 0.0};
    Aspose::Pdf::Point end_{0.0, 0.0};
};

}  // namespace Aspose::Pdf::Drawing
