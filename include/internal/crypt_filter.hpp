#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::crypt_filter {

// CFM (Crypt Filter Method) dispatch tag — same values PDF
// /Encrypt's CF dictionary uses, minus /Identity (consumers
// must short-circuit /Identity before reaching this primitive).
enum class Method {
    V2,     // RC4 (40-128 bit; PDF /V=2 in PDF 1.4-1.7)
    AESV2,  // AES-128-CBC (PDF /V=4 in PDF 1.6+)
    AESV3,  // AES-256-CBC (PDF /V=5 in PDF 2.0)
};

// Per-object encryption.
//
// file_key length must match `method`:
//   V2     — 5..16 bytes (per /Length)
//   AESV2  — 16 bytes
//   AESV3  — 32 bytes
//
// For AESV2 / AESV3, `iv` must be exactly 16 bytes; the
// returned bytes begin with `iv` (per PDF 32000 §7.6.2
// Algorithm 1a) followed by the AES-CBC ciphertext. For V2,
// `iv` is ignored — pass an empty span.
//
// Throws std::runtime_error on file_key/iv length mismatch
// with method, or on errors propagated from the underlying
// cipher primitive.
std::vector<std::byte> Encrypt(
    Method method,
    std::span<const std::byte> file_key,
    std::uint32_t object_id,
    std::uint32_t generation,
    std::span<const std::byte> plaintext,
    std::span<const std::byte> iv);

// Per-object decryption — symmetric counterpart of Encrypt
// for the same (method, file_key, object_id, generation)
// tuple. For AESV2 / AESV3, the first 16 bytes of
// `ciphertext` are interpreted as the IV and the remainder
// is the AES-CBC ciphertext. For V2, the entire `ciphertext`
// is the RC4-encrypted payload.
//
// Throws std::runtime_error on file_key length mismatch,
// ciphertext shorter than 16 bytes for AES variants, or on
// errors propagated from the cipher primitive (bad PKCS#7
// padding, etc.).
std::vector<std::byte> Decrypt(
    Method method,
    std::span<const std::byte> file_key,
    std::uint32_t object_id,
    std::uint32_t generation,
    std::span<const std::byte> ciphertext);

// Gate-only wrappers — DO NOT USE in production code.
// Implement the binding_roundtrip wire-format protocol
// described in crypt_filter.yaml: parse the (method,
// file_key, object_id, generation, iv) header off the input,
// dispatch through Encrypt/Decrypt, re-emit the header
// followed by cipher/plaintext.
std::vector<std::byte> Encode(std::span<const std::byte> input);
std::vector<std::byte> Decode(std::span<const std::byte> input);

}  // namespace foundation::crypt_filter
