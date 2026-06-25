#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::aes_cbc {

// Production API — use from PDF /Encrypt consumers.
//
// key size selects the cipher: 16 bytes → AES-128-CBC,
// 32 bytes → AES-256-CBC. Any other key length throws.
// iv MUST be exactly 16 bytes (AES block size); any other
// iv length throws. data may have any length; Encrypt
// output is PKCS#7-padded to a multiple of 16 (always
// produces at least one full padding block, even for
// empty or block-aligned plaintext, per RFC 5652).
//
// Throws std::runtime_error on wrong key/iv size, or on
// Decrypt of malformed ciphertext (non-16-aligned length,
// bad padding).
std::vector<std::byte> Encrypt(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data);

std::vector<std::byte> Decrypt(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data);

// No-padding CBC — used by encrypt_parser for the V=5
// password-handler algorithms (Algorithm 2.A file-key
// recovery from /UE / /OE; Algorithm 2.B R=6 hash cascade).
// Input length MUST be a positive multiple of 16. Output
// length equals input length. No padding byte is emitted
// or stripped; DecryptNoPad does not validate the tail.
//
// Throws std::runtime_error on wrong key/iv size or on
// input whose length is zero or not a multiple of 16.
std::vector<std::byte> EncryptNoPad(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data);

std::vector<std::byte> DecryptNoPad(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data);

// Gate-only wrappers — DO NOT USE in production code.
// Bake in a hardcoded 16-byte test key + zero 16-byte IV
// purely to satisfy the binding_roundtrip driver's
// single-span Encode/Decode signature. These exist
// solely so the freeze-gate driver template renders
// without customisation.
std::vector<std::byte> Encode(std::span<const std::byte> data);
std::vector<std::byte> Decode(std::span<const std::byte> data);

}
