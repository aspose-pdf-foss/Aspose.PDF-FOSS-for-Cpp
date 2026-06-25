#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ExternalSignature — externally-provided
// signature (the caller produces the signed bytes; the lib only
// wraps them). Mirrors canonical Aspose.Pdf.Forms.ExternalSignature;
// extends Signature.
//
// Phased drops:
//   * Certificate field (X509Certificate2) — X509Certificate2 cascade
//   * 3 ctors taking X509Certificate2 — X509Certificate2 cascade
//   * .ctor(base64, DigestHashAlgorithm) — DigestHashAlgorithm cascade
//
// Only the (base64, detached) ctor ships.
// =============================================================================

#include <string>

#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf::Forms {

class ExternalSignature : public Signature {
public:
    // The PEM/base64-encoded public certificate bytes + a flag
    // marking the signature as detached (true → /SubFilter
    // /adbe.pkcs7.detached; false → /adbe.pkcs7.sha1).
    ExternalSignature(const std::string& base64, bool detached);

protected:
    std::string base64_;
    bool detached_ = true;
};

}  // namespace Aspose::Pdf::Forms
