#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "encrypt_parser.hpp"

namespace foundation::encrypt_writer {

using foundation::encrypt_parser::EncryptDict;

// Supported PDF /Encrypt variants — same set as
// foundation::encrypt_parser (the read-side counterpart):
//   * V=1 R=2  — 40-bit RC4
//   * V=2 R=3  — variable-length RC4 (40..128, multiple of 8)
//   * V=4 R=4  — 128-bit AES via /CF.StdCF.CFM=/AESV2
//   * V=5 R=5  — AES-256 (PDF 1.7 Ext. Level 3, single-pass
//                SHA-256 key derivation)
//   * V=5 R=6  — AES-256 (PDF 2.0 / ISO 32000-2, iterative
//                Algorithm 2.B hash)
//
// The writer is the COMPUTE-side counterpart to encrypt_parser's
// Authenticate. Given the inputs below, it produces a populated
// EncryptDict (the same struct the parser CONSUMES) and the
// document encryption key for crypt_filter dispatch.
struct EncryptOutput {
    EncryptDict encrypt_dict;
    std::vector<std::byte> file_key;
};

// Build a /Encrypt dictionary and document encryption key from
// the inputs.
//
// `user_password`, `owner_password`:
//     raw password bytes (post any Unicode normalisation /
//     SASLprep / 127-byte truncation the consumer needs to do —
//     this primitive does NOT perform PDF 2.0 §7.6.4.3.4
//     normalisation). If owner_password is empty, the user
//     password is used in its place for Algorithm 3.
//
// `p`:
//     /P permissions bitfield (signed 32-bit integer; bit
//     semantics are the consumer's responsibility).
//
// `v`, `r`, `length_bits`:
//     /Encrypt /V, /R, /Length selectors. MUST be one of the
//     supported (V, R) combinations above with `length_bits`
//     consistent with the combination:
//         (1, 2)  →  length_bits == 40
//         (2, 3)  →  40 <= length_bits <= 128, multiple of 8
//         (4, 4)  →  length_bits == 128
//         (5, 5)  →  length_bits == 256
//         (5, 6)  →  length_bits == 256
//
// `file_id`:
//     /ID[0] permanent identifier (typically 16 bytes). R<5
//     algorithms incorporate it via Algorithms 2 and 5; R>=5
//     ignores it. An empty span is acceptable for R>=5.
//
// `encrypt_metadata`:
//     /EncryptMetadata. Persisted into the produced dict;
//     ALSO affects Algorithm 2 step 6 (R>=4 only: when false,
//     append 0xFFFFFFFF to the MD5 input) and is encoded into
//     /Perms plaintext byte 8 (R>=5).
//
// `salt`:
//     Deterministic random bytes the writer would otherwise
//     CSPRNG-generate. Layout:
//
//       R<5  (16 bytes):
//           bytes 0..15 = trailing 16 bytes of /U after
//                         Algorithm 5 emits the first 16. R=2
//                         doesn't actually consume this (its
//                         Algorithm 4 is deterministic), but the
//                         signature accepts 16 bytes for shape
//                         consistency.
//
//       R>=5 (68 bytes):
//           bytes  0..7   = user validation salt
//           bytes  8..15  = user key salt
//           bytes 16..23  = owner validation salt
//           bytes 24..31  = owner key salt
//           bytes 32..63  = file_key (32 bytes — document
//                           encryption key)
//           bytes 64..67  = /Perms anti-replay tail (4 bytes)
//
// Throws std::runtime_error on:
//   * unsupported (v, r) combination
//   * length_bits not in the valid range for (v, r)
//   * salt size mismatch — 16 bytes required for R<5; 68 bytes
//     required for R>=5
//   * file_id empty when r<5 (Algorithm 2 step 5 / Algorithm 5
//     both require it)
//   * any underlying digest / RC4 / AES error
EncryptOutput BuildEncryptDict(
    std::span<const std::byte> user_password,
    std::span<const std::byte> owner_password,
    std::int32_t p,
    int v,
    int r,
    int length_bits,
    std::span<const std::byte> file_id,
    bool encrypt_metadata,
    std::span<const std::byte> salt);

}  // namespace foundation::encrypt_writer
