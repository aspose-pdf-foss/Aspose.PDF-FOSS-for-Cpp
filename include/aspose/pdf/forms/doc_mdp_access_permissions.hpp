#pragma once

// =============================================================================
// Aspose::Pdf::Forms::DocMDPAccessPermissions — access permission
// level granted by a DocMDP (Modification Detection and Prevention)
// signature (PDF §12.8.2.2.1 /P entry). Mirrors canonical
// Aspose.Pdf.Forms.DocMDPAccessPermissions.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class DocMDPAccessPermissions : int {
    NoChanges = 1,
    FillingInForms = 2,
    AnnotationModification = 3,
};

}  // namespace Aspose::Pdf::Forms
