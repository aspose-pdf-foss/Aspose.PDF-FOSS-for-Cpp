#pragma once

// =============================================================================
// Aspose::Pdf::Facades::FontStyle — Standard14 (+ a few extras)
// font selector for FormattedText. Mirrors canonical
// Aspose.Pdf.Facades.FontStyle.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class FontStyle : int {
    Courier = 0,
    CourierBold = 1,
    CourierOblique = 2,
    CourierBoldOblique = 3,
    Helvetica = 4,
    HelveticaBold = 5,
    HelveticaOblique = 6,
    HelveticaBoldOblique = 7,
    Symbol = 8,
    TimesRoman = 9,
    TimesBold = 10,
    TimesItalic = 11,
    TimesBoldItalic = 12,
    ZapfDingbats = 13,
    Unknown = 14,
    CjkFont = 15,
};

}  // namespace Aspose::Pdf::Facades
