#include <aspose/pdf/annotations/border.hpp>

namespace Aspose::Pdf::Annotations {

Border::Border(Annotation* parent) noexcept : parent_(parent) {}

double Border::HCornerRadius() const noexcept { return h_corner_radius_; }
void Border::HCornerRadius(double v) noexcept { h_corner_radius_ = v; }

double Border::VCornerRadius() const noexcept { return v_corner_radius_; }
void Border::VCornerRadius(double v) noexcept { v_corner_radius_ = v; }

int Border::Width() const noexcept { return width_; }
void Border::Width(int v) noexcept { width_ = v; }

int Border::EffectIntensity() const noexcept { return effect_intensity_; }
void Border::EffectIntensity(int v) noexcept { effect_intensity_ = v; }

BorderStyle Border::Style() const noexcept { return style_; }
void Border::Style(BorderStyle v) noexcept { style_ = v; }

BorderEffect Border::Effect() const noexcept { return effect_; }
void Border::Effect(BorderEffect v) noexcept { effect_ = v; }

}  // namespace Aspose::Pdf::Annotations
