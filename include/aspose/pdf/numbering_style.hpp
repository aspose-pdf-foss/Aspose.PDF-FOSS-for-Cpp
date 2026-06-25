#pragma once

// =============================================================================
// Aspose::Pdf::NumberingStyle — the page-label numbering style (PDF §12.4.2,
// the /S entry of a page-label dictionary). Mirrors canonical
// Aspose.Pdf.NumberingStyle (Aspose.PDF 26.4.0).
// =============================================================================

namespace Aspose::Pdf {

enum class NumberingStyle : int {
    NumeralsArabic = 0,          // /D — decimal arabic (1, 2, 3 …)
    NumeralsRomanUppercase = 1,  // /R — uppercase roman (I, II, III …)
    NumeralsRomanLowercase = 2,  // /r — lowercase roman (i, ii, iii …)
    LettersUppercase = 3,        // /A — uppercase letters (A, B, C …)
    LettersLowercase = 4,        // /a — lowercase letters (a, b, c …)
    None = 5,                    // no /S — prefix-only / unnumbered
};

}  // namespace Aspose::Pdf
