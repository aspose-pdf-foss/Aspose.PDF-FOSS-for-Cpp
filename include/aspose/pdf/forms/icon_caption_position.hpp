#pragma once

// =============================================================================
// Aspose::Pdf::Forms::IconCaptionPosition — placement of a push-
// button widget's caption relative to its icon (PDF §12.5.6.19
// /MK dict /TP entry). Mirrors canonical
// Aspose.Pdf.Forms.IconCaptionPosition.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class IconCaptionPosition : int {
    NoIcon = 0,
    NoCaption = 1,
    CaptionBelowIcon = 2,
    CaptionAboveIcon = 3,
    CaptionToTheRight = 4,
    CaptionToTheLeft = 5,
    CaptionOverlaid = 6,
};

}  // namespace Aspose::Pdf::Forms
