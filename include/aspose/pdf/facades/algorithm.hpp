#pragma once

// =============================================================================
// Aspose::Pdf::Facades::Algorithm — encryption algorithm selector
// for PdfFileSecurity (PDF §7.6.2). Mirrors canonical
// Aspose.Pdf.Facades.Algorithm.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class Algorithm : int {
    RC4 = 0,
    AES = 1,
};

}  // namespace Aspose::Pdf::Facades
