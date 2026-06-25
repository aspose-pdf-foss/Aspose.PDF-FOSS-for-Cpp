#pragma once

// =============================================================================
// Aspose::Pdf::Forms::PKCS1 — PKCS#1 RSA signature (legacy PDF
// /SubFilter /adbe.x509.rsa_sha1). Mirrors canonical
// Aspose.Pdf.Forms.PKCS1; extends Signature.
//
// Phased drops:
//   * .ctor(Stream image) / .ctor(Stream pfx, string password) —
//     Stream cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf::Forms {

class PKCS1 : public Signature {
public:
    PKCS1() = default;
    PKCS1(const std::string& pfx, const std::string& password);
};

}  // namespace Aspose::Pdf::Forms
