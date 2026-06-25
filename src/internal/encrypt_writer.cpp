// foundation::encrypt_writer — PDF Standard Security Handler
// WRITE side. Computes /O, /U, /OE, /UE, /Perms and the document
// encryption key from a user/owner password pair, /P, V, R,
// /Length, /ID[0], /EncryptMetadata, and a deterministic salt
// buffer (CSPRNG in production; pinned in tests).
//
// Counterpart to foundation::encrypt_parser. The dict struct
// (EncryptDict) is re-used verbatim from encrypt_parser.hpp.
//
// Helper functions (PadPassword, DeriveFileKey, ComputeUForR3Plus,
// Algorithm2B, ComputeV5Hash, TruncatePassword) are DUPLICATED
// from encrypt_parser.cpp because that primitive is locked
// — any refactor to share helpers would
// invalidate the freeze hash. Drift between parser and writer
// is gate-detected: this primitive's .case fixtures are derived
// from the SAME pikepdf-encrypted PDFs encrypt_parser
// authenticates, so any divergence in /U / /UE / /Perms math
// surfaces as a fixture mismatch.

#include "encrypt_writer.hpp"

#include "aes_cbc.hpp"
#include "digest.hpp"
#include "rc4.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace foundation::encrypt_writer {

namespace {

// =========================================================
// Shared helpers — duplicates of encrypt_parser.cpp's anon-
// namespace utilities. See file header for the duplication
// rationale.
// =========================================================

constexpr std::array<std::uint8_t, 32> kPadding = {
    0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
    0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
    0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
    0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A,
};

void AppendBytes(std::vector<std::byte>& dst,
                 std::span<const std::byte> src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

void AppendRaw(std::vector<std::byte>& dst,
               const void* data, std::size_t n) {
    const auto* p = static_cast<const std::byte*>(data);
    dst.insert(dst.end(), p, p + n);
}

std::vector<std::byte> PadPassword(
        std::span<const std::byte> pwd) {
    const std::size_t take = std::min<std::size_t>(pwd.size(), 32);
    std::vector<std::byte> out(pwd.begin(), pwd.begin() + take);
    out.reserve(32);
    for (std::size_t i = take; i < 32; ++i) {
        out.push_back(static_cast<std::byte>(kPadding[i - take]));
    }
    return out;
}

void AppendP(std::vector<std::byte>& dst, std::int32_t p) {
    const auto u = static_cast<std::uint32_t>(p);
    const std::uint8_t bytes[4] = {
        static_cast<std::uint8_t>(u & 0xFF),
        static_cast<std::uint8_t>((u >> 8) & 0xFF),
        static_cast<std::uint8_t>((u >> 16) & 0xFF),
        static_cast<std::uint8_t>((u >> 24) & 0xFF),
    };
    AppendRaw(dst, bytes, 4);
}

// Algorithm 2 — derive the file encryption key from the user
// password. Same logic as encrypt_parser's DeriveFileKey but
// takes the /O bytes directly (rather than via EncryptDict)
// because Algorithm 3 must run FIRST in the writer's dispatch
// to produce /O before this function is called.
std::vector<std::byte> DeriveFileKey(
        std::span<const std::byte> user_password,
        std::span<const std::byte> o_bytes,
        std::int32_t p,
        std::span<const std::byte> file_id,
        int r,
        bool encrypt_metadata,
        int length_bits) {
    const std::size_t n = static_cast<std::size_t>(length_bits / 8);

    std::vector<std::byte> buf = PadPassword(user_password);
    AppendBytes(buf, o_bytes);
    AppendP(buf, p);
    AppendBytes(buf, file_id);
    if (r >= 4 && !encrypt_metadata) {
        const std::uint8_t ff[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        AppendRaw(buf, ff, 4);
    }

    auto k = digest::Md5(buf);
    if (r >= 3) {
        for (int i = 0; i < 50; ++i) {
            std::span<const std::byte> head(k.data(), n);
            k = digest::Md5(head);
        }
    }
    k.resize(n);
    return k;
}

// Algorithm 2.B (PDF 2.0 §7.6.4.3.3) — iterative hash. Verbatim
// duplicate of encrypt_parser's Algorithm2B.
std::vector<std::byte> Algorithm2B(
        std::span<const std::byte> password,
        std::span<const std::byte> salt,
        std::span<const std::byte> u_extra) {
    std::vector<std::byte> seed;
    seed.reserve(password.size() + salt.size() + u_extra.size());
    AppendBytes(seed, password);
    AppendBytes(seed, salt);
    AppendBytes(seed, u_extra);
    std::vector<std::byte> k = digest::Sha256(seed);

    int round = 0;
    while (true) {
        const std::size_t one_chunk =
            password.size() + k.size() + u_extra.size();
        std::vector<std::byte> k1;
        k1.reserve(one_chunk * 64);
        for (int rep = 0; rep < 64; ++rep) {
            AppendBytes(k1, password);
            AppendBytes(k1, std::span<const std::byte>(k));
            AppendBytes(k1, u_extra);
        }

        std::span<const std::byte> aes_key(k.data(), 16);
        std::span<const std::byte> aes_iv(k.data() + 16, 16);
        std::vector<std::byte> e = aes_cbc::EncryptNoPad(
            aes_key, aes_iv, k1);

        unsigned int sum_mod3 = 0;
        for (std::size_t i = 0; i < 16; ++i) {
            sum_mod3 += static_cast<unsigned int>(
                static_cast<std::uint8_t>(e[i]));
        }
        const unsigned int mod3 = sum_mod3 % 3;

        if (mod3 == 0)      k = digest::Sha256(e);
        else if (mod3 == 1) k = digest::Sha384(e);
        else                k = digest::Sha512(e);

        ++round;

        const std::uint8_t last_e = static_cast<std::uint8_t>(
            e[e.size() - 1]);
        if (round >= 64 &&
                last_e <= static_cast<unsigned int>(round - 32)) {
            break;
        }
    }

    k.resize(32);
    return k;
}

// V=5 password→hash dispatch — single SHA-256 for R=5; Algorithm
// 2.B for R=6. Verbatim duplicate of encrypt_parser's ComputeV5Hash.
std::vector<std::byte> ComputeV5Hash(
        int r,
        std::span<const std::byte> password,
        std::span<const std::byte> salt,
        std::span<const std::byte> u_extra) {
    if (r == 5) {
        std::vector<std::byte> seed;
        seed.reserve(password.size() + salt.size()
                     + u_extra.size());
        AppendBytes(seed, password);
        AppendBytes(seed, salt);
        AppendBytes(seed, u_extra);
        return digest::Sha256(seed);
    }
    return Algorithm2B(password, salt, u_extra);
}

std::vector<std::byte> TruncatePassword(
        std::span<const std::byte> password) {
    const std::size_t take = std::min<std::size_t>(
        password.size(), 127);
    return std::vector<std::byte>(
        password.begin(), password.begin() + take);
}

// =========================================================
// Write-side algorithms. NEW logic (not in encrypt_parser).
// =========================================================

// Algorithm 3 — compute /O.
//   1. Pad owner_password to 32 bytes (or pad user_password if
//      owner_password is empty).
//   2. MD5 the padded password → 16 bytes.
//   3. R ≥ 3: hash 50 more times.
//   4. First length_bits/8 bytes = RC4 key K.
//   5. Pad user_password to 32 bytes.
//   6. R=2:  RC4-encrypt(K, padded_user_pwd) → 32 bytes = /O.
//      R≥3:  20-round outer loop. Round n (n = 0..19) uses
//            key K XOR n (each byte of K XOR'd with n) and
//            RC4-encrypts the previous output (round 0 uses
//            the padded user password as input). /O = output
//            of round 19.
std::vector<std::byte> ComputeO(
        std::span<const std::byte> user_password,
        std::span<const std::byte> owner_password,
        int r,
        int length_bits) {
    const std::size_t n = static_cast<std::size_t>(length_bits / 8);

    // Step 1: padded owner (or user if owner empty).
    auto padded_owner = PadPassword(
        owner_password.empty() ? user_password : owner_password);

    // Step 2-3: MD5(padded_owner), then iterate 50 more times
    // for R ≥ 3.
    auto k_full = digest::Md5(padded_owner);
    if (r >= 3) {
        for (int i = 0; i < 50; ++i) {
            std::span<const std::byte> head(k_full.data(), n);
            k_full = digest::Md5(head);
        }
    }

    // Step 4: K = first n bytes (RC4 key).
    std::vector<std::byte> k(k_full.begin(), k_full.begin() + n);

    // Step 5: padded user password.
    auto padded_user = PadPassword(user_password);

    // Step 6: R=2 → single RC4 pass; R≥3 → 20 forward rounds
    // with key K XOR counter.
    if (r == 2) {
        return rc4::Apply(k, padded_user);
    }

    std::vector<std::byte> xor_key(n);
    std::vector<std::byte> current = padded_user;
    for (int round = 0; round < 20; ++round) {
        for (std::size_t j = 0; j < n; ++j) {
            xor_key[j] = static_cast<std::byte>(
                static_cast<std::uint8_t>(k[j])
                    ^ static_cast<std::uint8_t>(round));
        }
        current = rc4::Apply(xor_key, current);
    }
    return current;
}

// Algorithm 4 — /U for R=2. RC4-encrypt the 32-byte standard
// padding string with file_key. Output is 32 bytes — that's /U
// in its entirety; no salt trailer needed.
std::vector<std::byte> ComputeU_R2(
        std::span<const std::byte> file_key) {
    std::vector<std::byte> padding_bytes(32);
    for (std::size_t i = 0; i < 32; ++i) {
        padding_bytes[i] = static_cast<std::byte>(kPadding[i]);
    }
    return rc4::Apply(file_key, padding_bytes);
}

// Algorithm 5 — /U for R ≥ 3. First 16 bytes are deterministic
// (MD5 of padding || file_id, then 20 RC4 rounds with key
// XOR'd by counter); trailing 16 bytes are caller-supplied salt
// (Algorithm 5 step 6: "append 16 bytes of arbitrary padding").
std::vector<std::byte> ComputeU_R3Plus(
        std::span<const std::byte> file_key,
        std::span<const std::byte> file_id,
        std::span<const std::byte> salt_16) {
    // Steps 1-3: H = MD5(padding || file_id), 16 bytes.
    std::vector<std::byte> hbuf;
    hbuf.reserve(48);
    AppendRaw(hbuf, kPadding.data(), 32);
    AppendBytes(hbuf, file_id);
    auto h = digest::Md5(hbuf);

    // Step 4: encrypt H with RC4 using key = file_key.
    std::vector<std::byte> enc_h = rc4::Apply(file_key, h);

    // Step 5: 19 more RC4 rounds. Round i (i = 1..19) XORs every
    // byte of file_key with i.
    std::vector<std::byte> xor_key(file_key.size());
    for (int i = 1; i <= 19; ++i) {
        for (std::size_t j = 0; j < file_key.size(); ++j) {
            xor_key[j] = static_cast<std::byte>(
                static_cast<std::uint8_t>(file_key[j])
                    ^ static_cast<std::uint8_t>(i));
        }
        enc_h = rc4::Apply(xor_key, enc_h);
    }

    // Step 6: 16-byte deterministic head || 16-byte salt trailer.
    std::vector<std::byte> u(32);
    std::copy(enc_h.begin(), enc_h.begin() + 16, u.begin());
    std::copy(salt_16.begin(), salt_16.begin() + 16,
              u.begin() + 16);
    return u;
}

// Algorithms 3.7 (R=5) / 8 (R=6) — compute /U.
//   /U = validation_hash (32 bytes)
//        || user_validation_salt (8 bytes)
//        || user_key_salt        (8 bytes).
// validation_hash uses ComputeV5Hash with no `u_extra` input.
std::vector<std::byte> ComputeU_R5OrR6(
        int r,
        std::span<const std::byte> user_password,
        std::span<const std::byte> user_val_salt,
        std::span<const std::byte> user_key_salt) {
    auto pwd = TruncatePassword(user_password);
    auto val_hash = ComputeV5Hash(
        r, pwd, user_val_salt,
        std::span<const std::byte>{});
    std::vector<std::byte> u;
    u.reserve(48);
    AppendBytes(u, val_hash);
    AppendBytes(u, user_val_salt);
    AppendBytes(u, user_key_salt);
    return u;
}

// Algorithms 3.8 (R=5) / 8 (R=6) — compute /UE.
//   intermediate_key = ComputeV5Hash(user_password,
//                                     user_key_salt, empty).
//   /UE = AES-256-CBC-ENCRYPT(file_key) under intermediate_key
//         with ZERO 16-byte IV and NO PADDING. 32 bytes.
std::vector<std::byte> ComputeUE_R5OrR6(
        int r,
        std::span<const std::byte> user_password,
        std::span<const std::byte> user_key_salt,
        std::span<const std::byte> file_key) {
    auto pwd = TruncatePassword(user_password);
    auto inter = ComputeV5Hash(
        r, pwd, user_key_salt,
        std::span<const std::byte>{});
    std::array<std::byte, 16> zero_iv{};
    return aes_cbc::EncryptNoPad(
        std::span<const std::byte>(inter),
        std::span<const std::byte>(zero_iv),
        file_key);
}

// Algorithms 3.9 (R=5) / 9 (R=6) — compute /O.
//   /O = validation_hash (32 bytes)
//        || owner_validation_salt (8 bytes)
//        || owner_key_salt        (8 bytes).
// validation_hash uses ComputeV5Hash with /U as the u_extra
// input.
std::vector<std::byte> ComputeO_R5OrR6(
        int r,
        std::span<const std::byte> owner_password,
        std::span<const std::byte> owner_val_salt,
        std::span<const std::byte> owner_key_salt,
        std::span<const std::byte> u_48) {
    auto pwd = TruncatePassword(
        owner_password.empty() ? std::span<const std::byte>{}
                                : owner_password);
    auto val_hash = ComputeV5Hash(
        r, pwd, owner_val_salt, u_48);
    std::vector<std::byte> o;
    o.reserve(48);
    AppendBytes(o, val_hash);
    AppendBytes(o, owner_val_salt);
    AppendBytes(o, owner_key_salt);
    return o;
}

// Algorithm 3.10 (R=5) / 9 (R=6) — compute /OE.
//   intermediate_key = ComputeV5Hash(owner_password,
//                                     owner_key_salt, /U).
//   /OE = AES-256-CBC-ENCRYPT(file_key) under intermediate_key
//         with ZERO IV and NO PADDING. 32 bytes.
std::vector<std::byte> ComputeOE_R5OrR6(
        int r,
        std::span<const std::byte> owner_password,
        std::span<const std::byte> owner_key_salt,
        std::span<const std::byte> u_48,
        std::span<const std::byte> file_key) {
    auto pwd = TruncatePassword(
        owner_password.empty() ? std::span<const std::byte>{}
                                : owner_password);
    auto inter = ComputeV5Hash(r, pwd, owner_key_salt, u_48);
    std::array<std::byte, 16> zero_iv{};
    return aes_cbc::EncryptNoPad(
        std::span<const std::byte>(inter),
        std::span<const std::byte>(zero_iv),
        file_key);
}

// Algorithm 10 — compute /Perms (R ≥ 5).
//   plaintext = P_LE (4 bytes)
//               || 0xFFFFFFFF (4 bytes)
//               || 'T' or 'F' (1 byte; encrypt_metadata flag)
//               || 'a','d','b' (3 bytes)
//               || perms_tail (4 bytes, caller-supplied random
//                  anti-replay).
//   /Perms = AES-256-CBC-ENCRYPT-no-padding(plaintext) under
//            file_key with ZERO IV. 16 bytes.
//   (AES-256-CBC with zero IV on a single block == AES-256-ECB.)
std::vector<std::byte> ComputePerms(
        std::int32_t p,
        bool encrypt_metadata,
        std::span<const std::byte> perms_tail,
        std::span<const std::byte> file_key) {
    if (perms_tail.size() != 4) {
        throw std::runtime_error(
            "encrypt_writer: perms_tail must be 4 bytes");
    }
    if (file_key.size() != 32) {
        throw std::runtime_error(
            "encrypt_writer: file_key must be 32 bytes for /Perms");
    }
    std::array<std::byte, 16> plain{};
    const auto u = static_cast<std::uint32_t>(p);
    plain[0] = static_cast<std::byte>(u & 0xFF);
    plain[1] = static_cast<std::byte>((u >> 8) & 0xFF);
    plain[2] = static_cast<std::byte>((u >> 16) & 0xFF);
    plain[3] = static_cast<std::byte>((u >> 24) & 0xFF);
    plain[4] = static_cast<std::byte>(0xFF);
    plain[5] = static_cast<std::byte>(0xFF);
    plain[6] = static_cast<std::byte>(0xFF);
    plain[7] = static_cast<std::byte>(0xFF);
    plain[8] = static_cast<std::byte>(
        encrypt_metadata ? 'T' : 'F');
    plain[9] = static_cast<std::byte>('a');
    plain[10] = static_cast<std::byte>('d');
    plain[11] = static_cast<std::byte>('b');
    plain[12] = perms_tail[0];
    plain[13] = perms_tail[1];
    plain[14] = perms_tail[2];
    plain[15] = perms_tail[3];

    std::array<std::byte, 16> zero_iv{};
    return aes_cbc::EncryptNoPad(
        file_key,
        std::span<const std::byte>(zero_iv),
        std::span<const std::byte>(plain));
}

// =========================================================
// Dispatch — per-(V, R) variant.
// =========================================================

EncryptOutput BuildR_lt5(
        std::span<const std::byte> user_password,
        std::span<const std::byte> owner_password,
        std::int32_t p,
        int v,
        int r,
        int length_bits,
        std::span<const std::byte> file_id,
        bool encrypt_metadata,
        std::span<const std::byte> salt) {
    if (file_id.empty()) {
        throw std::runtime_error(
            "encrypt_writer: file_id required for R<5");
    }

    // Algorithm 3 must run BEFORE Algorithm 2 (Algorithm 2 step
    // 3 consumes /O).
    auto o = ComputeO(user_password, owner_password, r, length_bits);

    // Algorithm 2 — file key.
    auto file_key = DeriveFileKey(
        user_password, o, p, file_id, r,
        encrypt_metadata, length_bits);

    // Algorithm 4 / 5 — /U.
    std::vector<std::byte> u;
    if (r == 2) {
        u = ComputeU_R2(file_key);
    } else {
        u = ComputeU_R3Plus(file_key, file_id, salt);
    }

    EncryptDict ed;
    ed.v = v;
    ed.r = r;
    ed.length_bits = length_bits;
    ed.p = p;
    ed.o = std::move(o);
    ed.u = std::move(u);
    ed.oe = {};
    ed.ue = {};
    ed.perms = {};
    ed.encrypt_metadata = encrypt_metadata;
    return EncryptOutput{std::move(ed), std::move(file_key)};
}

EncryptOutput BuildR_ge5(
        std::span<const std::byte> user_password,
        std::span<const std::byte> owner_password,
        std::int32_t p,
        int v,
        int r,
        int length_bits,
        bool encrypt_metadata,
        std::span<const std::byte> salt) {
    // Salt layout: 4 × 8-byte salts + 32-byte file_key + 4-byte
    // perms_tail.
    std::span<const std::byte> user_val_salt(salt.data(), 8);
    std::span<const std::byte> user_key_salt(salt.data() + 8, 8);
    std::span<const std::byte> owner_val_salt(salt.data() + 16, 8);
    std::span<const std::byte> owner_key_salt(salt.data() + 24, 8);
    std::span<const std::byte> file_key(salt.data() + 32, 32);
    std::span<const std::byte> perms_tail(salt.data() + 64, 4);

    // Algorithm 3.7 / 8 — /U.
    auto u = ComputeU_R5OrR6(
        r, user_password, user_val_salt, user_key_salt);

    // Algorithm 3.8 / 8 — /UE.
    auto ue = ComputeUE_R5OrR6(
        r, user_password, user_key_salt, file_key);

    // Algorithm 3.9 / 9 — /O. Takes /U as u_extra.
    auto o = ComputeO_R5OrR6(
        r, owner_password, owner_val_salt, owner_key_salt, u);

    // Algorithm 3.10 / 9 — /OE.
    auto oe = ComputeOE_R5OrR6(
        r, owner_password, owner_key_salt, u, file_key);

    // Algorithm 10 — /Perms.
    auto perms = ComputePerms(
        p, encrypt_metadata, perms_tail, file_key);

    EncryptDict ed;
    ed.v = v;
    ed.r = r;
    ed.length_bits = length_bits;
    ed.p = p;
    ed.o = std::move(o);
    ed.u = std::move(u);
    ed.oe = std::move(oe);
    ed.ue = std::move(ue);
    ed.perms = std::move(perms);
    ed.encrypt_metadata = encrypt_metadata;
    return EncryptOutput{
        std::move(ed),
        std::vector<std::byte>(file_key.begin(), file_key.end())};
}

}  // namespace

EncryptOutput BuildEncryptDict(
        std::span<const std::byte> user_password,
        std::span<const std::byte> owner_password,
        std::int32_t p,
        int v,
        int r,
        int length_bits,
        std::span<const std::byte> file_id,
        bool encrypt_metadata,
        std::span<const std::byte> salt) {
    // Validate (v, r) combination and length_bits.
    const bool valid_combo =
        (v == 1 && r == 2 && length_bits == 40)
        || (v == 2 && r == 3 && length_bits >= 40
                              && length_bits <= 128
                              && length_bits % 8 == 0)
        || (v == 4 && r == 4 && length_bits == 128)
        || (v == 5 && r == 5 && length_bits == 256)
        || (v == 5 && r == 6 && length_bits == 256);
    if (!valid_combo) {
        throw std::runtime_error(
            "encrypt_writer: unsupported (V, R, length_bits) "
            "combination — supported are (1,2,40), (2,3,40..128 "
            "in multiples of 8), (4,4,128), (5,5,256), (5,6,256)");
    }

    // Validate salt size.
    const std::size_t expected_salt =
        (r < 5) ? 16 : 68;
    if (salt.size() != expected_salt) {
        throw std::runtime_error(
            r < 5
                ? "encrypt_writer: salt must be 16 bytes for R<5"
                : "encrypt_writer: salt must be 68 bytes for R>=5 "
                  "(32 layout salts + 32 file_key + 4 perms_tail)");
    }

    if (r < 5) {
        return BuildR_lt5(
            user_password, owner_password, p, v, r,
            length_bits, file_id, encrypt_metadata, salt);
    }
    return BuildR_ge5(
        user_password, owner_password, p, v, r,
        length_bits, encrypt_metadata, salt);
}

}  // namespace foundation::encrypt_writer
