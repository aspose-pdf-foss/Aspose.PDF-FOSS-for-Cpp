#pragma once

// =============================================================================
// foundation::crypto — key derivation and legacy block ciphers needed to open
// PKCS#12 (.pfx / .p12) files: HMAC, PBKDF2 (RFC 8018), the PKCS#12 KDF
// (RFC 7292 appendix B), and Triple-DES / RC2 CBC decryption (RFC 2268).
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::crypto {

enum class Hash { Sha1, Sha256, Sha384, Sha512 };

std::size_t HashSize(Hash h);
std::vector<std::byte> HashOf(Hash h, std::span<const std::byte> data);

// HMAC (RFC 2104) with `h`.
std::vector<std::byte> Hmac(Hash h, std::span<const std::byte> key,
                            std::span<const std::byte> data);

// PBKDF2 with HMAC-`h` (RFC 8018 §5.2): `length` bytes.
std::vector<std::byte> Pbkdf2(Hash h, std::span<const std::byte> password,
                              std::span<const std::byte> salt,
                              std::uint32_t iterations, std::size_t length);

// PKCS#12 key derivation (RFC 7292 appendix B.2). `password` is the text
// password (encoded as a NUL-terminated big-endian BMPString); `id` is 1
// (key), 2 (IV) or 3 (MAC key).
std::vector<std::byte> Pkcs12Kdf(Hash h, const std::string& password,
                                 std::span<const std::byte> salt,
                                 std::uint8_t id, std::uint32_t iterations,
                                 std::size_t length);

// Triple-DES (EDE, 24-byte key) CBC decryption; strips PKCS#7 padding.
// Throws std::runtime_error on a bad length or padding.
std::vector<std::byte> Des3CbcDecrypt(std::span<const std::byte> key,
                                      std::span<const std::byte> iv,
                                      std::span<const std::byte> data);

// RC2 (RFC 2268) CBC decryption with `effective_bits` (e.g. 40, 128);
// strips PKCS#7 padding. Throws std::runtime_error on a bad length or
// padding.
std::vector<std::byte> Rc2CbcDecrypt(std::span<const std::byte> key,
                                     int effective_bits,
                                     std::span<const std::byte> iv,
                                     std::span<const std::byte> data);

// One raw RC2 block decryption (8 bytes), for the RFC 2268 test vectors.
std::vector<std::byte> Rc2DecryptBlock(std::span<const std::byte> key,
                                       int effective_bits,
                                       std::span<const std::byte> block);

}  // namespace foundation::crypto
