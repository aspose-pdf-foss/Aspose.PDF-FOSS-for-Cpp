#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::digest {

// Production API — hash `data` and return the digest.
// Output length is fixed per algorithm:
//   Md5    -> 16 bytes
//   Sha1   -> 20 bytes
//   Sha256 -> 32 bytes
//   Sha384 -> 48 bytes
//   Sha512 -> 64 bytes
// Empty input is valid and produces the algorithm's
// canonical empty-input digest. Throws std::runtime_error
// on any OpenSSL failure.
std::vector<std::byte> Md5(std::span<const std::byte> data);
std::vector<std::byte> Sha1(std::span<const std::byte> data);
std::vector<std::byte> Sha256(std::span<const std::byte> data);
std::vector<std::byte> Sha384(std::span<const std::byte> data);
std::vector<std::byte> Sha512(std::span<const std::byte> data);

// Gate-visible data type + helpers (for the
// two_oracle_agreement driver template).
struct Bundle {
    std::vector<std::byte> md5;     // 16 bytes
    std::vector<std::byte> sha1;    // 20 bytes
    std::vector<std::byte> sha256;  // 32 bytes
    std::vector<std::byte> sha384;  // 48 bytes
    std::vector<std::byte> sha512;  // 64 bytes
};

// Run all five digests on the input and return a bundle.
// Used by the freeze-gate driver only; production code
// calls the per-algorithm functions directly.
Bundle Parse(std::span<const std::byte> input);

// Serialise a Bundle as the text manifest the freeze-gate
// sidecar format mandates. Exactly 400 chars for any input.
// Format:
//   md5: <32 lowercase hex>\n
//   sha1: <40 lowercase hex>\n
//   sha256: <64 lowercase hex>\n
//   sha384: <96 lowercase hex>\n
//   sha512: <128 lowercase hex>\n
std::string ToCanonical(const Bundle& bundle);

}
