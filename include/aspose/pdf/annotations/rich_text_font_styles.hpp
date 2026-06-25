#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::RichTextFontStyles — [Flags] bitfield
// for FreeTextAnnotation::SetTextStyle. Mirrors canonical
// Aspose.Pdf.Annotations.RichTextFontStyles.
// =============================================================================

#include <type_traits>

namespace Aspose::Pdf::Annotations {

enum class RichTextFontStyles : int {
    Plain = 0,
    Italic = 1,
    Bold = 2,
    BoldItalic = 3,
};

constexpr RichTextFontStyles operator|(
        RichTextFontStyles a, RichTextFontStyles b) noexcept {
    using U = std::underlying_type_t<RichTextFontStyles>;
    return static_cast<RichTextFontStyles>(
        static_cast<U>(a) | static_cast<U>(b));
}

constexpr RichTextFontStyles operator&(
        RichTextFontStyles a, RichTextFontStyles b) noexcept {
    using U = std::underlying_type_t<RichTextFontStyles>;
    return static_cast<RichTextFontStyles>(
        static_cast<U>(a) & static_cast<U>(b));
}

}  // namespace Aspose::Pdf::Annotations
