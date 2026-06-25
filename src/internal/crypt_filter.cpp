// PDF /Encrypt application layer for one indirect-object payload.
// Composes foundation::rc4 / foundation::aes_cbc / foundation::digest
// to implement the three CFM dispatch paths (V2 / AESV2 / AESV3)
// per PDF 32000-1:2008 §7.6.2 Algorithm 1 + Algorithm 1a and
// PDF 32000-2:2020 §7.6.4.4. Per-object key derivation lives
// here; cipher-randomness (random IV per AES object) lives in
// the caller — Encrypt accepts iv as an explicit parameter so
// crypt_filter is fully deterministic given inputs.

#include "crypt_filter.hpp"

#include "aes_cbc.hpp"
#include "digest.hpp"
#include "rc4.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace foundation::crypt_filter {

namespace {

constexpr std::size_t kAesIvSize = 16;
constexpr std::size_t kMd5Size = 16;
constexpr std::size_t kAesV2KeyBytes = 16;
constexpr std::size_t kAesV3KeyBytes = 32;

constexpr std::uint8_t kMethodV2 = 1;
constexpr std::uint8_t kMethodAesV2 = 2;
constexpr std::uint8_t kMethodAesV3 = 3;

void ValidateKey(Method method, std::span<const std::byte> file_key) {
    switch (method) {
    case Method::V2:
        if (file_key.size() < 5 || file_key.size() > 16) {
            throw std::runtime_error(
                "crypt_filter: V2 file_key length must be 5..16");
        }
        return;
    case Method::AESV2:
        if (file_key.size() != kAesV2KeyBytes) {
            throw std::runtime_error(
                "crypt_filter: AESV2 file_key length must be 16");
        }
        return;
    case Method::AESV3:
        if (file_key.size() != kAesV3KeyBytes) {
            throw std::runtime_error(
                "crypt_filter: AESV3 file_key length must be 32");
        }
        return;
    }
    throw std::runtime_error("crypt_filter: unknown method");
}

// Algorithm 1 / 1a (PDF 32000-1 §7.6.2): per-object key
// derivation for V2 and AESV2. AESV3 SKIPS this routine —
// V=5 uses file_key directly per PDF 32000-2 §7.6.4.4.
std::vector<std::byte> DerivePerObjKey(
        Method method,
        std::span<const std::byte> file_key,
        std::uint32_t object_id,
        std::uint32_t generation) {
    const bool wants_salt = (method == Method::AESV2);
    const std::size_t n = file_key.size();
    std::vector<std::byte> buf;
    buf.reserve(n + 5 + (wants_salt ? 4 : 0));
    buf.insert(buf.end(), file_key.begin(), file_key.end());
    // Low 3 bytes of object_id, little-endian.
    buf.push_back(static_cast<std::byte>(object_id & 0xFF));
    buf.push_back(static_cast<std::byte>((object_id >> 8) & 0xFF));
    buf.push_back(static_cast<std::byte>((object_id >> 16) & 0xFF));
    // Low 2 bytes of generation, little-endian.
    buf.push_back(static_cast<std::byte>(generation & 0xFF));
    buf.push_back(static_cast<std::byte>((generation >> 8) & 0xFF));
    // sAlT salt for AESV2 — exact bytes 0x73 0x41 0x6C 0x54
    // ('s', 'A', 'l', 'T') per PDF 32000-1 §7.6.2 Algorithm 1a.
    if (wants_salt) {
        buf.push_back(static_cast<std::byte>(0x73));
        buf.push_back(static_cast<std::byte>(0x41));
        buf.push_back(static_cast<std::byte>(0x6C));
        buf.push_back(static_cast<std::byte>(0x54));
    }
    auto md5 = digest::Md5(buf);
    // Cap at 16 bytes (MD5 size) — for file_key shorter than
    // 11 bytes the per-object key is shorter than the MD5
    // output.
    const std::size_t key_len = std::min<std::size_t>(n + 5, kMd5Size);
    return std::vector<std::byte>(md5.begin(), md5.begin() + key_len);
}

}  // namespace

std::vector<std::byte> Encrypt(
        Method method,
        std::span<const std::byte> file_key,
        std::uint32_t object_id,
        std::uint32_t generation,
        std::span<const std::byte> plaintext,
        std::span<const std::byte> iv) {
    ValidateKey(method, file_key);

    if (method == Method::V2) {
        const auto per_obj_key =
            DerivePerObjKey(method, file_key, object_id, generation);
        return rc4::Apply(per_obj_key, plaintext);
    }

    if (iv.size() != kAesIvSize) {
        throw std::runtime_error(
            "crypt_filter: iv must be 16 bytes for AES variants");
    }

    std::vector<std::byte> derived;
    std::span<const std::byte> cipher_key;
    if (method == Method::AESV2) {
        derived = DerivePerObjKey(method, file_key, object_id, generation);
        cipher_key = derived;
    } else {  // AESV3 — file_key is used directly.
        cipher_key = file_key;
    }

    auto cipher = aes_cbc::Encrypt(cipher_key, iv, plaintext);
    std::vector<std::byte> out;
    out.reserve(kAesIvSize + cipher.size());
    out.insert(out.end(), iv.begin(), iv.end());
    out.insert(out.end(), cipher.begin(), cipher.end());
    return out;
}

std::vector<std::byte> Decrypt(
        Method method,
        std::span<const std::byte> file_key,
        std::uint32_t object_id,
        std::uint32_t generation,
        std::span<const std::byte> ciphertext) {
    ValidateKey(method, file_key);

    if (method == Method::V2) {
        const auto per_obj_key =
            DerivePerObjKey(method, file_key, object_id, generation);
        return rc4::Apply(per_obj_key, ciphertext);
    }

    if (ciphertext.size() < kAesIvSize) {
        throw std::runtime_error(
            "crypt_filter: AES ciphertext must be >= 16 bytes (iv)");
    }

    std::span<const std::byte> iv = ciphertext.subspan(0, kAesIvSize);
    std::span<const std::byte> body = ciphertext.subspan(kAesIvSize);

    std::vector<std::byte> derived;
    std::span<const std::byte> cipher_key;
    if (method == Method::AESV2) {
        derived = DerivePerObjKey(method, file_key, object_id, generation);
        cipher_key = derived;
    } else {  // AESV3
        cipher_key = file_key;
    }

    return aes_cbc::Decrypt(cipher_key, iv, body);
}

namespace {

Method MethodFromId(std::uint8_t id) {
    switch (id) {
    case kMethodV2:    return Method::V2;
    case kMethodAesV2: return Method::AESV2;
    case kMethodAesV3: return Method::AESV3;
    }
    throw std::runtime_error("crypt_filter: unknown wire-format method id");
}

// Wire format header parsed off the gate-only Encode/Decode
// input. Layout (LITTLE-endian):
//   byte 0       : method_id (1=V2, 2=AESV2, 3=AESV3)
//   byte 1       : key_len
//   bytes 2..2+kl: file_key
//   next 4 bytes : object_id (uint32 LE)
//   next 4 bytes : generation (uint32 LE)
//   next 16 bytes: iv (passed to Encrypt for AES; ignored for V2)
//   remainder    : payload
struct WireHeader {
    Method method;
    std::span<const std::byte> file_key;
    std::uint32_t object_id;
    std::uint32_t generation;
    std::span<const std::byte> iv;
    std::size_t payload_offset;
};

WireHeader ParseWire(std::span<const std::byte> input) {
    if (input.size() < 2) {
        throw std::runtime_error(
            "crypt_filter: wire input too short for method+key_len");
    }
    const auto method =
        MethodFromId(static_cast<std::uint8_t>(input[0]));
    const std::size_t key_len = static_cast<std::uint8_t>(input[1]);
    const std::size_t header_len = 2 + key_len + 4 + 4 + kAesIvSize;
    if (input.size() < header_len) {
        throw std::runtime_error(
            "crypt_filter: wire header truncated");
    }
    const auto file_key = input.subspan(2, key_len);
    const std::size_t off = 2 + key_len;
    const std::uint32_t object_id =
        static_cast<std::uint32_t>(input[off]) |
        (static_cast<std::uint32_t>(input[off + 1]) << 8) |
        (static_cast<std::uint32_t>(input[off + 2]) << 16) |
        (static_cast<std::uint32_t>(input[off + 3]) << 24);
    const std::uint32_t generation =
        static_cast<std::uint32_t>(input[off + 4]) |
        (static_cast<std::uint32_t>(input[off + 5]) << 8) |
        (static_cast<std::uint32_t>(input[off + 6]) << 16) |
        (static_cast<std::uint32_t>(input[off + 7]) << 24);
    const auto iv = input.subspan(off + 8, kAesIvSize);
    return {method, file_key, object_id, generation, iv, header_len};
}

}  // namespace

std::vector<std::byte> Encode(std::span<const std::byte> input) {
    const auto h = ParseWire(input);
    const auto plaintext = input.subspan(h.payload_offset);
    const auto cipher = Encrypt(
        h.method, h.file_key, h.object_id, h.generation, plaintext, h.iv);
    std::vector<std::byte> out;
    out.reserve(h.payload_offset + cipher.size());
    out.insert(out.end(), input.begin(),
               input.begin() + static_cast<std::ptrdiff_t>(h.payload_offset));
    out.insert(out.end(), cipher.begin(), cipher.end());
    return out;
}

std::vector<std::byte> Decode(std::span<const std::byte> input) {
    const auto h = ParseWire(input);
    const auto cipher = input.subspan(h.payload_offset);
    const auto plaintext = Decrypt(
        h.method, h.file_key, h.object_id, h.generation, cipher);
    std::vector<std::byte> out;
    out.reserve(h.payload_offset + plaintext.size());
    out.insert(out.end(), input.begin(),
               input.begin() + static_cast<std::ptrdiff_t>(h.payload_offset));
    out.insert(out.end(), plaintext.begin(), plaintext.end());
    return out;
}

}  // namespace foundation::crypt_filter
