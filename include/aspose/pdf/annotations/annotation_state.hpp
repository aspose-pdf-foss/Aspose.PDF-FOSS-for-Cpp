#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::AnnotationState — review-state selector
// per PDF 32000-1:2008 §12.5.5 annotation review state. Mirrors
// canonical Aspose.Pdf.Annotations.AnnotationState.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class AnnotationState : int {
    Marked = 0,
    Unmarked = 1,
    Accepted = 2,
    Rejected = 3,
    Cancelled = 4,
    Completed = 5,
    None = 6,
};

}  // namespace Aspose::Pdf::Annotations
