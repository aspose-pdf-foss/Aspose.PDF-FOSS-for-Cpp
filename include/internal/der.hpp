#pragma once

// =============================================================================
// foundation::der — a small DER reader (ITU-T X.690) for PKCS#12, X.509 and
// CMS: tag / length / value nodes over a borrowed byte span, plus the X.509
// certificate fields signing and verification need.
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::der {

// One TLV: `tag` is the identifier octet (single-byte tags only), `content`
// the value bytes, `whole` the full encoding (tag + length + value).
struct Node {
    std::uint8_t tag = 0;
    std::span<const std::byte> content;
    std::span<const std::byte> whole;
};

// Read the TLV starting at `pos` and advance `pos` past it. Throws
// std::runtime_error on truncated input, indefinite or oversized lengths.
Node Read(std::span<const std::byte> in, std::size_t& pos);
// Read exactly one TLV spanning all of `in`.
Node ReadOne(std::span<const std::byte> in);
// The TLVs inside a constructed node's content.
std::vector<Node> Children(const Node& n);
// An OBJECT IDENTIFIER in dotted form ("1.2.840.113549.1.7.2").
std::string Oid(const Node& n);
// A non-negative INTEGER's magnitude (leading zero bytes removed).
std::vector<std::byte> UnsignedInteger(const Node& n);
// A small non-negative INTEGER as a number.
std::uint64_t SmallInteger(const Node& n);

// The X.509 certificate fields used for CMS signing and verification.
struct Certificate {
    std::vector<std::byte> der;         // the whole certificate
    std::vector<std::byte> issuer;      // issuer Name, DER
    std::vector<std::byte> serial;      // serialNumber INTEGER TLV, DER
    std::vector<std::byte> subject;     // subject Name, DER
    std::string subject_cn;             // subject commonName ("" if none)
    std::vector<std::byte> modulus;     // RSA public key (empty: not RSA)
    std::vector<std::byte> exponent;
    std::string ec_curve_oid;           // EC public key: namedCurve OID
    std::vector<std::byte> ec_point;    // ... and its point (uncompressed)
};

// Parse a DER X.509 certificate. Throws std::runtime_error when malformed.
Certificate ParseCertificate(std::span<const std::byte> der);

// The commonName of a DER Name ("" when absent).
std::string CommonName(std::span<const std::byte> name_der);

}  // namespace foundation::der
