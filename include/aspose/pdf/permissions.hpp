#pragma once

// =============================================================================
// Aspose::Pdf::Permissions — user-permission bitfield for /Encrypt /P.
//
// [Flags] enum mirroring canonical Aspose.Pdf.Permissions. Bit values
// are the PDF /P field bit positions per ISO 32000-1 §7.6.3.2 Table 22
// (1-based bit indices converted to bit-flag values: bit 3 -> 4, bit 4
// -> 8, etc.). Bits 1-2 are reserved by the PDF spec; bits 7-8 and 13+
// are unused by the v1.7 user-permission model.
//
// DLL surface (Aspose.PDF 26.4.0):
//   PrintDocument                  = 4
//   ModifyContent                  = 8
//   ExtractContent                 = 16
//   ModifyTextAnnotations          = 32
//   FillForm                       = 256
//   ExtractContentWithDisabilities = 512
//   AssembleDocument               = 1024
//   PrintingQuality                = 2048
//
// Bitwise composition (|, &, ^, ~, |=, &=, ^=) is supported via the
// operator overloads defined below — same ergonomics as .NET's [Flags]
// enum.
// =============================================================================

#include <type_traits>

namespace Aspose::Pdf {

enum class Permissions : int {
    PrintDocument = 4,
    ModifyContent = 8,
    ExtractContent = 16,
    ModifyTextAnnotations = 32,
    FillForm = 256,
    ExtractContentWithDisabilities = 512,
    AssembleDocument = 1024,
    PrintingQuality = 2048,
};

constexpr Permissions operator|(Permissions a, Permissions b) noexcept {
    using U = std::underlying_type_t<Permissions>;
    return static_cast<Permissions>(
        static_cast<U>(a) | static_cast<U>(b));
}

constexpr Permissions operator&(Permissions a, Permissions b) noexcept {
    using U = std::underlying_type_t<Permissions>;
    return static_cast<Permissions>(
        static_cast<U>(a) & static_cast<U>(b));
}

constexpr Permissions operator^(Permissions a, Permissions b) noexcept {
    using U = std::underlying_type_t<Permissions>;
    return static_cast<Permissions>(
        static_cast<U>(a) ^ static_cast<U>(b));
}

constexpr Permissions operator~(Permissions a) noexcept {
    using U = std::underlying_type_t<Permissions>;
    return static_cast<Permissions>(~static_cast<U>(a));
}

constexpr Permissions& operator|=(Permissions& a, Permissions b) noexcept {
    a = a | b;
    return a;
}

constexpr Permissions& operator&=(Permissions& a, Permissions b) noexcept {
    a = a & b;
    return a;
}

constexpr Permissions& operator^=(Permissions& a, Permissions b) noexcept {
    a = a ^ b;
    return a;
}

}  // namespace Aspose::Pdf
