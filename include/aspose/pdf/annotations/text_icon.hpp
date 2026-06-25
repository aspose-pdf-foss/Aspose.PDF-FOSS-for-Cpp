#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::TextIcon — sticky-note icon selector
// for TextAnnotation. Mirrors canonical Aspose.Pdf.Annotations.
// TextIcon (PDF /Subtype /Text /Name entry per §12.5.6.4).
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class TextIcon : int {
    Note = 0,
    Comment = 1,
    Key = 2,
    Help = 3,
    NewParagraph = 4,
    Paragraph = 5,
    Insert = 6,
    Check = 7,
    Cross = 8,
    Circle = 9,
    Star = 10,
    RightPointer = 11,
    RightArrow = 12,
    UpArrow = 13,
    UpLeftArrow = 14,
};

}  // namespace Aspose::Pdf::Annotations
