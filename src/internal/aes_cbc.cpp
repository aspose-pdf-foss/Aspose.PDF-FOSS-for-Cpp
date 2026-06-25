#include "aes_cbc.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace foundation::aes_cbc {

namespace {

// ──────────────────────────────────────────────────────────────────────────
// AES tables (FIPS 197 §5.1.1, §5.3.2)
// ──────────────────────────────────────────────────────────────────────────

constexpr std::array<std::uint8_t, 256> kSbox = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

constexpr std::array<std::uint8_t, 256> kInvSbox = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

// Round constants (FIPS 197 §5.2). Rcon[i] is RC[i] || 0 || 0
// || 0; we bake the byte sequence directly. AES-256 uses Rcon
// indices 1..7; AES-128 uses indices 1..10.
constexpr std::array<std::uint8_t, 11> kRcon = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36
};

// xtime: multiply by 2 (i.e. {02}) in GF(2^8) modulo
// x^8 + x^4 + x^3 + x + 1 (0x11B; reduced low byte is 0x1B).
inline std::uint8_t Xtime(std::uint8_t b) {
    return std::uint8_t((b << 1) ^ ((b & 0x80) ? 0x1B : 0x00));
}

// GF(2^8) multiply by an arbitrary constant. Used by
// InvMixColumns where the multipliers are {09, 0B, 0D, 0E};
// forward MixColumns uses {01, 02, 03} which is just XOR + xtime.
inline std::uint8_t GfMul(std::uint8_t a, std::uint8_t b) {
    std::uint8_t r = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) r ^= a;
        a = Xtime(a);
        b = std::uint8_t(b >> 1);
    }
    return r;
}

// ──────────────────────────────────────────────────────────────────────────
// AES round transformations (FIPS 197 §5.1)
// ──────────────────────────────────────────────────────────────────────────
//
// State is laid out column-major (FIPS 197 §3.4): index = row + 4*col.

void SubBytes(std::uint8_t* s) {
    for (int i = 0; i < 16; ++i) s[i] = kSbox[s[i]];
}

void InvSubBytes(std::uint8_t* s) {
    for (int i = 0; i < 16; ++i) s[i] = kInvSbox[s[i]];
}

void ShiftRows(std::uint8_t* s) {
    std::uint8_t t;
    // Row 1: shift left by 1
    t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
    // Row 2: shift left by 2
    t = s[2];  s[2]  = s[10]; s[10] = t;
    t = s[6];  s[6]  = s[14]; s[14] = t;
    // Row 3: shift left by 3 (== shift right by 1)
    t = s[3]; s[3] = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = t;
}

void InvShiftRows(std::uint8_t* s) {
    std::uint8_t t;
    // Row 1: shift right by 1
    t = s[13]; s[13] = s[9]; s[9] = s[5]; s[5] = s[1]; s[1] = t;
    // Row 2: shift right by 2 (same as left 2)
    t = s[2];  s[2]  = s[10]; s[10] = t;
    t = s[6];  s[6]  = s[14]; s[14] = t;
    // Row 3: shift right by 3 (== shift left by 1)
    t = s[3]; s[3] = s[7]; s[7] = s[11]; s[11] = s[15]; s[15] = t;
}

// MixColumns: per column [a,b,c,d] -> [2a^3b^c^d, a^2b^3c^d,
// a^b^2c^3d, 3a^b^c^2d]. The compact xtime form below is the
// standard "fast" implementation (Joan Daemen / Vincent Rijmen).
void MixColumns(std::uint8_t* s) {
    for (int c = 0; c < 4; ++c) {
        std::uint8_t a0 = s[4*c+0], a1 = s[4*c+1], a2 = s[4*c+2], a3 = s[4*c+3];
        std::uint8_t t = std::uint8_t(a0 ^ a1 ^ a2 ^ a3);
        std::uint8_t u = a0;
        s[4*c+0] ^= std::uint8_t(t ^ Xtime(std::uint8_t(a0 ^ a1)));
        s[4*c+1] ^= std::uint8_t(t ^ Xtime(std::uint8_t(a1 ^ a2)));
        s[4*c+2] ^= std::uint8_t(t ^ Xtime(std::uint8_t(a2 ^ a3)));
        s[4*c+3] ^= std::uint8_t(t ^ Xtime(std::uint8_t(a3 ^ u)));
    }
}

// InvMixColumns: per column the inverse 4x4 matrix is
// {0e,0b,0d,09; 09,0e,0b,0d; 0d,09,0e,0b; 0b,0d,09,0e}.
void InvMixColumns(std::uint8_t* s) {
    for (int c = 0; c < 4; ++c) {
        std::uint8_t a0 = s[4*c+0], a1 = s[4*c+1], a2 = s[4*c+2], a3 = s[4*c+3];
        s[4*c+0] = std::uint8_t(GfMul(a0, 0x0E) ^ GfMul(a1, 0x0B) ^ GfMul(a2, 0x0D) ^ GfMul(a3, 0x09));
        s[4*c+1] = std::uint8_t(GfMul(a0, 0x09) ^ GfMul(a1, 0x0E) ^ GfMul(a2, 0x0B) ^ GfMul(a3, 0x0D));
        s[4*c+2] = std::uint8_t(GfMul(a0, 0x0D) ^ GfMul(a1, 0x09) ^ GfMul(a2, 0x0E) ^ GfMul(a3, 0x0B));
        s[4*c+3] = std::uint8_t(GfMul(a0, 0x0B) ^ GfMul(a1, 0x0D) ^ GfMul(a2, 0x09) ^ GfMul(a3, 0x0E));
    }
}

void AddRoundKey(std::uint8_t* s, const std::uint8_t* rk) {
    for (int i = 0; i < 16; ++i) s[i] ^= rk[i];
}

// ──────────────────────────────────────────────────────────────────────────
// Key expansion (FIPS 197 §5.2)
// ──────────────────────────────────────────────────────────────────────────
//
// Nk = key length in 32-bit words (4 for AES-128, 8 for AES-256).
// Nr = number of rounds (10 for AES-128, 14 for AES-256).
// Output: (Nr+1) * 16 round-key bytes packed contiguously.

void ExpandKey(const std::uint8_t* key, int nk, std::uint8_t* round_keys) {
    const int total_words = 4 * (nk == 4 ? 11 : 15);  // (Nr+1)*4
    std::memcpy(round_keys, key, std::size_t(nk) * 4u);
    for (int i = nk; i < total_words; ++i) {
        std::uint8_t t[4];
        std::memcpy(t, round_keys + (i - 1) * 4, 4);
        if (i % nk == 0) {
            // RotWord + SubWord + XOR Rcon[i/Nk] on byte 0.
            std::uint8_t tmp = t[0];
            t[0] = kSbox[t[1]];
            t[1] = kSbox[t[2]];
            t[2] = kSbox[t[3]];
            t[3] = kSbox[tmp];
            t[0] ^= kRcon[i / nk];
        } else if (nk > 6 && i % nk == 4) {
            // AES-256 only: extra SubWord at i % Nk == 4.
            t[0] = kSbox[t[0]];
            t[1] = kSbox[t[1]];
            t[2] = kSbox[t[2]];
            t[3] = kSbox[t[3]];
        }
        for (int b = 0; b < 4; ++b) {
            round_keys[i*4 + b] = std::uint8_t(round_keys[(i - nk)*4 + b] ^ t[b]);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Block encrypt / decrypt
// ──────────────────────────────────────────────────────────────────────────

void EncryptBlock(std::uint8_t* state, const std::uint8_t* round_keys, int nr) {
    AddRoundKey(state, round_keys);
    for (int r = 1; r < nr; ++r) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, round_keys + r * 16);
    }
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, round_keys + nr * 16);
}

void DecryptBlock(std::uint8_t* state, const std::uint8_t* round_keys, int nr) {
    AddRoundKey(state, round_keys + nr * 16);
    for (int r = nr - 1; r >= 1; --r) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, round_keys + r * 16);
        InvMixColumns(state);
    }
    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, round_keys);
}

// ──────────────────────────────────────────────────────────────────────────
// Validation helpers
// ──────────────────────────────────────────────────────────────────────────

void ValidateKey(std::span<const std::byte> key, int& nk_out, int& nr_out) {
    if (key.size() == 16) {
        nk_out = 4; nr_out = 10;  // AES-128
    } else if (key.size() == 32) {
        nk_out = 8; nr_out = 14;  // AES-256
    } else {
        throw std::runtime_error(
            "aes_cbc: key size must be 16 (AES-128) or 32 (AES-256)");
    }
}

void ValidateIv(std::span<const std::byte> iv) {
    if (iv.size() != 16) {
        throw std::runtime_error(
            "aes_cbc: iv must be exactly 16 bytes");
    }
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────

std::vector<std::byte> Encrypt(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data) {
    int nk = 0, nr = 0;
    ValidateKey(key, nk, nr);
    ValidateIv(iv);

    // PKCS#7 padding (RFC 5652): pad byte = 16 - (len % 16),
    // appended (16 - len%16) times. Always at least one byte;
    // a block-aligned plaintext gets a full 16-byte padding
    // block.
    const std::size_t pad = 16u - (data.size() % 16u);
    const std::size_t out_len = data.size() + pad;

    std::uint8_t round_keys[240];
    ExpandKey(reinterpret_cast<const std::uint8_t*>(key.data()), nk, round_keys);

    std::vector<std::byte> out(out_len);
    std::uint8_t* op = reinterpret_cast<std::uint8_t*>(out.data());
    const std::uint8_t* ip = reinterpret_cast<const std::uint8_t*>(data.data());

    std::uint8_t prev[16];
    std::memcpy(prev, iv.data(), 16);

    const std::size_t full_blocks = data.size() / 16u;
    for (std::size_t b = 0; b < full_blocks; ++b) {
        std::uint8_t block[16];
        for (int i = 0; i < 16; ++i) block[i] = std::uint8_t(ip[b*16 + i] ^ prev[i]);
        EncryptBlock(block, round_keys, nr);
        std::memcpy(op + b*16, block, 16);
        std::memcpy(prev, block, 16);
    }

    // Tail block (always emitted, even when input is block-aligned).
    std::uint8_t tail[16];
    const std::size_t rem = data.size() - full_blocks * 16u;
    for (std::size_t i = 0; i < rem; ++i) tail[i] = ip[full_blocks * 16u + i];
    for (std::size_t i = rem; i < 16; ++i) tail[i] = std::uint8_t(pad);
    for (int i = 0; i < 16; ++i) tail[i] ^= prev[i];
    EncryptBlock(tail, round_keys, nr);
    std::memcpy(op + full_blocks * 16u, tail, 16);

    return out;
}

std::vector<std::byte> Decrypt(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data) {
    int nk = 0, nr = 0;
    ValidateKey(key, nk, nr);
    ValidateIv(iv);

    if (data.empty() || data.size() % 16u != 0) {
        throw std::runtime_error(
            "aes_cbc: ciphertext length must be a positive multiple of 16");
    }

    std::uint8_t round_keys[240];
    ExpandKey(reinterpret_cast<const std::uint8_t*>(key.data()), nk, round_keys);

    std::vector<std::byte> out(data.size());
    std::uint8_t* op = reinterpret_cast<std::uint8_t*>(out.data());
    const std::uint8_t* ip = reinterpret_cast<const std::uint8_t*>(data.data());

    std::uint8_t prev[16];
    std::memcpy(prev, iv.data(), 16);

    const std::size_t blocks = data.size() / 16u;
    for (std::size_t b = 0; b < blocks; ++b) {
        std::uint8_t block[16];
        std::memcpy(block, ip + b*16, 16);
        std::uint8_t saved[16];
        std::memcpy(saved, block, 16);
        DecryptBlock(block, round_keys, nr);
        for (int i = 0; i < 16; ++i) op[b*16 + i] = std::uint8_t(block[i] ^ prev[i]);
        std::memcpy(prev, saved, 16);
    }

    // PKCS#7 strip. Last byte must be a valid pad value (1..16);
    // every byte in the trailing pad must equal the pad value.
    const std::uint8_t pad = std::uint8_t(out.back());
    if (pad < 1 || pad > 16) {
        throw std::runtime_error("aes_cbc: invalid PKCS#7 padding");
    }
    for (std::size_t i = 0; i < pad; ++i) {
        if (op[out.size() - 1 - i] != pad) {
            throw std::runtime_error("aes_cbc: invalid PKCS#7 padding");
        }
    }
    out.resize(out.size() - pad);
    return out;
}

// No-padding CBC variants — same key-size dispatch, same IV
// rules, same CBC chaining as Encrypt/Decrypt, but the input
// must already be block-aligned and no padding byte is
// emitted or stripped. Used by encrypt_parser's Algorithm
// 2.A and 2.B paths; calling Decrypt() on /UE or /OE would
// throw on the missing PKCS#7 padding.
std::vector<std::byte> EncryptNoPad(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data) {
    int nk = 0, nr = 0;
    ValidateKey(key, nk, nr);
    ValidateIv(iv);

    if (data.empty() || data.size() % 16u != 0) {
        throw std::runtime_error(
            "aes_cbc: EncryptNoPad input length must be a positive multiple of 16");
    }

    std::uint8_t round_keys[240];
    ExpandKey(reinterpret_cast<const std::uint8_t*>(key.data()), nk, round_keys);

    std::vector<std::byte> out(data.size());
    std::uint8_t* op = reinterpret_cast<std::uint8_t*>(out.data());
    const std::uint8_t* ip = reinterpret_cast<const std::uint8_t*>(data.data());

    std::uint8_t prev[16];
    std::memcpy(prev, iv.data(), 16);

    const std::size_t blocks = data.size() / 16u;
    for (std::size_t b = 0; b < blocks; ++b) {
        std::uint8_t block[16];
        for (int i = 0; i < 16; ++i) block[i] = std::uint8_t(ip[b*16 + i] ^ prev[i]);
        EncryptBlock(block, round_keys, nr);
        std::memcpy(op + b*16, block, 16);
        std::memcpy(prev, block, 16);
    }
    return out;
}

std::vector<std::byte> DecryptNoPad(
    std::span<const std::byte> key,
    std::span<const std::byte> iv,
    std::span<const std::byte> data) {
    int nk = 0, nr = 0;
    ValidateKey(key, nk, nr);
    ValidateIv(iv);

    if (data.empty() || data.size() % 16u != 0) {
        throw std::runtime_error(
            "aes_cbc: DecryptNoPad input length must be a positive multiple of 16");
    }

    std::uint8_t round_keys[240];
    ExpandKey(reinterpret_cast<const std::uint8_t*>(key.data()), nk, round_keys);

    std::vector<std::byte> out(data.size());
    std::uint8_t* op = reinterpret_cast<std::uint8_t*>(out.data());
    const std::uint8_t* ip = reinterpret_cast<const std::uint8_t*>(data.data());

    std::uint8_t prev[16];
    std::memcpy(prev, iv.data(), 16);

    const std::size_t blocks = data.size() / 16u;
    for (std::size_t b = 0; b < blocks; ++b) {
        std::uint8_t block[16];
        std::memcpy(block, ip + b*16, 16);
        std::uint8_t saved[16];
        std::memcpy(saved, block, 16);
        DecryptBlock(block, round_keys, nr);
        for (int i = 0; i < 16; ++i) op[b*16 + i] = std::uint8_t(block[i] ^ prev[i]);
        std::memcpy(prev, saved, 16);
    }
    return out;
}

// Gate-only wrappers — bake in a 16-byte test key + zero IV.
static constexpr std::byte kGateTestKey[16] = {
    std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
    std::byte{0x04}, std::byte{0x05}, std::byte{0x06}, std::byte{0x07},
    std::byte{0x08}, std::byte{0x09}, std::byte{0x0A}, std::byte{0x0B},
    std::byte{0x0C}, std::byte{0x0D}, std::byte{0x0E}, std::byte{0x0F}
};
static constexpr std::byte kGateTestIv[16] = {
    std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
    std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
    std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
    std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}
};

std::vector<std::byte> Encode(std::span<const std::byte> data) {
    return Encrypt(std::span<const std::byte>{kGateTestKey, 16},
                   std::span<const std::byte>{kGateTestIv, 16}, data);
}

std::vector<std::byte> Decode(std::span<const std::byte> data) {
    return Decrypt(std::span<const std::byte>{kGateTestKey, 16},
                   std::span<const std::byte>{kGateTestIv, 16}, data);
}

}  // namespace foundation::aes_cbc
