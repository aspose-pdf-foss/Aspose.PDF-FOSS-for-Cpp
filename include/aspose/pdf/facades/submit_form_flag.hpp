#pragma once

// =============================================================================
// Aspose::Pdf::Facades::SubmitFormFlag — submit-form format flag
// for PdfContentEditor / Form submit actions (PDF §12.7.5.2 /Flags).
// Mirrors canonical Aspose.Pdf.Facades.SubmitFormFlag.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class SubmitFormFlag : int {
    Fdf = 0,
    Html = 1,
    Xfdf = 2,
    FdfWithComments = 3,
    XfdfWithComments = 4,
    Pdf = 5,
};

}  // namespace Aspose::Pdf::Facades
