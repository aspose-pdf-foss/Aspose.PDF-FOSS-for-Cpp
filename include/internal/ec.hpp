#pragma once

// =============================================================================
// foundation::ec — ECDSA over the NIST prime curves P-256, P-384 and P-521
// (FIPS 186-4 / SEC 1): public key derivation, signing with a deterministic
// nonce (RFC 6979) and verification. Arithmetic is on foundation::bigint in
// Jacobian coordinates. Signatures are DER Ecdsa-Sig-Value SEQUENCE {r, s}.
// Not constant-time: fine for signing documents with a local key, not for
// a service exposed to timing attackers.
// =============================================================================

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "crypto.hpp"

namespace foundation::ec {

struct Curve {
    std::string oid;           // namedCurve OID ("1.2.840.10045.3.1.7")
    std::string name;          // "P-256"
    std::size_t bytes = 0;     // field element size
    bigint::BigUint p, b, gx, gy, n;  // a = -3
    crypto::Hash hash;         // the matching digest (SHA-256/384/512)
};

// nullptr for curves other than P-256 / P-384 / P-521.
const Curve* CurveByOid(const std::string& oid);
const Curve* CurveByName(const std::string& name);

// The uncompressed public point 04 || X || Y for private key `d`.
std::vector<std::byte> PublicKey(const Curve& c, std::span<const std::byte> d);

// ECDSA over a message `digest` (made with `h`); `k` from RFC 6979 with
// HMAC-`h`. Returns the DER signature.
std::vector<std::byte> SignDigest(const Curve& c, std::span<const std::byte> d,
                                  std::span<const std::byte> digest, crypto::Hash h);

// Verify a DER signature over `digest` with the public point `q`
// (uncompressed). False on any malformed input.
bool VerifyDigest(const Curve& c, std::span<const std::byte> q,
                  std::span<const std::byte> digest, std::span<const std::byte> sig);

// True when (x, y) of the uncompressed point `q` lies on the curve.
bool OnCurve(const Curve& c, std::span<const std::byte> q);

// n·G is the point at infinity (curve parameter self-check).
bool OrderAnnihilatesGenerator(const Curve& c);

}  // namespace foundation::ec
