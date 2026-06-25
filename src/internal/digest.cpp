#include "digest.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace foundation::digest {

namespace {

// ──────────────────────────────────────────────────────────────────────────
// Bit-fiddling helpers
// ──────────────────────────────────────────────────────────────────────────

inline std::uint32_t Rotl32(std::uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

inline std::uint32_t Rotr32(std::uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

inline std::uint64_t Rotr64(std::uint64_t x, int n) {
    return (x >> n) | (x << (64 - n));
}

inline std::uint32_t LoadLe32(const std::uint8_t* p) {
    return std::uint32_t(p[0])
         | (std::uint32_t(p[1]) <<  8)
         | (std::uint32_t(p[2]) << 16)
         | (std::uint32_t(p[3]) << 24);
}

inline std::uint32_t LoadBe32(const std::uint8_t* p) {
    return (std::uint32_t(p[0]) << 24)
         | (std::uint32_t(p[1]) << 16)
         | (std::uint32_t(p[2]) <<  8)
         |  std::uint32_t(p[3]);
}

inline std::uint64_t LoadBe64(const std::uint8_t* p) {
    return (std::uint64_t(LoadBe32(p))     << 32)
         |  std::uint64_t(LoadBe32(p + 4));
}

inline void StoreBe32(std::uint8_t* p, std::uint32_t v) {
    p[0] = std::uint8_t(v >> 24);
    p[1] = std::uint8_t(v >> 16);
    p[2] = std::uint8_t(v >>  8);
    p[3] = std::uint8_t(v);
}

inline void StoreLe32(std::uint8_t* p, std::uint32_t v) {
    p[0] = std::uint8_t(v);
    p[1] = std::uint8_t(v >>  8);
    p[2] = std::uint8_t(v >> 16);
    p[3] = std::uint8_t(v >> 24);
}

inline void StoreBe64(std::uint8_t* p, std::uint64_t v) {
    StoreBe32(p,     std::uint32_t(v >> 32));
    StoreBe32(p + 4, std::uint32_t(v));
}

// ──────────────────────────────────────────────────────────────────────────
// MD5 (RFC 1321)
// ──────────────────────────────────────────────────────────────────────────

constexpr std::array<std::uint32_t, 64> kMd5T = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
    0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
    0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
    0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
    0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
    0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
    0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
    0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
    0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

constexpr std::array<int, 64> kMd5S = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

void Md5Compress(std::array<std::uint32_t, 4>& H, const std::uint8_t* block) {
    std::uint32_t M[16];
    for (int i = 0; i < 16; ++i) M[i] = LoadLe32(block + i * 4);

    std::uint32_t a = H[0], b = H[1], c = H[2], d = H[3];

    for (int i = 0; i < 64; ++i) {
        std::uint32_t f;
        int g;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) % 16;
        }
        std::uint32_t temp = d;
        d = c;
        c = b;
        b = b + Rotl32(a + f + kMd5T[i] + M[g], kMd5S[i]);
        a = temp;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
}

std::vector<std::byte> Md5Impl(std::span<const std::byte> data) {
    std::array<std::uint32_t, 4> H = {
        0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476
    };

    const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t total = data.size();
    const std::size_t blocks = total / 64;
    for (std::size_t i = 0; i < blocks; ++i) Md5Compress(H, p + i * 64);

    // Tail + padding. RFC 1321 §3.1: append 0x80, then zero bytes
    // until length mod 64 == 56, then append 64-bit LE bit length.
    std::uint8_t tail[128] = {0};
    const std::size_t rem = total % 64;
    std::memcpy(tail, p + blocks * 64, rem);
    tail[rem] = 0x80;
    const std::size_t pad_to = (rem < 56) ? 56 : 120;
    const std::uint64_t bit_len = std::uint64_t(total) * 8u;
    for (int i = 0; i < 8; ++i) {
        tail[pad_to + i] = std::uint8_t(bit_len >> (8 * i));
    }
    Md5Compress(H, tail);
    if (pad_to == 120) Md5Compress(H, tail + 64);

    std::vector<std::byte> out(16);
    for (int i = 0; i < 4; ++i) {
        StoreLe32(reinterpret_cast<std::uint8_t*>(out.data()) + i * 4, H[i]);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// SHA-1 (FIPS 180-4)
// ──────────────────────────────────────────────────────────────────────────

void Sha1Compress(std::array<std::uint32_t, 5>& H, const std::uint8_t* block) {
    std::uint32_t W[80];
    for (int i = 0; i < 16; ++i) W[i] = LoadBe32(block + i * 4);
    for (int i = 16; i < 80; ++i) {
        W[i] = Rotl32(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    std::uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4];
    for (int i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        std::uint32_t temp = Rotl32(a, 5) + f + e + k + W[i];
        e = d;
        d = c;
        c = Rotl32(b, 30);
        b = a;
        a = temp;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d; H[4] += e;
}

std::vector<std::byte> Sha1Impl(std::span<const std::byte> data) {
    std::array<std::uint32_t, 5> H = {
        0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
    };

    const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t total = data.size();
    const std::size_t blocks = total / 64;
    for (std::size_t i = 0; i < blocks; ++i) Sha1Compress(H, p + i * 64);

    std::uint8_t tail[128] = {0};
    const std::size_t rem = total % 64;
    std::memcpy(tail, p + blocks * 64, rem);
    tail[rem] = 0x80;
    const std::size_t pad_to = (rem < 56) ? 56 : 120;
    StoreBe64(tail + pad_to, std::uint64_t(total) * 8u);
    Sha1Compress(H, tail);
    if (pad_to == 120) Sha1Compress(H, tail + 64);

    std::vector<std::byte> out(20);
    for (int i = 0; i < 5; ++i) {
        StoreBe32(reinterpret_cast<std::uint8_t*>(out.data()) + i * 4, H[i]);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// SHA-256 (FIPS 180-4)
// ──────────────────────────────────────────────────────────────────────────

constexpr std::array<std::uint32_t, 64> kSha256K = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

void Sha256Compress(std::array<std::uint32_t, 8>& H, const std::uint8_t* block) {
    std::uint32_t W[64];
    for (int i = 0; i < 16; ++i) W[i] = LoadBe32(block + i * 4);
    for (int i = 16; i < 64; ++i) {
        std::uint32_t s0 = Rotr32(W[i-15], 7) ^ Rotr32(W[i-15], 18) ^ (W[i-15] >> 3);
        std::uint32_t s1 = Rotr32(W[i-2], 17) ^ Rotr32(W[i-2], 19) ^ (W[i-2] >> 10);
        W[i] = W[i-16] + s0 + W[i-7] + s1;
    }

    std::uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
    std::uint32_t e = H[4], f = H[5], g = H[6], h = H[7];
    for (int i = 0; i < 64; ++i) {
        std::uint32_t S1 = Rotr32(e, 6) ^ Rotr32(e, 11) ^ Rotr32(e, 25);
        std::uint32_t ch = (e & f) ^ (~e & g);
        std::uint32_t t1 = h + S1 + ch + kSha256K[i] + W[i];
        std::uint32_t S0 = Rotr32(a, 2) ^ Rotr32(a, 13) ^ Rotr32(a, 22);
        std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        std::uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
}

std::vector<std::byte> Sha256Impl(std::span<const std::byte> data) {
    std::array<std::uint32_t, 8> H = {
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
    };

    const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t total = data.size();
    const std::size_t blocks = total / 64;
    for (std::size_t i = 0; i < blocks; ++i) Sha256Compress(H, p + i * 64);

    std::uint8_t tail[128] = {0};
    const std::size_t rem = total % 64;
    std::memcpy(tail, p + blocks * 64, rem);
    tail[rem] = 0x80;
    const std::size_t pad_to = (rem < 56) ? 56 : 120;
    StoreBe64(tail + pad_to, std::uint64_t(total) * 8u);
    Sha256Compress(H, tail);
    if (pad_to == 120) Sha256Compress(H, tail + 64);

    std::vector<std::byte> out(32);
    for (int i = 0; i < 8; ++i) {
        StoreBe32(reinterpret_cast<std::uint8_t*>(out.data()) + i * 4, H[i]);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// SHA-512 / SHA-384 (FIPS 180-4)
// ──────────────────────────────────────────────────────────────────────────
//
// SHA-512 and SHA-384 share the compress function; they differ in
// initial hash values + truncation length. 80 rounds, 64-bit ops,
// 128-byte block, 128-bit length pad.

constexpr std::array<std::uint64_t, 80> kSha512K = {
    0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
    0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL, 0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
    0xD807AA98A3030242ULL, 0x12835B0145706FBEULL, 0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
    0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
    0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL, 0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
    0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
    0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
    0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL, 0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
    0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
    0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
    0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL, 0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
    0xD192E819D6EF5218ULL, 0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
    0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL, 0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
    0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL, 0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
    0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
    0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,
    0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL, 0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL,
    0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL, 0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
    0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL, 0x3C9EBE0A15C9BEBCULL, 0x431D67C49C100D4CULL,
    0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL, 0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL
};

void Sha512Compress(std::array<std::uint64_t, 8>& H, const std::uint8_t* block) {
    std::uint64_t W[80];
    for (int i = 0; i < 16; ++i) W[i] = LoadBe64(block + i * 8);
    for (int i = 16; i < 80; ++i) {
        std::uint64_t s0 = Rotr64(W[i-15], 1) ^ Rotr64(W[i-15], 8) ^ (W[i-15] >> 7);
        std::uint64_t s1 = Rotr64(W[i-2], 19) ^ Rotr64(W[i-2], 61) ^ (W[i-2] >> 6);
        W[i] = W[i-16] + s0 + W[i-7] + s1;
    }

    std::uint64_t a = H[0], b = H[1], c = H[2], d = H[3];
    std::uint64_t e = H[4], f = H[5], g = H[6], h = H[7];
    for (int i = 0; i < 80; ++i) {
        std::uint64_t S1 = Rotr64(e, 14) ^ Rotr64(e, 18) ^ Rotr64(e, 41);
        std::uint64_t ch = (e & f) ^ (~e & g);
        std::uint64_t t1 = h + S1 + ch + kSha512K[i] + W[i];
        std::uint64_t S0 = Rotr64(a, 28) ^ Rotr64(a, 34) ^ Rotr64(a, 39);
        std::uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        std::uint64_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
}

// Drives SHA-512 / SHA-384 from initial-state-vector + truncation
// length parameters. 128-byte block, 128-bit length field. The
// length field is 16 bytes BE; we never approach 2^64 bits in one
// digest call so the upper 8 bytes are always zero.
std::vector<std::byte> Sha512Variant(std::span<const std::byte> data,
                                     const std::array<std::uint64_t, 8>& iv,
                                     std::size_t out_bytes) {
    std::array<std::uint64_t, 8> H = iv;

    const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t total = data.size();
    const std::size_t blocks = total / 128;
    for (std::size_t i = 0; i < blocks; ++i) Sha512Compress(H, p + i * 128);

    std::uint8_t tail[256] = {0};
    const std::size_t rem = total % 128;
    std::memcpy(tail, p + blocks * 128, rem);
    tail[rem] = 0x80;
    const std::size_t pad_to = (rem < 112) ? 112 : 240;
    // 16-byte BE length: top 8 bytes are zero (no input ≥ 2^61 bytes
    // fits in size_t on any practical platform); low 8 bytes carry
    // the bit count.
    StoreBe64(tail + pad_to + 8, std::uint64_t(total) * 8u);
    Sha512Compress(H, tail);
    if (pad_to == 240) Sha512Compress(H, tail + 128);

    std::vector<std::byte> out(out_bytes);
    std::uint8_t buf[64];
    for (int i = 0; i < 8; ++i) StoreBe64(buf + i * 8, H[i]);
    std::memcpy(out.data(), buf, out_bytes);
    return out;
}

constexpr std::array<std::uint64_t, 8> kSha384Iv = {
    0xCBBB9D5DC1059ED8ULL, 0x629A292A367CD507ULL,
    0x9159015A3070DD17ULL, 0x152FECD8F70E5939ULL,
    0x67332667FFC00B31ULL, 0x8EB44A8768581511ULL,
    0xDB0C2E0D64F98FA7ULL, 0x47B5481DBEFA4FA4ULL
};

constexpr std::array<std::uint64_t, 8> kSha512Iv = {
    0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL,
    0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
    0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL,
    0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL
};

}  // namespace

// ──────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────

std::vector<std::byte> Md5(std::span<const std::byte> data) {
    return Md5Impl(data);
}

std::vector<std::byte> Sha1(std::span<const std::byte> data) {
    return Sha1Impl(data);
}

std::vector<std::byte> Sha256(std::span<const std::byte> data) {
    return Sha256Impl(data);
}

std::vector<std::byte> Sha384(std::span<const std::byte> data) {
    return Sha512Variant(data, kSha384Iv, 48);
}

std::vector<std::byte> Sha512(std::span<const std::byte> data) {
    return Sha512Variant(data, kSha512Iv, 64);
}

Bundle Parse(std::span<const std::byte> input) {
    Bundle b;
    b.md5    = Md5(input);
    b.sha1   = Sha1(input);
    b.sha256 = Sha256(input);
    b.sha384 = Sha384(input);
    b.sha512 = Sha512(input);
    return b;
}

namespace {

std::string HexLower(const std::vector<std::byte>& bytes) {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (auto b : bytes) {
        std::uint8_t v = std::uint8_t(b);
        out.push_back(kHex[v >> 4]);
        out.push_back(kHex[v & 0x0F]);
    }
    return out;
}

}  // namespace

std::string ToCanonical(const Bundle& bundle) {
    std::string out;
    out.reserve(400);
    out += "md5: ";    out += HexLower(bundle.md5);    out += '\n';
    out += "sha1: ";   out += HexLower(bundle.sha1);   out += '\n';
    out += "sha256: "; out += HexLower(bundle.sha256); out += '\n';
    out += "sha384: "; out += HexLower(bundle.sha384); out += '\n';
    out += "sha512: "; out += HexLower(bundle.sha512); out += '\n';
    return out;
}

}  // namespace foundation::digest
