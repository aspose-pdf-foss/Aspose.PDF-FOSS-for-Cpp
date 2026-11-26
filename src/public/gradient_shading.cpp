// Bodies for Aspose::Pdf::Drawing::GradientAxialShading and
// GradientRadialShading — plain value holders read by the Save pipeline.

#include <aspose/pdf/drawing/gradient_axial_shading.hpp>
#include <aspose/pdf/drawing/gradient_radial_shading.hpp>

namespace Aspose::Pdf::Drawing {

using Aspose::Pdf::Color;
using Aspose::Pdf::Point;

GradientAxialShading::GradientAxialShading() = default;
GradientAxialShading::GradientAxialShading(const Color& startColor, const Color& endColor)
    : start_color_(startColor), end_color_(endColor) {}
const Color& GradientAxialShading::StartColor() const noexcept { return start_color_; }
void GradientAxialShading::StartColor(const Color& value) { start_color_ = value; }
const Color& GradientAxialShading::EndColor() const noexcept { return end_color_; }
void GradientAxialShading::EndColor(const Color& value) { end_color_ = value; }
const Point& GradientAxialShading::Start() const noexcept { return start_; }
void GradientAxialShading::Start(const Point& value) { start_ = value; }
const Point& GradientAxialShading::End() const noexcept { return end_; }
void GradientAxialShading::End(const Point& value) { end_ = value; }

GradientRadialShading::GradientRadialShading() = default;
GradientRadialShading::GradientRadialShading(const Color& startColor, const Color& endColor)
    : start_color_(startColor), end_color_(endColor) {}
const Color& GradientRadialShading::StartColor() const noexcept { return start_color_; }
void GradientRadialShading::StartColor(const Color& value) { start_color_ = value; }
const Color& GradientRadialShading::EndColor() const noexcept { return end_color_; }
void GradientRadialShading::EndColor(const Color& value) { end_color_ = value; }
const Point& GradientRadialShading::Start() const noexcept { return start_; }
void GradientRadialShading::Start(const Point& value) { start_ = value; }
const Point& GradientRadialShading::End() const noexcept { return end_; }
void GradientRadialShading::End(const Point& value) { end_ = value; }
double GradientRadialShading::StartingRadius() const noexcept { return starting_radius_; }
void GradientRadialShading::StartingRadius(double value) { starting_radius_ = value; }
double GradientRadialShading::EndingRadius() const noexcept { return ending_radius_; }
void GradientRadialShading::EndingRadius(double value) { ending_radius_ = value; }

}  // namespace Aspose::Pdf::Drawing
