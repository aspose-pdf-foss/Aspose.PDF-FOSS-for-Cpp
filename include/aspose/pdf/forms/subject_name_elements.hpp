#pragma once

// =============================================================================
// Aspose::Pdf::Forms::SubjectNameElements — X.509 distinguished-
// name components surfaced through a Signature's subject info
// (PDF §12.8 — digital signatures). Mirrors canonical
// Aspose.Pdf.Forms.SubjectNameElements.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class SubjectNameElements : int {
    CN = 0,    // Common name
    O = 1,     // Organization
    L = 2,     // Locality
    OU = 3,    // Organizational unit
    S = 4,     // State / province
    C = 5,     // Country
    E = 6,     // E-mail
};

}  // namespace Aspose::Pdf::Forms
