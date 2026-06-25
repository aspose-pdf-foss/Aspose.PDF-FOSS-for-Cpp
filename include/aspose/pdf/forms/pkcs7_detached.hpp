#pragma once

// =============================================================================
// Aspose::Pdf::Forms::PKCS7Detached — detached PKCS#7 signature
// (PDF /SubFilter /adbe.pkcs7.detached — the modern default).
// Mirrors canonical Aspose.Pdf.Forms.PKCS7Detached; extends
// Signature.
//
// Phased drops:
//   * Stream-bound ctors — Stream cascade
//   * DigestHashAlgorithm-bound ctors — DigestHashAlgorithm cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf::Forms {

class PKCS7Detached : public Signature {
public:
    PKCS7Detached() = default;
    PKCS7Detached(const std::string& pfx, const std::string& password);
};

}  // namespace Aspose::Pdf::Forms
