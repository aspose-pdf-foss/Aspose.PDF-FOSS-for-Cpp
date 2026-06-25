#pragma once

// =============================================================================
// Aspose::Pdf::CryptoAlgorithm — encryption algorithm selector for
// Document::Encrypt and the Standard Security Handler. Maps to the
// PDF /Encrypt /V + /R combinations:
//
//   RC4x40   = 0   V=1 R=2   40-bit RC4
//   RC4x128  = 1   V=2 R=3   128-bit RC4
//   AESx128  = 2   V=4 R=4   AES-128-CBC via /CF.StdCF.CFM=/AESV2
//   AESx256  = 3   V=5 R=6   AES-256-CBC PDF 2.0 (Algorithm 2.B hash)
//
// DLL surface (Aspose.PDF 26.4.0): the canonical Aspose.Pdf.CryptoAlgorithm
// values verbatim.
// =============================================================================

namespace Aspose::Pdf {

enum class CryptoAlgorithm : int {
    RC4x40 = 0,
    RC4x128 = 1,
    AESx128 = 2,
    AESx256 = 3,
};

}  // namespace Aspose::Pdf
