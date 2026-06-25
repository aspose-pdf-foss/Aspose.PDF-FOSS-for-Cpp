#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PositioningMode — text-positioning mode
// for FormattedText line layout (legacy vs modern line-spacing
// metrics). Mirrors canonical Aspose.Pdf.Facades.PositioningMode.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class PositioningMode : int {
    Legacy = 0,
    ModernLineSpacing = 1,
    Current = 2,
};

}  // namespace Aspose::Pdf::Facades
