#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::DefaultAppearance — text-rendering
// appearance carrier consumed by FreeTextAnnotation. Mirrors
// canonical Aspose.Pdf.Annotations.DefaultAppearance.
//
// Canonical members dropped via translations.yaml cascade:
//   * .ctor(string fontName, double, System.Drawing.Color) —
//     Drawing.Color in dropped: list
//   * .ctor(Aspose.Pdf.Text.Font, double, System.Drawing.Color) —
//     both types in dropped: list
//   * TextColor (Drawing.Color)
//   * Font (Aspose.Pdf.Text.Font)
//
// Only the default ctor + 3 simple string/double properties +
// the read-only Text property ship.
// =============================================================================

#include <string>

namespace Aspose::Pdf::Annotations {

class DefaultAppearance {
public:
    DefaultAppearance() noexcept = default;

    double FontSize() const noexcept;
    void FontSize(double value) noexcept;

    const std::string& FontName() const noexcept;
    void FontName(std::string value) noexcept;

    const std::string& FontResourceName() const noexcept;
    void FontResourceName(std::string value) noexcept;

    // Read-only — composed string of the form "<FontName> <FontSize>".
    const std::string& Text() const noexcept;

private:
    mutable std::string text_cache_;
    std::string font_name_;
    std::string font_resource_name_;
    double font_size_ = 12.0;
};

}  // namespace Aspose::Pdf::Annotations
