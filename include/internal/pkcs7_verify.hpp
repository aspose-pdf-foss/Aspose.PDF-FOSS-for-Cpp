#pragma once

// =============================================================================
// foundation::pkcs7 (verify) — check a detached CMS / PKCS#7 SignedData
// (RFC 5652) over its content: the signer's certificate is found by its
// issuer and serial number (or tried in turn), the content digest
// (SHA-1/256/384/512) must equal the signed messageDigest attribute, and the
// RSA PKCS#1 v1.5, RSASSA-PSS (MGF1) or ECDSA (P-256/384/521) signature over the signed
// attributes must verify with the certificate's public key. Certificate
// trust and revocation are NOT
// checked. Also accepts adbe.pkcs7.sha1 (the SHA-1 of the content as
// encapsulated content).
// =============================================================================

#include <cstddef>
#include <span>
#include <string>

#include "der.hpp"

namespace foundation::pkcs7 {

struct Verification {
    bool valid = false;
    std::string error;          // why not valid ("" when valid)
    der::Certificate signer;    // the signer's certificate (when found)
};

// `cms` may carry trailing zero padding (a PDF /Contents placeholder).
Verification VerifyDetached(std::span<const std::byte> cms,
                            std::span<const std::byte> content);

}  // namespace foundation::pkcs7
