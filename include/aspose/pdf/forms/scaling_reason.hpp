#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ScalingReason — when an IconFit applies
// scaling (PDF §12.5.6.19 /MK dict /IF /SW entry). Mirrors
// canonical Aspose.Pdf.Forms.ScalingReason.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class ScalingReason : int {
    Always = 0,
    IconIsBigger = 1,
    IconIsSmaller = 2,
    Never = 3,
};

}  // namespace Aspose::Pdf::Forms
