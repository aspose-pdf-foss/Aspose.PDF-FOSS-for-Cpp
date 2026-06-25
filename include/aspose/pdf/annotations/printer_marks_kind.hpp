#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PrinterMarksKind — bitfield selector
// for PrinterMarkAnnotation::AddPrinterMarks (PDF §14.11.3.3
// printer mark annotation /MarksKind). Mirrors canonical
// Aspose.Pdf.Annotations.PrinterMarksKind.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class PrinterMarksKind : int {
    None = 0,
    TrimMarks = 1,
    BleedMarks = 2,
    RegistrationMarks = 4,
    ColorBars = 8,
    PageInformation = 16,
    All = 31,
};

}  // namespace Aspose::Pdf::Annotations
