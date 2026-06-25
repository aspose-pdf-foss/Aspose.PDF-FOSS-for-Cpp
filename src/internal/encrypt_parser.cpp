// PDF /Encrypt authentication + key derivation for the Standard
// security handler. Supports:
//   * V=1 R=2  — 40-bit RC4
//   * V=2 R=3  — variable-length RC4 (40..128, multiple of 8)
//   * V=4 R=4  — AES-128
//   * V=5 R=5  — AES-256 (deprecated single-pass SHA-256 path)
//   * V=5 R=6  — AES-256 (PDF 2.0 SHA-2 / AES-128 cascade)
// Implements:
//   * Algorithm 2   (V<5 file encryption key from password)
//   * Algorithm 2.A (V=5  file encryption key — wraps 2.B for R=6,
//                   single SHA-256 for R=5)
//   * Algorithm 2.B (R=6 iterative hashing — SHA-256/384/512 +
//                   AES-128-CBC, round-counted via last-byte rule)
//   * Algorithm 4   (compute /U for R=2)
//   * Algorithm 5   (compute /U for R≥3)
//   * Algorithm 6   (test V<5 user password)
//   * Algorithm 7   (test V<5 owner password — recover user
//                   password via RC4-decrypt of /O, then run
//                   Algorithm 6)
//   * Algorithm 11  (test V=5 user password against /U[0:32])
//   * Algorithm 12  (test V=5 owner password against /O[0:32])
// Spec: PDF 32000-1:2008 §7.6.3.3 / §7.6.3.4 (V<5);
//       ISO 32000-2:2020 §7.6.4.3 (V=5).
//
// Reuses foundation primitives:
//   * digest::Md5/Sha256/Sha384/Sha512 for the hash cascades
//   * rc4::Apply         for RC4 rounds in Algorithms 4 / 5 / 7
//   * aes_cbc::EncryptNoPad / DecryptNoPad for raw (no-padding)
//     AES-CBC blocks in Algorithm 2.A's /UE / /OE recovery and
//     Algorithm 2.B's inner cipher round. The PKCS#7 Encrypt /
//     Decrypt entry points cannot be used here because V=5's
//     /UE / /OE / Perms are exactly block-aligned ciphertext
//     with no padding byte to strip.

#include "encrypt_parser.hpp"

#include "aes_cbc.hpp"
#include "digest.hpp"
#include "rc4.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace foundation::encrypt_parser {

namespace {

// Standard 32-byte padding string from PDF §7.6.3.3 — used to
// pad short passwords to exactly 32 bytes. The literal is
// intentionally fixed; do not reformat or shorten.
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

// Algorithm 2 step 4: append /P as a 4-byte little-endian
// signed integer.
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

// Algorithm 2 — derive the file encryption key from a
// (possibly user, possibly owner-recovered) password.
// Returns a key of length_bits/8 bytes (16 for V=4 R=4).
std::vector<std::byte> DeriveFileKey(
        const EncryptDict& enc,
        std::span<const std::byte> password,
        std::span<const std::byte> file_id) {
    const std::size_t n = static_cast<std::size_t>(
        enc.length_bits / 8);

    // Step 1+2: padded password.
    std::vector<std::byte> buf = PadPassword(password);
    // Step 3: append /O.
    AppendBytes(buf, enc.o);
    // Step 4: append /P (4 bytes LE signed).
    AppendP(buf, enc.p);
    // Step 5: append the file_id (the first /ID element).
    AppendBytes(buf, file_id);
    // Step 6: if R ≥ 4 and /EncryptMetadata == false, append
    // 0xFFFFFFFF.
    if (enc.r >= 4 && !enc.encrypt_metadata) {
        const std::uint8_t ff[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        AppendRaw(buf, ff, 4);
    }

    // Step 7: K = MD5(buf).
    auto k = digest::Md5(buf);
    // Step 8 (R ≥ 3): repeat 50 times K = first n bytes of MD5(K).
    if (enc.r >= 3) {
        for (int i = 0; i < 50; ++i) {
            // Hash only the first n bytes of K — the leading n
            // bytes carry the file-key material; trailing bytes
            // are residue from the previous round and ignored.
            std::span<const std::byte> head(k.data(), n);
            k = digest::Md5(head);
        }
    }
    // Step 9: file key = first n bytes of K.
    k.resize(n);
    return k;
}

// Algorithm 4 — compute the expected /U value for R = 2 given
// a candidate file_key. RC4-encrypt the standard 32-byte
// padding string using the file_key as the RC4 key. The full
// 32-byte result must equal /U for the password to be correct.
std::vector<std::byte> ComputeUForR2(
        std::span<const std::byte> file_key) {
    std::vector<std::byte> padding_bytes(32);
    for (std::size_t i = 0; i < 32; ++i) {
        padding_bytes[i] = static_cast<std::byte>(kPadding[i]);
    }
    return rc4::Apply(file_key, padding_bytes);
}

// Algorithm 5 — compute the expected /U value for R ≥ 3 given
// a candidate file_key. Returns the 32-byte test value; the
// first 16 bytes equal /U[0:16] when the password is correct.
std::vector<std::byte> ComputeUForR3Plus(
        std::span<const std::byte> file_key,
        std::span<const std::byte> file_id) {
    // Steps 1-3: H = MD5(padding || file_id), 16 bytes.
    std::vector<std::byte> hbuf;
    hbuf.reserve(48);
    AppendRaw(hbuf, kPadding.data(), 32);
    AppendBytes(hbuf, file_id);
    auto h = digest::Md5(hbuf);

    // Step 4: encrypt H with RC4 using key=file_key.
    std::vector<std::byte> enc_h = rc4::Apply(file_key, h);

    // Step 5: 19 more rounds. Round i (1..19) XORs every byte
    // of file_key with i, runs RC4 on the previous result.
    std::vector<std::byte> xor_key(file_key.size());
    for (int i = 1; i <= 19; ++i) {
        for (std::size_t j = 0; j < file_key.size(); ++j) {
            xor_key[j] = static_cast<std::byte>(
                static_cast<std::uint8_t>(file_key[j])
                    ^ static_cast<std::uint8_t>(i));
        }
        enc_h = rc4::Apply(xor_key, enc_h);
    }

    // Step 6: append 16 bytes of arbitrary padding to produce
    // a 32-byte /U-test. Spec says the trailer is undefined;
    // qpdf and Acrobat both pad with the standard padding's
    // first 16 bytes — and PDF readers compare only the first
    // 16 bytes anyway, so the trailing 16 bytes don't matter
    // for authentication.
    std::vector<std::byte> u_test = std::move(enc_h);
    u_test.resize(32);
    return u_test;
}

// Compare the first N bytes of two byte spans.
bool FirstNMatch(std::span<const std::byte> a,
                 std::span<const std::byte> b,
                 std::size_t n) {
    if (a.size() < n || b.size() < n) return false;
    return std::memcmp(a.data(), b.data(), n) == 0;
}

// Compute the expected /U test value for the given file_key.
// R=2 uses Algorithm 4 (full 32-byte match required); R≥3 uses
// Algorithm 5 (first-16 match required — the trailing 16 bytes
// of /U are arbitrary padding per spec).
std::vector<std::byte> ComputeUTest(
        const EncryptDict& enc,
        std::span<const std::byte> file_key,
        std::span<const std::byte> file_id) {
    if (enc.r == 2) return ComputeUForR2(file_key);
    return ComputeUForR3Plus(file_key, file_id);
}

// Match width depends on R: R=2 compares all 32 bytes; R≥3
// compares only the first 16 (Algorithm 6 step 2).
std::size_t UMatchWidth(int r) { return r == 2 ? 32 : 16; }

// =========================================================
// V=5 helpers (Algorithms 2.A, 2.B, 11, 12).
// =========================================================

// Algorithm 2.B (ISO 32000-2 §7.6.4.3.4) — iterative hash for
// V=5 R≥6.
//   K0 = SHA-256(password || salt || u_extra)
//   loop:
//     K1 = (password || K || u_extra) repeated 64 times
//     E  = AES-128-CBC(key=K[0:16], IV=K[16:32], data=K1, no_pad)
//     mod3 = (first 16 bytes of E as big-endian unsigned int)
//            mod 3
//          == sum-of-bytes mod 3 (because 256 ≡ 1 (mod 3))
//     K  = SHA-256(E) | SHA-384(E) | SHA-512(E)  for mod3 = 0|1|2
//     round_count++
//     stop when round_count >= 64 AND last byte of E ≤
//                                       round_count − 32
//   return first 32 bytes of K
std::vector<std::byte> Algorithm2B(
        std::span<const std::byte> password,
        std::span<const std::byte> salt,
        std::span<const std::byte> u_extra) {
    // K0.
    std::vector<std::byte> seed;
    seed.reserve(password.size() + salt.size() + u_extra.size());
    AppendBytes(seed, password);
    AppendBytes(seed, salt);
    AppendBytes(seed, u_extra);
    std::vector<std::byte> k = digest::Sha256(seed);

    int round = 0;
    while (true) {
        // K1 = (password || K || u_extra) repeated 64 times.
        const std::size_t one_chunk =
            password.size() + k.size() + u_extra.size();
        std::vector<std::byte> k1;
        k1.reserve(one_chunk * 64);
        for (int rep = 0; rep < 64; ++rep) {
            AppendBytes(k1, password);
            AppendBytes(k1, std::span<const std::byte>(k));
            AppendBytes(k1, u_extra);
        }

        // E = AES-128-CBC(key=K[0:16], IV=K[16:32], data=K1).
        std::span<const std::byte> aes_key(k.data(), 16);
        std::span<const std::byte> aes_iv(k.data() + 16, 16);
        std::vector<std::byte> e = aes_cbc::EncryptNoPad(
            aes_key, aes_iv, k1);

        // mod3 of (first 16 bytes of E as big-endian unsigned int).
        // 256 ≡ 1 (mod 3), so int mod 3 == sum-of-bytes mod 3.
        unsigned int sum_mod3 = 0;
        for (std::size_t i = 0; i < 16; ++i) {
            sum_mod3 += static_cast<unsigned int>(
                static_cast<std::uint8_t>(e[i]));
        }
        const unsigned int mod3 = sum_mod3 % 3;

        // Next K from a SHA-2 over the full E.
        if (mod3 == 0)      k = digest::Sha256(e);
        else if (mod3 == 1) k = digest::Sha384(e);
        else                k = digest::Sha512(e);

        ++round;

        // Stop condition.
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

// V=5 password→hash dispatch. R=5 is single SHA-256(password ||
// salt || u_extra); R=6 is Algorithm 2.B over the same inputs.
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

// Slice helper.
std::span<const std::byte> Slice(
        const std::vector<std::byte>& buf,
        std::size_t off, std::size_t n) {
    return std::span<const std::byte>(buf.data() + off, n);
}

// Truncate a password to 127 bytes per spec (Algorithm 2.A
// step 1). qpdf's fixtures use short ASCII passwords; the
// truncation is preserved here for forward compatibility with
// pre-processed UTF-8 inputs.
std::vector<std::byte> TruncatePassword(
        std::span<const std::byte> password) {
    const std::size_t take = std::min<std::size_t>(
        password.size(), 127);
    return std::vector<std::byte>(
        password.begin(), password.begin() + take);
}

// V=5 user-password authentication — Algorithm 11 + Algorithm
// 2.A user branch. Returns {true, file_key} on match; {false, {}}
// otherwise.
AuthResult AuthenticateV5User(
        const EncryptDict& enc,
        std::span<const std::byte> password) {
    auto pwd = TruncatePassword(password);

    // Algorithm 11 — verify against /U[0:32] using validation
    // salt /U[32:40].
    auto val_hash = ComputeV5Hash(
        enc.r, pwd, Slice(enc.u, 32, 8),
        std::span<const std::byte>{});
    if (!std::equal(val_hash.begin(),
                    val_hash.begin() + 32,
                    enc.u.begin())) {
        return {false, {}, false};
    }

    // Algorithm 2.A user branch — derive intermediate key from
    // key salt /U[40:48], then AES-256-CBC-decrypt /UE (IV=0,
    // no padding) to recover the 32-byte file_key.
    auto inter = ComputeV5Hash(
        enc.r, pwd, Slice(enc.u, 40, 8),
        std::span<const std::byte>{});
    std::array<std::byte, 16> zero_iv{};
    std::vector<std::byte> file_key = aes_cbc::DecryptNoPad(
        std::span<const std::byte>(inter),
        std::span<const std::byte>(zero_iv),
        std::span<const std::byte>(enc.ue));

    return {true, std::move(file_key), false};
}

// V=5 owner-password authentication — Algorithm 12 + Algorithm
// 2.A owner branch.
AuthResult AuthenticateV5Owner(
        const EncryptDict& enc,
        std::span<const std::byte> password) {
    auto pwd = TruncatePassword(password);

    // Algorithm 12 — verify against /O[0:32] using validation
    // salt /O[32:40] AND /U[0:48] as the trailing input.
    auto val_hash = ComputeV5Hash(
        enc.r, pwd, Slice(enc.o, 32, 8),
        std::span<const std::byte>(enc.u.data(), 48));
    if (!std::equal(val_hash.begin(),
                    val_hash.begin() + 32,
                    enc.o.begin())) {
        return {false, {}, false};
    }

    // Algorithm 2.A owner branch — derive intermediate key from
    // key salt /O[40:48] AND /U[0:48], then AES-256-CBC-decrypt
    // /OE.
    auto inter = ComputeV5Hash(
        enc.r, pwd, Slice(enc.o, 40, 8),
        std::span<const std::byte>(enc.u.data(), 48));
    std::array<std::byte, 16> zero_iv{};
    std::vector<std::byte> file_key = aes_cbc::DecryptNoPad(
        std::span<const std::byte>(inter),
        std::span<const std::byte>(zero_iv),
        std::span<const std::byte>(enc.oe));

    return {true, std::move(file_key), true};
}

// Algorithm 7 — recover the user password from /O given the
// owner password. Returns the (still padded) 32-byte user
// password on success, or an empty vector on cipher failure.
std::vector<std::byte> RecoverUserPasswordFromOwner(
        const EncryptDict& enc,
        std::span<const std::byte> owner_pwd) {
    // Step 1: pad the owner password to 32 bytes.
    auto padded = PadPassword(owner_pwd);
    // Step 2: K = MD5(padded). 50 rounds for R ≥ 3.
    auto k = digest::Md5(padded);
    if (enc.r >= 3) {
        const std::size_t n = static_cast<std::size_t>(
            enc.length_bits / 8);
        for (int i = 0; i < 50; ++i) {
            std::span<const std::byte> head(k.data(), n);
            k = digest::Md5(head);
        }
    }
    // Step 3: derive RC4 key = first n bytes of K (only the
    // length_bits/8 bytes participate per spec).
    const std::size_t n = static_cast<std::size_t>(
        enc.length_bits / 8);
    std::vector<std::byte> key(k.begin(), k.begin() + n);

    // Step 4: RC4-decrypt /O. For R≥3, run 20 rounds with
    // descending iteration counter (19 .. 0) XOR'd into key.
    std::vector<std::byte> enc_o(enc.o.begin(), enc.o.end());
    if (enc.r == 2) {
        enc_o = rc4::Apply(key, enc_o);
    } else {
        std::vector<std::byte> xor_key(n);
        for (int i = 19; i >= 0; --i) {
            for (std::size_t j = 0; j < n; ++j) {
                xor_key[j] = static_cast<std::byte>(
                    static_cast<std::uint8_t>(key[j])
                        ^ static_cast<std::uint8_t>(i));
            }
            enc_o = rc4::Apply(xor_key, enc_o);
        }
    }
    // The decrypted /O is the (padded) user password.
    return enc_o;
}

}  // namespace

// Pad/truncate a password to 32 bytes per Algorithm 2 step 1 (PDF
// §7.6.3.3): the first 32 password bytes, then the standard padding string.
std::vector<std::byte> PadPassword(std::span<const std::byte> pwd) {
    const std::size_t take = std::min<std::size_t>(pwd.size(), 32);
    std::vector<std::byte> out(pwd.begin(), pwd.begin() + take);
    out.reserve(32);
    for (std::size_t i = take; i < 32; ++i) {
        out.push_back(static_cast<std::byte>(kPadding[i - take]));
    }
    return out;
}

AuthResult Authenticate(
        const EncryptDict& enc,
        std::span<const std::byte> password,
        std::span<const std::byte> file_id) {
    const bool valid_combo =
        (enc.v == 1 && enc.r == 2) ||
        (enc.v == 2 && enc.r == 3) ||
        (enc.v == 4 && enc.r == 4) ||
        (enc.v == 5 && (enc.r == 5 || enc.r == 6));
    if (!valid_combo) {
        throw std::runtime_error(
            "encrypt_parser: unsupported (V, R) — supported are "
            "(1,2), (2,3), (4,4), (5,5), (5,6)");
    }
    const bool valid_length =
        (enc.v == 1 && enc.length_bits == 40) ||
        (enc.v == 2 && enc.length_bits >= 40
                    && enc.length_bits <= 128
                    && (enc.length_bits % 8) == 0) ||
        (enc.v == 4 && enc.length_bits == 128) ||
        (enc.v == 5 && enc.length_bits == 256);
    if (!valid_length) {
        throw std::runtime_error(
            "encrypt_parser: invalid length_bits for (V, R)");
    }
    if (enc.v == 5) {
        if (enc.o.size() != 48 || enc.u.size() != 48) {
            throw std::runtime_error(
                "encrypt_parser: /O and /U must be 48 bytes for "
                "V=5");
        }
        if (enc.oe.size() != 32 || enc.ue.size() != 32) {
            throw std::runtime_error(
                "encrypt_parser: /OE and /UE must be 32 bytes for "
                "V=5");
        }
        if (enc.perms.size() != 16) {
            throw std::runtime_error(
                "encrypt_parser: /Perms must be 16 bytes for V=5");
        }
    } else {
        if (enc.o.size() != 32 || enc.u.size() != 32) {
            throw std::runtime_error(
                "encrypt_parser: /O and /U must be 32 bytes");
        }
    }
    if (enc.v == 4 && file_id.empty()) {
        throw std::runtime_error(
            "encrypt_parser: file_id required (V=4 trailer must "
            "have /ID)");
    }

    if (enc.v == 5) {
        // V=5 uses Algorithms 11/12 + 2.A — the file_id is not
        // an input to V=5 key derivation.
        auto user = AuthenticateV5User(enc, password);
        if (user.authenticated) return user;
        auto owner = AuthenticateV5Owner(enc, password);
        if (owner.authenticated) return owner;
        return {false, {}, false};
    }

    const std::size_t match_n = UMatchWidth(enc.r);

    // Try as USER password first.
    {
        auto fk = DeriveFileKey(enc, password, file_id);
        auto u_test = ComputeUTest(enc, fk, file_id);
        if (FirstNMatch(u_test, enc.u, match_n)) {
            AuthResult ok;
            ok.authenticated = true;
            ok.file_key = std::move(fk);
            ok.used_owner_path = false;
            return ok;
        }
    }

    // Try as OWNER password — recover user password from /O,
    // then redo Algorithm 6 with that.
    {
        auto recovered_user = RecoverUserPasswordFromOwner(
            enc, password);
        auto fk = DeriveFileKey(enc, recovered_user, file_id);
        auto u_test = ComputeUTest(enc, fk, file_id);
        if (FirstNMatch(u_test, enc.u, match_n)) {
            AuthResult ok;
            ok.authenticated = true;
            ok.file_key = std::move(fk);
            ok.used_owner_path = true;
            return ok;
        }
    }

    AuthResult fail;
    fail.authenticated = false;
    fail.used_owner_path = false;
    return fail;
}

}  // namespace foundation::encrypt_parser
