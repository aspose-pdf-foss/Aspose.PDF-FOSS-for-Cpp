#include "crypto.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

#include "digest.hpp"

namespace foundation::crypto {

namespace {

using Bytes = std::vector<std::byte>;

std::size_t BlockSize(Hash h) {
    return (h == Hash::Sha384 || h == Hash::Sha512) ? 128 : 64;
}

Bytes StripPkcs7(Bytes data, std::size_t block) {
    if (data.empty() || data.size() % block != 0)
        throw std::runtime_error("crypto: bad ciphertext length");
    const auto pad = std::to_integer<std::size_t>(data.back());
    if (pad == 0 || pad > block || pad > data.size())
        throw std::runtime_error("crypto: bad padding");
    for (std::size_t i = data.size() - pad; i < data.size(); ++i)
        if (std::to_integer<std::size_t>(data[i]) != pad)
            throw std::runtime_error("crypto: bad padding");
    data.resize(data.size() - pad);
    return data;
}

// ---- DES (FIPS 46-3) ---------------------------------------------------------

constexpr std::array<std::uint8_t, 64> kIp = {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9,  1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};
constexpr std::array<std::uint8_t, 64> kFp = {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9,  49, 17, 57, 25};
constexpr std::array<std::uint8_t, 48> kE = {
    32, 1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9,  10, 11,
    12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
    22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};
constexpr std::array<std::uint8_t, 32> kP = {
    16, 7, 20, 21, 29, 12, 28, 17, 1,  15, 23, 26, 5,  18, 31, 10,
    2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25};
constexpr std::array<std::uint8_t, 56> kPc1 = {
    57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18,
    10, 2,  59, 51, 43, 35, 27, 19, 11, 3,  60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22,
    14, 6,  61, 53, 45, 37, 29, 21, 13, 5,  28, 20, 12, 4};
constexpr std::array<std::uint8_t, 48> kPc2 = {
    14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,
    26, 8,  16, 7,  27, 20, 13, 2,  41, 52, 31, 37, 47, 55, 30, 40,
    51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};
constexpr std::array<std::uint8_t, 16> kShifts = {1, 1, 2, 2, 2, 2, 2, 2,
                                                  1, 2, 2, 2, 2, 2, 2, 1};
constexpr std::uint8_t kS[8][64] = {
    {14, 4,  13, 1, 2,  15, 11, 8,  3,  10, 6,  12, 5,  9,  0, 7,
     0,  15, 7,  4, 14, 2,  13, 1,  10, 6,  12, 11, 9,  5,  3, 8,
     4,  1,  14, 8, 13, 6,  2,  11, 15, 12, 9,  7,  3,  10, 5, 0,
     15, 12, 8,  2, 4,  9,  1,  7,  5,  11, 3,  14, 10, 0,  6, 13},
    {15, 1,  8,  14, 6,  11, 3,  4,  9,  7, 2,  13, 12, 0, 5,  10,
     3,  13, 4,  7,  15, 2,  8,  14, 12, 0, 1,  10, 6,  9, 11, 5,
     0,  14, 7,  11, 10, 4,  13, 1,  5,  8, 12, 6,  9,  3, 2,  15,
     13, 8,  10, 1,  3,  15, 4,  2,  11, 6, 7,  12, 0,  5, 14, 9},
    {10, 0,  9,  14, 6, 3,  15, 5,  1,  13, 12, 7,  11, 4,  2,  8,
     13, 7,  0,  9,  3, 4,  6,  10, 2,  8,  5,  14, 12, 11, 15, 1,
     13, 6,  4,  9,  8, 15, 3,  0,  11, 1,  2,  12, 5,  10, 14, 7,
     1,  10, 13, 0,  6, 9,  8,  7,  4,  15, 14, 3,  11, 5,  2,  12},
    {7,  13, 14, 3, 0,  6,  9,  10, 1,  2, 8, 5,  11, 12, 4,  15,
     13, 8,  11, 5, 6,  15, 0,  3,  4,  7, 2, 12, 1,  10, 14, 9,
     10, 6,  9,  0, 12, 11, 7,  13, 15, 1, 3, 14, 5,  2,  8,  4,
     3,  15, 0,  6, 10, 1,  13, 8,  9,  4, 5, 11, 12, 7,  2,  14},
    {2,  12, 4,  1,  7,  10, 11, 6,  8,  5,  3,  15, 13, 0, 14, 9,
     14, 11, 2,  12, 4,  7,  13, 1,  5,  0,  15, 10, 3,  9, 8,  6,
     4,  2,  1,  11, 10, 13, 7,  8,  15, 9,  12, 5,  6,  3, 0,  14,
     11, 8,  12, 7,  1,  14, 2,  13, 6,  15, 0,  9,  10, 4, 5,  3},
    {12, 1,  10, 15, 9, 2,  6,  8,  0,  13, 3,  4,  14, 7,  5,  11,
     10, 15, 4,  2,  7, 12, 9,  5,  6,  1,  13, 14, 0,  11, 3,  8,
     9,  14, 15, 5,  2, 8,  12, 3,  7,  0,  4,  10, 1,  13, 11, 6,
     4,  3,  2,  12, 9, 5,  15, 10, 11, 14, 1,  7,  6,  0,  8,  13},
    {4,  11, 2,  14, 15, 0, 8,  13, 3,  12, 9, 7,  5,  10, 6, 1,
     13, 0,  11, 7,  4,  9, 1,  10, 14, 3,  5, 12, 2,  15, 8, 6,
     1,  4,  11, 13, 12, 3, 7,  14, 10, 15, 6, 8,  0,  5,  9, 2,
     6,  11, 13, 8,  1,  4, 10, 7,  9,  5,  0, 15, 14, 2,  3, 12},
    {13, 2,  8,  4, 6,  15, 11, 1,  10, 9,  3,  14, 5,  0,  12, 7,
     1,  15, 13, 8, 10, 3,  7,  4,  12, 5,  6,  11, 0,  14, 9,  2,
     7,  11, 4,  1, 9,  12, 14, 2,  0,  6,  10, 13, 15, 3,  5,  8,
     2,  1,  14, 7, 4,  10, 8,  13, 15, 12, 9,  0,  3,  5,  6,  11}};

// Bit permutation: output bit i (MSB first) = input bit table[i] (1-based,
// MSB first, of an `in_bits`-wide value).
template <std::size_t N>
std::uint64_t Permute(std::uint64_t in, const std::array<std::uint8_t, N>& t,
                      int in_bits) {
    std::uint64_t out = 0;
    for (std::size_t i = 0; i < N; ++i)
        out = (out << 1) | ((in >> (in_bits - t[i])) & 1);
    return out;
}

using DesKeys = std::array<std::uint64_t, 16>;

DesKeys DesSchedule(std::span<const std::byte> key8) {
    std::uint64_t k = 0;
    for (int i = 0; i < 8; ++i)
        k = (k << 8) | std::to_integer<std::uint64_t>(key8[static_cast<std::size_t>(i)]);
    const std::uint64_t cd = Permute(k, kPc1, 64);
    std::uint32_t c = static_cast<std::uint32_t>(cd >> 28) & 0x0FFFFFFF;
    std::uint32_t d = static_cast<std::uint32_t>(cd) & 0x0FFFFFFF;
    DesKeys keys{};
    for (int r = 0; r < 16; ++r) {
        for (int s = 0; s < kShifts[static_cast<std::size_t>(r)]; ++s) {
            c = ((c << 1) | (c >> 27)) & 0x0FFFFFFF;
            d = ((d << 1) | (d >> 27)) & 0x0FFFFFFF;
        }
        keys[static_cast<std::size_t>(r)] =
            Permute((static_cast<std::uint64_t>(c) << 28) | d, kPc2, 56);
    }
    return keys;
}

std::uint32_t DesF(std::uint32_t r, std::uint64_t k) {
    const std::uint64_t x = Permute(r, kE, 32) ^ k;
    std::uint32_t out = 0;
    for (int i = 0; i < 8; ++i) {
        const auto six = static_cast<unsigned>((x >> (42 - 6 * i)) & 0x3F);
        const unsigned row = ((six >> 4) & 2) | (six & 1);
        const unsigned col = (six >> 1) & 0xF;
        out = (out << 4) | kS[i][row * 16 + col];
    }
    return static_cast<std::uint32_t>(Permute(out, kP, 32));
}

std::uint64_t DesBlock(std::uint64_t block, const DesKeys& keys, bool decrypt) {
    const std::uint64_t ip = Permute(block, kIp, 64);
    std::uint32_t l = static_cast<std::uint32_t>(ip >> 32);
    std::uint32_t r = static_cast<std::uint32_t>(ip);
    for (int i = 0; i < 16; ++i) {
        const std::uint64_t k = keys[static_cast<std::size_t>(decrypt ? 15 - i : i)];
        const std::uint32_t t = r;
        r = l ^ DesF(r, k);
        l = t;
    }
    return Permute((static_cast<std::uint64_t>(r) << 32) | l, kFp, 64);
}

std::uint64_t Load64(std::span<const std::byte> b) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v = (v << 8) | std::to_integer<std::uint64_t>(b[static_cast<std::size_t>(i)]);
    return v;
}

void Store64(std::uint64_t v, std::byte* out) {
    for (int i = 7; i >= 0; --i) {
        out[i] = static_cast<std::byte>(v & 0xFF);
        v >>= 8;
    }
}

// ---- RC2 (RFC 2268) ----------------------------------------------------------

constexpr std::uint8_t kPiTable[256] = {
    0xd9, 0x78, 0xf9, 0xc4, 0x19, 0xdd, 0xb5, 0xed, 0x28, 0xe9, 0xfd, 0x79, 0x4a, 0xa0, 0xd8, 0x9d,
    0xc6, 0x7e, 0x37, 0x83, 0x2b, 0x76, 0x53, 0x8e, 0x62, 0x4c, 0x64, 0x88, 0x44, 0x8b, 0xfb, 0xa2,
    0x17, 0x9a, 0x59, 0xf5, 0x87, 0xb3, 0x4f, 0x13, 0x61, 0x45, 0x6d, 0x8d, 0x09, 0x81, 0x7d, 0x32,
    0xbd, 0x8f, 0x40, 0xeb, 0x86, 0xb7, 0x7b, 0x0b, 0xf0, 0x95, 0x21, 0x22, 0x5c, 0x6b, 0x4e, 0x82,
    0x54, 0xd6, 0x65, 0x93, 0xce, 0x60, 0xb2, 0x1c, 0x73, 0x56, 0xc0, 0x14, 0xa7, 0x8c, 0xf1, 0xdc,
    0x12, 0x75, 0xca, 0x1f, 0x3b, 0xbe, 0xe4, 0xd1, 0x42, 0x3d, 0xd4, 0x30, 0xa3, 0x3c, 0xb6, 0x26,
    0x6f, 0xbf, 0x0e, 0xda, 0x46, 0x69, 0x07, 0x57, 0x27, 0xf2, 0x1d, 0x9b, 0xbc, 0x94, 0x43, 0x03,
    0xf8, 0x11, 0xc7, 0xf6, 0x90, 0xef, 0x3e, 0xe7, 0x06, 0xc3, 0xd5, 0x2f, 0xc8, 0x66, 0x1e, 0xd7,
    0x08, 0xe8, 0xea, 0xde, 0x80, 0x52, 0xee, 0xf7, 0x84, 0xaa, 0x72, 0xac, 0x35, 0x4d, 0x6a, 0x2a,
    0x96, 0x1a, 0xd2, 0x71, 0x5a, 0x15, 0x49, 0x74, 0x4b, 0x9f, 0xd0, 0x5e, 0x04, 0x18, 0xa4, 0xec,
    0xc2, 0xe0, 0x41, 0x6e, 0x0f, 0x51, 0xcb, 0xcc, 0x24, 0x91, 0xaf, 0x50, 0xa1, 0xf4, 0x70, 0x39,
    0x99, 0x7c, 0x3a, 0x85, 0x23, 0xb8, 0xb4, 0x7a, 0xfc, 0x02, 0x36, 0x5b, 0x25, 0x55, 0x97, 0x31,
    0x2d, 0x5d, 0xfa, 0x98, 0xe3, 0x8a, 0x92, 0xae, 0x05, 0xdf, 0x29, 0x10, 0x67, 0x6c, 0xba, 0xc9,
    0xd3, 0x00, 0xe6, 0xcf, 0xe1, 0x9e, 0xa8, 0x2c, 0x63, 0x16, 0x01, 0x3f, 0x58, 0xe2, 0x89, 0xa9,
    0x0d, 0x38, 0x34, 0x1b, 0xab, 0x33, 0xff, 0xb0, 0xbb, 0x48, 0x0c, 0x5f, 0xb9, 0xb1, 0xcd, 0x2e,
    0xc5, 0xf3, 0xdb, 0x47, 0xe5, 0xa5, 0x9c, 0x77, 0x0a, 0xa6, 0x20, 0x68, 0xfe, 0x7f, 0xc1, 0xad};

std::array<std::uint16_t, 64> Rc2Schedule(std::span<const std::byte> key,
                                          int effective_bits) {
    const std::size_t t = key.size();
    if (t == 0 || t > 128) throw std::runtime_error("rc2: bad key length");
    std::array<std::uint8_t, 128> l{};
    for (std::size_t i = 0; i < t; ++i) l[i] = std::to_integer<std::uint8_t>(key[i]);
    for (std::size_t i = t; i < 128; ++i)
        l[i] = kPiTable[(l[i - 1] + l[i - t]) & 0xFF];
    const int t1 = effective_bits;
    const int t8 = (t1 + 7) / 8;
    const unsigned tm = 0xFFu >> (8 * t8 - t1);
    l[static_cast<std::size_t>(128 - t8)] =
        kPiTable[l[static_cast<std::size_t>(128 - t8)] & tm];
    for (int i = 127 - t8; i >= 0; --i)
        l[static_cast<std::size_t>(i)] =
            kPiTable[l[static_cast<std::size_t>(i + 1)] ^
                     l[static_cast<std::size_t>(i + t8)]];
    std::array<std::uint16_t, 64> k{};
    for (std::size_t i = 0; i < 64; ++i)
        k[i] = static_cast<std::uint16_t>(l[2 * i] | (l[2 * i + 1] << 8));
    return k;
}

void Rc2Decrypt(std::array<std::uint16_t, 4>& r,
                const std::array<std::uint16_t, 64>& k) {
    static constexpr int kRot[4] = {1, 2, 3, 5};
    int j = 63;
    auto rmix = [&]() {
        for (int i = 3; i >= 0; --i) {
            const int s = kRot[i];
            r[static_cast<std::size_t>(i)] = static_cast<std::uint16_t>(
                (r[static_cast<std::size_t>(i)] >> s) |
                (r[static_cast<std::size_t>(i)] << (16 - s)));
            const std::uint16_t a = r[static_cast<std::size_t>((i + 3) & 3)];
            const std::uint16_t b = r[static_cast<std::size_t>((i + 2) & 3)];
            const std::uint16_t c = r[static_cast<std::size_t>((i + 1) & 3)];
            r[static_cast<std::size_t>(i)] = static_cast<std::uint16_t>(
                r[static_cast<std::size_t>(i)] - k[static_cast<std::size_t>(j)] -
                (a & b) - (static_cast<std::uint16_t>(~a) & c));
            --j;
        }
    };
    auto rmash = [&]() {
        for (int i = 3; i >= 0; --i)
            r[static_cast<std::size_t>(i)] = static_cast<std::uint16_t>(
                r[static_cast<std::size_t>(i)] -
                k[r[static_cast<std::size_t>((i + 3) & 3)] & 63]);
    };
    for (int n = 0; n < 5; ++n) rmix();
    rmash();
    for (int n = 0; n < 6; ++n) rmix();
    rmash();
    for (int n = 0; n < 5; ++n) rmix();
}

}  // namespace

std::size_t HashSize(Hash h) {
    switch (h) {
        case Hash::Sha1: return 20;
        case Hash::Sha256: return 32;
        case Hash::Sha384: return 48;
        case Hash::Sha512: return 64;
    }
    return 0;
}

std::vector<std::byte> HashOf(Hash h, std::span<const std::byte> data) {
    switch (h) {
        case Hash::Sha1: return foundation::digest::Sha1(data);
        case Hash::Sha256: return foundation::digest::Sha256(data);
        case Hash::Sha384: return foundation::digest::Sha384(data);
        case Hash::Sha512: return foundation::digest::Sha512(data);
    }
    return {};
}

std::vector<std::byte> Hmac(Hash h, std::span<const std::byte> key,
                            std::span<const std::byte> data) {
    const std::size_t b = BlockSize(h);
    Bytes k(key.begin(), key.end());
    if (k.size() > b) k = HashOf(h, k);
    k.resize(b, std::byte{0});
    Bytes inner(b), outer(b);
    for (std::size_t i = 0; i < b; ++i) {
        inner[i] = k[i] ^ std::byte{0x36};
        outer[i] = k[i] ^ std::byte{0x5c};
    }
    inner.insert(inner.end(), data.begin(), data.end());
    const Bytes ih = HashOf(h, inner);
    outer.insert(outer.end(), ih.begin(), ih.end());
    return HashOf(h, outer);
}

std::vector<std::byte> Pbkdf2(Hash h, std::span<const std::byte> password,
                              std::span<const std::byte> salt,
                              std::uint32_t iterations, std::size_t length) {
    Bytes out;
    for (std::uint32_t block = 1; out.size() < length; ++block) {
        Bytes s(salt.begin(), salt.end());
        for (int shift = 24; shift >= 0; shift -= 8)
            s.push_back(static_cast<std::byte>((block >> shift) & 0xFF));
        Bytes u = Hmac(h, password, s);
        Bytes t = u;
        for (std::uint32_t i = 1; i < iterations; ++i) {
            u = Hmac(h, password, u);
            for (std::size_t j = 0; j < t.size(); ++j) t[j] ^= u[j];
        }
        out.insert(out.end(), t.begin(), t.end());
    }
    out.resize(length);
    return out;
}

std::vector<std::byte> Pkcs12Kdf(Hash h, const std::string& password,
                                 std::span<const std::byte> salt,
                                 std::uint8_t id, std::uint32_t iterations,
                                 std::size_t length) {
    const std::size_t u = HashSize(h), v = BlockSize(h);
    // Password as a NUL-terminated big-endian BMPString (ASCII / Latin-1).
    Bytes pw;
    for (unsigned char c : password) {
        pw.push_back(std::byte{0});
        pw.push_back(static_cast<std::byte>(c));
    }
    pw.push_back(std::byte{0});
    pw.push_back(std::byte{0});
    auto repeat = [&](std::span<const std::byte> in) {
        Bytes out;
        if (in.empty()) return out;
        const std::size_t len = (in.size() + v - 1) / v * v;
        for (std::size_t i = 0; i < len; ++i) out.push_back(in[i % in.size()]);
        return out;
    };
    Bytes i_buf = repeat(salt);
    const Bytes p = repeat(pw);
    i_buf.insert(i_buf.end(), p.begin(), p.end());
    const Bytes d(v, static_cast<std::byte>(id));
    Bytes out;
    while (out.size() < length) {
        Bytes a = d;
        a.insert(a.end(), i_buf.begin(), i_buf.end());
        a = HashOf(h, a);
        for (std::uint32_t r = 1; r < iterations; ++r) a = HashOf(h, a);
        out.insert(out.end(), a.begin(), a.end());
        if (out.size() >= length) break;
        Bytes b(v);
        for (std::size_t k = 0; k < v; ++k) b[k] = a[k % u];
        for (std::size_t off = 0; off < i_buf.size(); off += v) {
            unsigned carry = 1;
            for (std::size_t k = v; k-- > 0;) {
                const unsigned sum = std::to_integer<unsigned>(i_buf[off + k]) +
                                     std::to_integer<unsigned>(b[k]) + carry;
                i_buf[off + k] = static_cast<std::byte>(sum & 0xFF);
                carry = sum >> 8;
            }
        }
    }
    out.resize(length);
    return out;
}

std::vector<std::byte> Des3CbcDecrypt(std::span<const std::byte> key,
                                      std::span<const std::byte> iv,
                                      std::span<const std::byte> data) {
    if (key.size() != 24 || iv.size() != 8 || data.size() % 8 != 0)
        throw std::runtime_error("3des: bad key, iv or data length");
    const DesKeys k1 = DesSchedule(key.subspan(0, 8));
    const DesKeys k2 = DesSchedule(key.subspan(8, 8));
    const DesKeys k3 = DesSchedule(key.subspan(16, 8));
    Bytes out(data.size());
    std::uint64_t prev = Load64(iv);
    for (std::size_t off = 0; off < data.size(); off += 8) {
        const std::uint64_t c = Load64(data.subspan(off, 8));
        std::uint64_t p = DesBlock(c, k3, true);
        p = DesBlock(p, k2, false);
        p = DesBlock(p, k1, true);
        Store64(p ^ prev, out.data() + off);
        prev = c;
    }
    return StripPkcs7(std::move(out), 8);
}

std::vector<std::byte> Rc2DecryptBlock(std::span<const std::byte> key,
                                       int effective_bits,
                                       std::span<const std::byte> block) {
    const auto k = Rc2Schedule(key, effective_bits);
    std::array<std::uint16_t, 4> r{};
    for (std::size_t i = 0; i < 4; ++i)
        r[i] = static_cast<std::uint16_t>(
            std::to_integer<unsigned>(block[2 * i]) |
            (std::to_integer<unsigned>(block[2 * i + 1]) << 8));
    Rc2Decrypt(r, k);
    Bytes out(8);
    for (std::size_t i = 0; i < 4; ++i) {
        out[2 * i] = static_cast<std::byte>(r[i] & 0xFF);
        out[2 * i + 1] = static_cast<std::byte>(r[i] >> 8);
    }
    return out;
}

std::vector<std::byte> Rc2CbcDecrypt(std::span<const std::byte> key,
                                     int effective_bits,
                                     std::span<const std::byte> iv,
                                     std::span<const std::byte> data) {
    if (iv.size() != 8 || data.size() % 8 != 0)
        throw std::runtime_error("rc2: bad iv or data length");
    Bytes out(data.size());
    Bytes prev(iv.begin(), iv.end());
    for (std::size_t off = 0; off < data.size(); off += 8) {
        const Bytes p = Rc2DecryptBlock(key, effective_bits, data.subspan(off, 8));
        for (std::size_t i = 0; i < 8; ++i) out[off + i] = p[i] ^ prev[i];
        prev.assign(data.begin() + static_cast<std::ptrdiff_t>(off),
                    data.begin() + static_cast<std::ptrdiff_t>(off + 8));
    }
    return StripPkcs7(std::move(out), 8);
}

}  // namespace foundation::crypto
