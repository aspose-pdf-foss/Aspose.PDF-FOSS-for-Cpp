#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::encrypt_parser {

// Supported PDF /Encrypt variants (Standard security handler):
//   * V=1 R=2  — 40-bit RC4
//   * V=2 R=3  — variable-length RC4 (40..128, multiple of 8)
//   * V=4 R=4  — 128-bit AES via /CF.StdCF.CFM=/AESV2
//   * V=5 R=5  — AES-256 (PDF 1.7 Ext. Level 3, deprecated single-
//                pass SHA-256 key derivation). Algorithms 2.A, 11.
//   * V=5 R=6  — AES-256 (PDF 2.0 / ISO 32000-2). Algorithms 2.A,
//                2.B (iterative SHA-2 / AES-128 cascade), 11.
// V=1/V=2/V=4 authentication uses Algorithm 2 (file_key derivation)
// + Algorithm 4 (R=2 /U test) or Algorithm 5 (R≥3 /U test) +
// Algorithm 6/7 (user/owner password match). For V=5 the file_key
// is recovered directly from /UE (user) or /OE (owner) by AES-256-
// CBC-decrypting them with an intermediate key derived from the
// password (and /U for owner) — see PDF 2.0 §7.6.4.3 Algorithms
// 2.A/2.B + Algorithm 11 (user) + Algorithm 12 (owner).
struct EncryptDict {
    int v;                          // 1, 2, 4, or 5
    int r;                          // 2, 3, 4, 5, or 6
    int length_bits;                // 40 for V=1; 40..128 (mult
                                    // of 8) for V=2; 128 for V=4;
                                    // 256 for V=5
    std::int32_t p;                 // permissions bitmask
    std::vector<std::byte> o;       // /O — 32 bytes (V<5);
                                    //      48 bytes (V=5)
    std::vector<std::byte> u;       // /U — 32 bytes (V<5);
                                    //      48 bytes (V=5)
    std::vector<std::byte> oe;      // /OE — empty (V<5);
                                    //       32 bytes (V=5)
    std::vector<std::byte> ue;      // /UE — empty (V<5);
                                    //       32 bytes (V=5)
    std::vector<std::byte> perms;   // /Perms — empty (V<5);
                                    //          16 bytes (V=5)
    bool encrypt_metadata;          // default true; affects key
                                    // derivation per Alg. 2 step 6
                                    // (only honoured for R≥4) and
                                    // is verified against the
                                    // decrypted /Perms for V=5
};

// Successful authentication produces a file_key of length
// length_bits/8 bytes (5 for V=1, 5..16 for V=2, 16 for V=4,
// 32 for V=5). `authenticated == false` means the supplied
// password did not match either the user (/U) or the owner (/O)
// entry; file_key is then empty.
struct AuthResult {
    bool authenticated;
    std::vector<std::byte> file_key;
    bool used_owner_path;           // true if /O matched the
                                    // password instead of /U
};

// Authenticate a password against a /Encrypt dict.
// Tries the password as a user password first (Algorithm 6 for
// V<5 / Algorithm 11 for V=5); if that fails, tries it as an
// owner password (Algorithm 7+6 for V<5 / Algorithm 12 for V=5).
// The returned file_key is fit for per-object decryption via
// foundation::crypt_filter (RC4 for V=1/V=2; AES-128 for V=4;
// AES-256 directly applied to objects without per-object key
// derivation for V=5).
//
// Throws std::runtime_error on:
//   * unsupported (v, r) combination — must be one of (1,2),
//     (2,3), (4,4), (5,5), (5,6)
//   * length_bits not in the valid range for (v, r) — must be
//     40 for V=1; 40..128 (multiple of 8) for V=2; 128 for V=4;
//     256 for V=5
//   * /O or /U wrong size — 32 bytes for V<5; 48 bytes for V=5
//   * /OE, /UE not 32 bytes when V=5; /Perms not 16 bytes when V=5
//   * file_id empty when v in {4} (V=4 trailer must include /ID;
//     V=1/V=2 also feed /ID into Algorithm 2 but qpdf-produced
//     files always have one — empty is treated as zero-length;
//     V=5 doesn't use /ID in its key derivation at all)
//   * any underlying digest / RC4 / AES error
AuthResult Authenticate(
    const EncryptDict& enc,
    std::span<const std::byte> password,
    std::span<const std::byte> file_id);

// Pad/truncate a password to exactly 32 bytes per PDF §7.6.3.3 Algorithm 2
// step 1 (first 32 bytes, then the standard 32-byte padding string).
std::vector<std::byte> PadPassword(std::span<const std::byte> password);

}  // namespace foundation::encrypt_parser
