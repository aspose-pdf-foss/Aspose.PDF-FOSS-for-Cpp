#pragma once

// =============================================================================
// Aspose::Pdf::Forms::Symbology — barcode encoding used by a
// BarcodeField (PDF §12.7.4 — interactive form fields, Aspose
// extension to support PDF417 / QR / DataMatrix). Mirrors
// canonical Aspose.Pdf.Forms.Symbology.
// =============================================================================

namespace Aspose::Pdf::Forms {

enum class Symbology : int {
    PDF417 = 0,
    QRCode = 1,
    DataMatrix = 2,
};

}  // namespace Aspose::Pdf::Forms
