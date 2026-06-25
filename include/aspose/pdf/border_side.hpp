#pragma once

// =============================================================================
// Aspose::Pdf::BorderSide — which side(s) of a table cell / row / table a
// BorderInfo applies to (PDF table model). A [Flags] enum: sides combine
// bitwise, and All / Box select every side. Mirrors canonical
// Aspose.Pdf.BorderSide (Aspose.PDF 26.4.0).
// =============================================================================

namespace Aspose::Pdf {

enum class BorderSide : int {
    None = 0,
    Left = 1,
    Top = 2,
    Right = 4,
    Bottom = 8,
    All = 15,
    Box = 15,
};

constexpr BorderSide operator|(BorderSide a, BorderSide b) {
    return static_cast<BorderSide>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr BorderSide operator&(BorderSide a, BorderSide b) {
    return static_cast<BorderSide>(static_cast<int>(a) & static_cast<int>(b));
}
constexpr bool HasSide(BorderSide value, BorderSide flag) {
    return (static_cast<int>(value) & static_cast<int>(flag)) != 0;
}

}  // namespace Aspose::Pdf
