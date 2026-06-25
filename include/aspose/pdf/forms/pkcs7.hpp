#pragma once

// =============================================================================
// Aspose::Pdf::Forms::PKCS7 — PKCS#7 signature (PDF /SubFilter
// /adbe.pkcs7.sha1). Mirrors canonical Aspose.Pdf.Forms.PKCS7;
// extends Signature.
//
// Phased drops:
//   * .ctor(Stream pfx, string password) — Stream cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf::Forms {

class PKCS7 : public Signature {
public:
    PKCS7() = default;
    PKCS7(const std::string& pfx, const std::string& password);
};

}  // namespace Aspose::Pdf::Forms
