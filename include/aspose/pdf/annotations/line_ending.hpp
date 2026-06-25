#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::LineEnding — line/free-text annotation
// terminator-shape selector (PDF /LE entry per §12.5.6.7).
// Mirrors canonical Aspose.Pdf.Annotations.LineEnding.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class LineEnding : int {
    None = 0,
    Square = 1,
    Circle = 2,
    Diamond = 3,
    OpenArrow = 4,
    ClosedArrow = 5,
    Butt = 6,
    ROpenArrow = 7,
    RClosedArrow = 8,
    Slash = 9,
};

}  // namespace Aspose::Pdf::Annotations
