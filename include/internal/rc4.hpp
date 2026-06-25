#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::rc4 {

// Production API — use from PDF /Encrypt consumers.
//
// RC4 is SYMMETRIC: the same keystream XOR encrypts and
// decrypts. One function covers both directions.
//
// key size must be between 5 and 16 bytes (40 to 128 bits
// per PDF §7.6.4). Any length outside that range throws.
// data may be any length; output is exactly data.size()
// bytes (stream cipher, no padding, no block alignment).
//
// Throws std::runtime_error on any OpenSSL failure (legacy
// provider unavailable, EVP_CIPHER_fetch returns null, or
// any Init/Update/Final returns 0).
std::vector<std::byte> Apply(
    std::span<const std::byte> key,
    std::span<const std::byte> data);

// Gate-only wrappers — DO NOT USE in production code.
// Both call Apply() with a hardcoded 16-byte test key so
// the binding_roundtrip driver (which expects single-span
// Encode/Decode signatures) can exercise the round-trip.
// Encode and Decode are BIT-EXACT the same function —
// RC4 is symmetric, so there's no distinction between
// "encrypt" and "decrypt" direction.
std::vector<std::byte> Encode(std::span<const std::byte> data);
std::vector<std::byte> Decode(std::span<const std::byte> data);

}
