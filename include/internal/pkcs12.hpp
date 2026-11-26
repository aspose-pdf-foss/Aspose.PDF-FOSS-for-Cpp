#pragma once

// =============================================================================
// foundation::pkcs12 — open a PKCS#12 (.pfx / .p12) file (RFC 7292): check its
// password MAC and extract the RSA private key and its certificate chain.
// Encryption: PBES2 (PBKDF2-HMAC-SHA1/256 with AES-128/192/256-CBC) and the
// legacy pbeWithSHAAnd3-KeyTripleDES-CBC / pbeWithSHAAnd40/128BitRC2-CBC.
// =============================================================================

#include <span>
#include <string>
#include <vector>

#include "der.hpp"

namespace foundation::pkcs12 {

struct RsaPrivateKey {
    std::vector<std::byte> modulus;
    std::vector<std::byte> public_exponent;
    std::vector<std::byte> private_exponent;
};

struct Contents {
    RsaPrivateKey key;
    der::Certificate signer;                  // the key's certificate
    std::vector<der::Certificate> chain;      // the other certificates
};

// Throws std::runtime_error: "pkcs12: wrong password" when the MAC does not
// verify, or a message naming the unsupported / malformed part.
Contents Parse(std::span<const std::byte> pfx, const std::string& password);

}  // namespace foundation::pkcs12
