#pragma once

// =============================================================================
// foundation::pkcs7 — detached CMS / PKCS#7 SignedData builder.
//
// Hand-authored foundation crypto backing the digital-signature parity beat
// (Aspose::Pdf::Facades::PdfFileSignature). Builds a DER-encoded
// `adbe.pkcs7.detached` blob over arbitrary content, the way a PDF signature
// /Contents value requires:
//
//   * SHA-256 message digest of the content (foundation::digest).
//   * Authenticated attributes: contentType, signingTime, messageDigest.
//   * RSA PKCS#1 v1.5 signature (foundation::bigint modexp) over the
//     DER of the SignedAttributes.
//   * Embedded signer certificate + issuerAndSerialNumber SID.
//
// The signer identity is a bundled RSA-2048 + self-signed X.509 test fixture
// (include/internal/pkcs7_keymat.hpp). The cryptographic construction — DER,
// PKCS#1 v1.5 padding, RSA modexp, CMS assembly — is entirely from-scratch.
// =============================================================================

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace foundation::pkcs7 {

// Build a detached SignedData (DER) over `content`. `signing_time_utc` is a
// 13-character UTCTime string ("YYMMDDHHMMSSZ"). Throws std::runtime_error on
// malformed bundled key material.
std::vector<std::byte> SignDetached(std::span<const std::byte> content,
                                    const std::string& signing_time_utc);

// The subject common name of the bundled signer certificate
// ("Aspose.PDF FOSS Test Signer"). Returned by the facade's GetSignerName.
std::string BundledSignerName();

}  // namespace foundation::pkcs7
