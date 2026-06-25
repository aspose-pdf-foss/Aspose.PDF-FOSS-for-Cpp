#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::Border — annotation border carrier.
// Mirrors canonical Aspose.Pdf.Annotations.Border (Aspose.PDF
// 26.4.0). Constructed with a back-reference to the owning
// Annotation; persists via that back-reference's appearance
// stream at save time.
//
// Dropped from this beat: the Dash property (canonical
// Aspose.Pdf.Annotations.Dash has 'true' / 'false' member names
// reserved keywords in C++ that need
// investigation before shipping).
// =============================================================================

#include <aspose/pdf/annotations/border_effect.hpp>
#include <aspose/pdf/annotations/border_style.hpp>

namespace Aspose::Pdf::Annotations {

class Annotation;

class Border {
public:
    explicit Border(Annotation* parent) noexcept;

    double HCornerRadius() const noexcept;
    void HCornerRadius(double value) noexcept;

    double VCornerRadius() const noexcept;
    void VCornerRadius(double value) noexcept;

    int Width() const noexcept;
    void Width(int value) noexcept;

    int EffectIntensity() const noexcept;
    void EffectIntensity(int value) noexcept;

    BorderStyle Style() const noexcept;
    void Style(BorderStyle value) noexcept;

    BorderEffect Effect() const noexcept;
    void Effect(BorderEffect value) noexcept;

private:
    Annotation* parent_ = nullptr;
    double h_corner_radius_ = 0.0;
    double v_corner_radius_ = 0.0;
    int width_ = 1;
    int effect_intensity_ = 0;
    BorderStyle style_ = BorderStyle::Solid;
    BorderEffect effect_ = BorderEffect::None;
};

}  // namespace Aspose::Pdf::Annotations
