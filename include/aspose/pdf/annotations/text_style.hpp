#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::TextStyle — text-rendering style
// carrier consumed by FreeTextAnnotation::TextStyle. Mirrors
// canonical Aspose.Pdf.Annotations.TextStyle.
//
// Canonical members dropped via translations/drops cascade:
//   * ToString — drops.yaml (deferred string format)
//   * Color (System.Drawing.Color) — translations dropped: cascade
//
// Only 4 of 6 canonical members ship: FontName, FontSize,
// Alignment (TextAlignment), HorizontalAlignment.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/text_alignment.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>

namespace Aspose::Pdf::Annotations {

class TextStyle {
public:
    TextStyle() noexcept = default;

    const std::string& FontName() const noexcept;
    void FontName(std::string value) noexcept;

    double FontSize() const noexcept;
    void FontSize(double value) noexcept;

    TextAlignment Alignment() const noexcept;
    void Alignment(TextAlignment value) noexcept;

    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value) noexcept;

private:
    std::string font_name_;
    double font_size_ = 12.0;
    TextAlignment alignment_ = TextAlignment::Left;
    Aspose::Pdf::HorizontalAlignment horizontal_alignment_
        = Aspose::Pdf::HorizontalAlignment::None;
};

}  // namespace Aspose::Pdf::Annotations
