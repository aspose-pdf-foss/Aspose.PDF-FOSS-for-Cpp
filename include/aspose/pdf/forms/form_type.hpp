#pragma once

// =============================================================================
// Aspose::Pdf::Forms::FormType — top-level form encoding (AcroForm
// vs XFA static / dynamic). Mirrors canonical
// Aspose.Pdf.Forms.FormType.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class FormType : int {
    Standard = 0,
    Static = 1,
    Dynamic = 2,
};

}  // namespace Aspose::Pdf::Forms
