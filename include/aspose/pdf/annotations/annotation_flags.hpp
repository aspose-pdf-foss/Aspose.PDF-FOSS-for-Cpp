#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::AnnotationFlags — [Flags] bitfield
// for the /F entry on every PDF annotation per ISO 32000-1 §12.5.3.
// Mirrors canonical Aspose.Pdf.Annotations.AnnotationFlags
// (Aspose.PDF 26.4.0) verbatim.
//
// Bitwise composition (|, &, ^, ~, |=, &=, ^=) is supported via
// the operator overloads below — same ergonomics as .NET's
// [Flags] enum.
// =============================================================================

#include <type_traits>

namespace Aspose::Pdf::Annotations {

enum class AnnotationFlags : int {
    Default = 0,
    Invisible = 1,
    Hidden = 2,
    Print = 4,
    NoZoom = 8,
    NoRotate = 16,
    NoView = 32,
    ReadOnly = 64,
    Locked = 128,
    ToggleNoView = 256,
    LockedContents = 512,
};

constexpr AnnotationFlags operator|(
        AnnotationFlags a, AnnotationFlags b) noexcept {
    using U = std::underlying_type_t<AnnotationFlags>;
    return static_cast<AnnotationFlags>(
        static_cast<U>(a) | static_cast<U>(b));
}

constexpr AnnotationFlags operator&(
        AnnotationFlags a, AnnotationFlags b) noexcept {
    using U = std::underlying_type_t<AnnotationFlags>;
    return static_cast<AnnotationFlags>(
        static_cast<U>(a) & static_cast<U>(b));
}

constexpr AnnotationFlags operator^(
        AnnotationFlags a, AnnotationFlags b) noexcept {
    using U = std::underlying_type_t<AnnotationFlags>;
    return static_cast<AnnotationFlags>(
        static_cast<U>(a) ^ static_cast<U>(b));
}

constexpr AnnotationFlags operator~(AnnotationFlags a) noexcept {
    using U = std::underlying_type_t<AnnotationFlags>;
    return static_cast<AnnotationFlags>(~static_cast<U>(a));
}

constexpr AnnotationFlags& operator|=(
        AnnotationFlags& a, AnnotationFlags b) noexcept {
    a = a | b;
    return a;
}

constexpr AnnotationFlags& operator&=(
        AnnotationFlags& a, AnnotationFlags b) noexcept {
    a = a & b;
    return a;
}

constexpr AnnotationFlags& operator^=(
        AnnotationFlags& a, AnnotationFlags b) noexcept {
    a = a ^ b;
    return a;
}

}  // namespace Aspose::Pdf::Annotations
