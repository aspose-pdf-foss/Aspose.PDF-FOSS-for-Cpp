#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::HighlightingMode — visual feedback
// mode for the user pointer interacting with LinkAnnotation
// (PDF /H entry per §12.5.6.5). Mirrors canonical Aspose.Pdf.
// Annotations.HighlightingMode.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class HighlightingMode : int {
    None = 0,
    Invert = 1,
    Outline = 2,
    Push = 3,
    Toggle = 4,
};

}  // namespace Aspose::Pdf::Annotations
