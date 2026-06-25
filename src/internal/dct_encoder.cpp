#include "dct_encoder.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::dct_encoder {

namespace {

// ──────────────────────────────────────────────────────────────────────────
// Marker bytes (T.81 Table B.1)
// ──────────────────────────────────────────────────────────────────────────

constexpr std::uint8_t kSOI  = 0xD8;
constexpr std::uint8_t kEOI  = 0xD9;
constexpr std::uint8_t kSOF0 = 0xC0;
constexpr std::uint8_t kDHT  = 0xC4;
constexpr std::uint8_t kDQT  = 0xDB;
constexpr std::uint8_t kSOS  = 0xDA;

// JPEG zig-zag scan order (T.81 Figure A.6): natural row-major
// index → zig-zag scan index. Used to permute the 64-coefficient
// block at quantisation time so AC coefficients are emitted in
// the order T.81 §F.1.4 expects.
constexpr std::array<std::uint8_t, 64> kZigZag = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

// ──────────────────────────────────────────────────────────────────────────
// Annex K reference tables
// ──────────────────────────────────────────────────────────────────────────
//
// Stored in natural row-major order [u*8+v]; the encoder permutes
// to zig-zag at DQT-emit and AC-encode time.

// T.81 Annex K.1 — luminance quantisation table.
constexpr std::array<std::uint8_t, 64> kLumQuant = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
};

// T.81 Annex K.2 — chrominance quantisation table.
constexpr std::array<std::uint8_t, 64> kChromQuant = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

// T.81 Annex K.3 — DC luminance Huffman code lengths and values.
// BITS[i] (1-indexed, 16 entries) = number of codes of length i.
constexpr std::array<std::uint8_t, 16> kDcLumBits = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};
constexpr std::array<std::uint8_t, 12> kDcLumVals = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

// T.81 Annex K.5 — DC chrominance Huffman.
constexpr std::array<std::uint8_t, 16> kDcChromBits = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};
constexpr std::array<std::uint8_t, 12> kDcChromVals = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

// T.81 Annex K.4 — AC luminance Huffman.
constexpr std::array<std::uint8_t, 16> kAcLumBits = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7D
};
constexpr std::array<std::uint8_t, 162> kAcLumVals = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};

// T.81 Annex K.6 — AC chrominance Huffman.
constexpr std::array<std::uint8_t, 16> kAcChromBits = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77
};
constexpr std::array<std::uint8_t, 162> kAcChromVals = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
    0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
    0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};

[[noreturn]] void Fail(const char* msg) {
    throw std::runtime_error(std::string("dct_encoder: ") + msg);
}

// ──────────────────────────────────────────────────────────────────────────
// Huffman encode table built from BITS + HUFFVAL (T.81 Annex C)
// ──────────────────────────────────────────────────────────────────────────
//
// Indexed by symbol value (HUFFVAL byte). Non-present symbols
// have size==0 and must never be looked up; the encoder only
// emits symbols that the standard tables define.

struct EncodeTable {
    std::array<std::uint16_t, 256> code{};
    std::array<std::uint8_t, 256>  size{};
};

EncodeTable BuildEncodeTable(std::span<const std::uint8_t> bits,
                             std::span<const std::uint8_t> huffval) {
    // Generate huffsize and huffcode per Annex C.
    std::array<std::uint8_t, 257> huffsize{};
    std::size_t k = 0;
    for (int l = 1; l <= 16; ++l) {
        for (int j = 0; j < bits[l - 1]; ++j) {
            huffsize[k++] = std::uint8_t(l);
        }
    }
    huffsize[k] = 0;
    if (k != huffval.size()) {
        Fail("Huffman: BITS sum != HUFFVAL length");
    }

    std::array<std::uint16_t, 257> huffcode{};
    if (k > 0) {
        std::uint16_t code = 0;
        std::uint8_t si = huffsize[0];
        std::size_t p = 0;
        while (huffsize[p] != 0) {
            while (huffsize[p] == si) {
                huffcode[p++] = code;
                ++code;
            }
            if (huffsize[p] == 0) break;
            do { code = std::uint16_t(code << 1); ++si; }
                while (huffsize[p] != si);
        }
    }

    EncodeTable t;
    for (std::size_t i = 0; i < k; ++i) {
        std::uint8_t v = huffval[i];
        t.code[v] = huffcode[i];
        t.size[v] = huffsize[i];
    }
    return t;
}

// ──────────────────────────────────────────────────────────────────────────
// IJG quality scaling (T.81 doesn't standardise this — it's the
// de-facto convention shipped by libjpeg / libjpeg-turbo and
// consumed by every major JPEG decoder)
// ──────────────────────────────────────────────────────────────────────────

std::array<std::uint8_t, 64>
ScaleQuant(std::span<const std::uint8_t, 64> reference,
           std::uint8_t quality) {
    // Clamp to [1, 100]: quality=0 would divide by zero in the
    // q<50 branch; libjpeg clamps to 1. quality=100 → scale=0,
    // every entry rounds to 1 (unit table).
    int q = quality;
    if (q < 1) q = 1;
    if (q > 100) q = 100;
    int scale = (q < 50) ? (5000 / q) : (200 - q * 2);

    std::array<std::uint8_t, 64> out{};
    for (int i = 0; i < 64; ++i) {
        int v = (int(reference[i]) * scale + 50) / 100;
        if (v < 1) v = 1;
        if (v > 255) v = 255;
        out[i] = std::uint8_t(v);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// Forward DCT (T.81 §A.3.3)
// ──────────────────────────────────────────────────────────────────────────
//
// Direct float-precision separable 2-D DCT. Mirrors the IDCT in
// foundation::dctdecode (row-column decomposition with Cu folded
// into the cosine coefficients). The encoder applies the level
// shift (subtract 128) before the DCT to bring 8-bit unsigned
// samples into the signed [-128, 127] range required by §A.3.1.

struct CosineTable {
    std::array<std::array<float, 8>, 8> v{};  // v[u][x] = C(u) * cos((2x+1)uπ/16)
};

const CosineTable& Cos() {
    static const CosineTable t = []{
        CosineTable r;
        const float pi = 3.14159265358979323846f;
        const float invsqrt2 = 1.0f / std::sqrt(2.0f);
        for (int u = 0; u < 8; ++u) {
            float cu = (u == 0) ? invsqrt2 : 1.0f;
            for (int x = 0; x < 8; ++x) {
                r.v[u][x] = cu * std::cos((2.0f * float(x) + 1.0f)
                                         * float(u) * pi / 16.0f);
            }
        }
        return r;
    }();
    return t;
}

// Forward 8x8 DCT. ``in`` is 64 signed samples (already level-
// shifted to [-128, 127]); ``out`` receives 64 coefficients in
// natural row-major order.
void ForwardDCT8x8(const float* in, float* out) {
    const auto& c = Cos();
    float tmp[64];
    // Row pass: tmp[y*8 + u] = sum_x C(u)cos((2x+1)uπ/16) * in[y*8 + x]
    for (int y = 0; y < 8; ++y) {
        for (int u = 0; u < 8; ++u) {
            float s = 0.0f;
            for (int x = 0; x < 8; ++x) s += c.v[u][x] * in[y * 8 + x];
            tmp[y * 8 + u] = s;
        }
    }
    // Column pass: out[v*8 + u] = sum_y C(v)cos((2y+1)vπ/16) * tmp[y*8 + u]
    // T.81 §A.3.3 has a leading factor of 1/4 once Cu*Cv are folded
    // into the cosine table; in our split, each pass contributes
    // a 1/2 (the C(0) = 1/√2 row/col entry plus the implicit 1/2
    // from cos-sum normalisation), giving the same 1/4 overall.
    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            float s = 0.0f;
            for (int y = 0; y < 8; ++y) s += c.v[v][y] * tmp[y * 8 + u];
            out[v * 8 + u] = s * 0.25f;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Bit writer with byte-stuffing (T.81 §F.1.2.3)
// ──────────────────────────────────────────────────────────────────────────
//
// MSB-first bit accumulator. On each complete output byte, the
// 0xFF byte-stuffing rule is applied: a 0xFF data byte is
// followed by a 0x00 stuffing byte to distinguish it from a
// marker prefix. The Flush() call emits the trailing partial
// byte (if any), padded with 1-bits per §F.1.2.3.

class BitWriter {
public:
    explicit BitWriter(std::vector<std::byte>& out) : out_(out) {}

    // Append ``n`` low bits of ``v`` to the stream, MSB-first.
    // n is 0..16 in baseline JPEG (DC magnitudes up to 11 bits,
    // AC magnitudes up to 10 bits, plus Huffman codes up to 16).
    void Put(std::uint32_t v, int n) {
        if (n == 0) return;
        buf_ = (buf_ << n) | (v & ((std::uint32_t(1) << n) - 1u));
        bits_ += n;
        while (bits_ >= 8) {
            std::uint8_t b = std::uint8_t((buf_ >> (bits_ - 8)) & 0xFFu);
            bits_ -= 8;
            out_.push_back(std::byte(b));
            if (b == 0xFF) {
                out_.push_back(std::byte(0));  // T.81 §F.1.2.3
            }
        }
    }

    // Emit the trailing partial byte padded with 1-bits, then
    // reset state. Per §F.1.2.3 the 1-pad means the padding
    // bits never look like an EOB-prefix half-byte.
    void Flush() {
        if (bits_ > 0) {
            int pad = 8 - bits_;
            buf_ = (buf_ << pad) | ((std::uint32_t(1) << pad) - 1u);
            bits_ = 8;
            std::uint8_t b = std::uint8_t(buf_ & 0xFFu);
            bits_ = 0;
            buf_ = 0;
            out_.push_back(std::byte(b));
            if (b == 0xFF) out_.push_back(std::byte(0));
        }
    }

private:
    std::vector<std::byte>& out_;
    std::uint32_t buf_ = 0;
    int bits_ = 0;
};

// ──────────────────────────────────────────────────────────────────────────
// Marker emit helpers
// ──────────────────────────────────────────────────────────────────────────

void PushByte(std::vector<std::byte>& out, std::uint8_t b) {
    out.push_back(std::byte(b));
}

void PushU16Be(std::vector<std::byte>& out, std::uint16_t v) {
    out.push_back(std::byte(std::uint8_t(v >> 8)));
    out.push_back(std::byte(std::uint8_t(v & 0xFFu)));
}

void EmitMarker(std::vector<std::byte>& out, std::uint8_t marker) {
    PushByte(out, 0xFF);
    PushByte(out, marker);
}

// SOI / EOI are markers without a length-prefixed segment.

// DQT segment. Pq (precision) = 0 (8-bit). Tq (table id) selects
// destination 0 (luma) or 1 (chroma). Both tables in one DQT
// segment when Rgb; just luma when Grayscale. T.81 §B.2.4.1.
void EmitDQT(std::vector<std::byte>& out,
             std::span<const std::uint8_t, 64> luma,
             const std::array<std::uint8_t, 64>* chroma) {
    // Length = 2 (length field itself) + 65 per table (1 byte
    // Pq/Tq + 64 bytes values).
    int tables = chroma ? 2 : 1;
    std::uint16_t len = std::uint16_t(2 + tables * 65);
    EmitMarker(out, kDQT);
    PushU16Be(out, len);

    // Table 0 — luminance.
    PushByte(out, 0x00);  // Pq=0, Tq=0
    for (int i = 0; i < 64; ++i) PushByte(out, luma[kZigZag[i]]);

    if (chroma) {
        // Table 1 — chrominance.
        PushByte(out, 0x01);  // Pq=0, Tq=1
        for (int i = 0; i < 64; ++i) PushByte(out, (*chroma)[kZigZag[i]]);
    }
}

// SOF0 segment — baseline sequential, 8-bit precision. T.81
// §B.2.2. Components carry id / sampling factors / quant table
// selector. For 4:4:4 every component is 1×1; Y uses quant 0,
// Cb / Cr use quant 1.
void EmitSOF0(std::vector<std::byte>& out,
              std::uint16_t width, std::uint16_t height,
              bool grayscale) {
    int nf = grayscale ? 1 : 3;
    std::uint16_t len = std::uint16_t(8 + 3 * nf);
    EmitMarker(out, kSOF0);
    PushU16Be(out, len);
    PushByte(out, 8);  // sample precision
    PushU16Be(out, height);
    PushU16Be(out, width);
    PushByte(out, std::uint8_t(nf));

    if (grayscale) {
        PushByte(out, 1);     // component id
        PushByte(out, 0x11);  // sampling factor 1×1
        PushByte(out, 0);     // quant table id
    } else {
        PushByte(out, 1); PushByte(out, 0x11); PushByte(out, 0);  // Y
        PushByte(out, 2); PushByte(out, 0x11); PushByte(out, 1);  // Cb
        PushByte(out, 3); PushByte(out, 0x11); PushByte(out, 1);  // Cr
    }
}

// DHT segment. One segment carries 4 tables for Rgb (DC-Y,
// AC-Y, DC-C, AC-C) or 2 tables for Grayscale (DC-Y, AC-Y).
// Tc=0 selects DC-class, Tc=1 selects AC-class; Th picks the
// destination slot (0 = luma, 1 = chroma). T.81 §B.2.4.2.
void EmitDHT(std::vector<std::byte>& out, bool grayscale) {
    int tables = grayscale ? 2 : 4;
    std::uint16_t len = 2;
    for (int t = 0; t < tables; ++t) {
        // 1 byte Tc/Th + 16 bytes BITS + N bytes HUFFVAL.
        switch (t) {
            case 0: len = std::uint16_t(len + 1 + 16 + kDcLumVals.size()); break;
            case 1: len = std::uint16_t(len + 1 + 16 + kAcLumVals.size()); break;
            case 2: len = std::uint16_t(len + 1 + 16 + kDcChromVals.size()); break;
            case 3: len = std::uint16_t(len + 1 + 16 + kAcChromVals.size()); break;
        }
    }

    EmitMarker(out, kDHT);
    PushU16Be(out, len);

    auto write_table = [&](std::uint8_t tc_th,
                           std::span<const std::uint8_t> bits,
                           std::span<const std::uint8_t> vals) {
        PushByte(out, tc_th);
        for (auto b : bits) PushByte(out, b);
        for (auto v : vals) PushByte(out, v);
    };

    write_table(0x00, kDcLumBits, kDcLumVals);    // DC luma
    write_table(0x10, kAcLumBits, kAcLumVals);    // AC luma
    if (!grayscale) {
        write_table(0x01, kDcChromBits, kDcChromVals);  // DC chroma
        write_table(0x11, kAcChromBits, kAcChromVals);  // AC chroma
    }
}

// SOS segment — baseline scan over every component. Spectral
// selection 0..63, Ah=Al=0 (no successive approximation).
// T.81 §B.2.3.
void EmitSOS(std::vector<std::byte>& out, bool grayscale) {
    int ns = grayscale ? 1 : 3;
    std::uint16_t len = std::uint16_t(6 + 2 * ns);
    EmitMarker(out, kSOS);
    PushU16Be(out, len);
    PushByte(out, std::uint8_t(ns));
    if (grayscale) {
        PushByte(out, 1);     // Cs1 = Y component id
        PushByte(out, 0x00);  // Td=0 / Ta=0 (luma DC + AC tables)
    } else {
        PushByte(out, 1); PushByte(out, 0x00);  // Y: DC=0, AC=0
        PushByte(out, 2); PushByte(out, 0x11);  // Cb: DC=1, AC=1
        PushByte(out, 3); PushByte(out, 0x11);  // Cr: DC=1, AC=1
    }
    PushByte(out, 0x00);  // Ss=0
    PushByte(out, 0x3F);  // Se=63
    PushByte(out, 0x00);  // Ah=0, Al=0
}

// ──────────────────────────────────────────────────────────────────────────
// DC / AC coefficient encoding (T.81 §F.1.2.1, §F.1.2.2)
// ──────────────────────────────────────────────────────────────────────────

// SSSS magnitude category for a (signed) coefficient value.
// Category k spans [-(2^k - 1) .. -2^(k-1)] ∪ [2^(k-1) .. 2^k - 1].
// k=0 is the diff==0 case.
int Category(int v) {
    if (v == 0) return 0;
    int a = (v < 0) ? -v : v;
    int k = 0;
    while (a) { ++k; a >>= 1; }
    return k;
}

// Encoded magnitude bits per T.81 §F.1.2.1.3: the value is
// emitted as ``size`` raw bits MSB-first; for positive v the
// raw bits are simply the low ``size`` bits of v; for negative
// v the bias v + (1<<size) - 1 is used (one's-complement).
std::uint32_t MagBits(int v, int size) {
    if (size == 0) return 0;
    if (v >= 0) return std::uint32_t(v);
    return std::uint32_t(v + ((1 << size) - 1));
}

// Encode one DC coefficient (already differential-coded) as a
// (size Huffman code, raw magnitude bits) pair.
void EncodeDc(BitWriter& bw, int diff, const EncodeTable& dc) {
    int s = Category(diff);
    if (s > 11) Fail("DC: magnitude bits > 11");
    bw.Put(dc.code[s], dc.size[s]);
    if (s) bw.Put(MagBits(diff, s), s);
}

// Encode 63 AC coefficients in zig-zag order with run/size
// packing (T.81 §F.1.2.2). zz[1..63] are the AC band; zz[0] is
// DC and is encoded separately by EncodeDc.
void EncodeAc(BitWriter& bw,
              const std::array<int, 64>& zz,
              const EncodeTable& ac) {
    int run = 0;
    for (int k = 1; k < 64; ++k) {
        int v = zz[k];
        if (v == 0) {
            ++run;
            continue;
        }
        // ZRL for every full 16-zero run beyond a non-zero.
        while (run >= 16) {
            // ZRL: RRRR=15, SSSS=0 → packed byte 0xF0.
            bw.Put(ac.code[0xF0], ac.size[0xF0]);
            run -= 16;
        }
        int s = Category(v);
        if (s > 10) Fail("AC: magnitude bits > 10");
        std::uint8_t rs = std::uint8_t((run << 4) | s);
        bw.Put(ac.code[rs], ac.size[rs]);
        bw.Put(MagBits(v, s), s);
        run = 0;
    }
    if (run > 0) {
        // EOB: RRRR=0, SSSS=0 → packed byte 0x00.
        bw.Put(ac.code[0x00], ac.size[0x00]);
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Color conversion + level shift (per 8×8 block)
// ──────────────────────────────────────────────────────────────────────────
//
// JFIF Appendix A / BT.601:
//   Y  =  0.299 R + 0.587 G + 0.114 B
//   Cb = -0.168736 R - 0.331264 G + 0.5 B + 128
//   Cr =  0.5 R - 0.418688 G - 0.081312 B + 128
// Round to nearest. After conversion subtract 128 to put samples
// into the signed [-128, 127] range required by T.81 §A.3.1.

float Y_From_RGB(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

float Cb_From_RGB(float r, float g, float b) {
    return -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
}

float Cr_From_RGB(float r, float g, float b) {
    return 0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
}

// Encode one component: forward DCT, quantise, zigzag, emit
// DC + AC. dc_pred is the previous block's DC for this same
// component (initialised to 0 at scan start).
void EncodeBlock(BitWriter& bw,
                 const float* samples_signed,         // 64 signed samples
                 const std::array<std::uint8_t, 64>& q_natural,
                 const EncodeTable& dc, const EncodeTable& ac,
                 int& dc_pred) {
    float coefs[64];
    ForwardDCT8x8(samples_signed, coefs);

    // Quantise (round to nearest) into the zig-zag scan order:
    // for each scan index i, look up the natural-order position
    // kZigZag[i] in coefs/q_natural. After this, zz[0] is DC and
    // zz[1..63] are AC in T.81 §F.1.4 order.
    std::array<int, 64> zz{};
    for (int i = 0; i < 64; ++i) {
        std::size_t n = kZigZag[i];
        zz[i] = int(std::lroundf(coefs[n] / float(q_natural[n])));
    }

    int dc_val = zz[0];
    int diff = dc_val - dc_pred;
    dc_pred = dc_val;
    EncodeDc(bw, diff, dc);
    EncodeAc(bw, zz, ac);
}

// ──────────────────────────────────────────────────────────────────────────
// Frame encode
// ──────────────────────────────────────────────────────────────────────────

void EncodeGrayscale(std::vector<std::byte>& out,
                     std::uint32_t width, std::uint32_t height,
                     std::span<const std::byte> rgba8,
                     const std::array<std::uint8_t, 64>& q_lum,
                     const EncodeTable& dc_l, const EncodeTable& ac_l) {
    // Validate Grayscale precondition: R == G == B at every pixel.
    const std::uint8_t* px =
        reinterpret_cast<const std::uint8_t*>(rgba8.data());
    const std::size_t total = std::size_t(width) * height;
    for (std::size_t i = 0; i < total; ++i) {
        std::uint8_t r = px[i * 4 + 0];
        std::uint8_t g = px[i * 4 + 1];
        std::uint8_t b = px[i * 4 + 2];
        if (r != g || g != b) Fail("photometric=Grayscale requires R=G=B");
    }

    BitWriter bw(out);
    int dc_pred = 0;

    const std::uint32_t bx_count = (width  + 7u) / 8u;
    const std::uint32_t by_count = (height + 7u) / 8u;

    for (std::uint32_t by = 0; by < by_count; ++by) {
        for (std::uint32_t bx = 0; bx < bx_count; ++bx) {
            float blk[64];
            for (int y = 0; y < 8; ++y) {
                std::uint32_t src_y = by * 8u + std::uint32_t(y);
                if (src_y >= height) src_y = height - 1u;  // edge-clamp
                for (int x = 0; x < 8; ++x) {
                    std::uint32_t src_x = bx * 8u + std::uint32_t(x);
                    if (src_x >= width) src_x = width - 1u;
                    std::uint8_t v = px[(src_y * width + src_x) * 4u + 0];
                    blk[y * 8 + x] = float(v) - 128.0f;
                }
            }
            EncodeBlock(bw, blk, q_lum, dc_l, ac_l, dc_pred);
        }
    }
    bw.Flush();
}

void EncodeRgb(std::vector<std::byte>& out,
               std::uint32_t width, std::uint32_t height,
               std::span<const std::byte> rgba8,
               const std::array<std::uint8_t, 64>& q_lum,
               const std::array<std::uint8_t, 64>& q_chrom,
               const EncodeTable& dc_l, const EncodeTable& ac_l,
               const EncodeTable& dc_c, const EncodeTable& ac_c) {
    const std::uint8_t* px =
        reinterpret_cast<const std::uint8_t*>(rgba8.data());

    BitWriter bw(out);
    int dc_y = 0, dc_cb = 0, dc_cr = 0;

    const std::uint32_t bx_count = (width  + 7u) / 8u;
    const std::uint32_t by_count = (height + 7u) / 8u;

    // 4:4:4 sampling: every MCU contributes one block per
    // component, scanned in (Y, Cb, Cr) order. Block padding
    // replicates the last in-image column / row when the pixel
    // grid isn't a multiple of 8.
    for (std::uint32_t by = 0; by < by_count; ++by) {
        for (std::uint32_t bx = 0; bx < bx_count; ++bx) {
            float yblk[64], cbblk[64], crblk[64];
            for (int yy = 0; yy < 8; ++yy) {
                std::uint32_t src_y = by * 8u + std::uint32_t(yy);
                if (src_y >= height) src_y = height - 1u;
                for (int xx = 0; xx < 8; ++xx) {
                    std::uint32_t src_x = bx * 8u + std::uint32_t(xx);
                    if (src_x >= width) src_x = width - 1u;
                    const std::uint8_t* p = px + (src_y * width + src_x) * 4u;
                    float r = float(p[0]);
                    float g = float(p[1]);
                    float b = float(p[2]);
                    float Y  = Y_From_RGB (r, g, b);
                    float Cb = Cb_From_RGB(r, g, b);
                    float Cr = Cr_From_RGB(r, g, b);
                    int idx = yy * 8 + xx;
                    yblk [idx] = Y  - 128.0f;
                    cbblk[idx] = Cb - 128.0f;
                    crblk[idx] = Cr - 128.0f;
                }
            }
            EncodeBlock(bw, yblk,  q_lum,   dc_l, ac_l, dc_y);
            EncodeBlock(bw, cbblk, q_chrom, dc_c, ac_c, dc_cb);
            EncodeBlock(bw, crblk, q_chrom, dc_c, ac_c, dc_cr);
        }
    }
    bw.Flush();
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────
// Public Encode
// ──────────────────────────────────────────────────────────────────────────

std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options) {
    if (width == 0 || height == 0) {
        Fail("zero dimension");
    }
    if (width > 65535u || height > 65535u) {
        Fail("dimension > 65535 (JPEG SOF marker is 16-bit)");
    }
    const std::size_t expected = std::size_t(width) * height * 4u;
    if (rgba8.size() != expected) {
        Fail("rgba8 length != width * height * 4");
    }
    if (options.dpi == 0) {
        Fail("dpi must be positive");
    }
    if (options.photometric != Photometric::Grayscale
        && options.photometric != Photometric::Rgb) {
        Fail("photometric not in {Grayscale, Rgb}");
    }

    const bool grayscale = (options.photometric == Photometric::Grayscale);

    auto q_lum   = ScaleQuant(std::span<const std::uint8_t, 64>(kLumQuant),
                              options.quality);
    auto q_chrom = ScaleQuant(std::span<const std::uint8_t, 64>(kChromQuant),
                              options.quality);

    EncodeTable dc_l = BuildEncodeTable(kDcLumBits, kDcLumVals);
    EncodeTable ac_l = BuildEncodeTable(kAcLumBits, kAcLumVals);
    EncodeTable dc_c = BuildEncodeTable(kDcChromBits, kDcChromVals);
    EncodeTable ac_c = BuildEncodeTable(kAcChromBits, kAcChromVals);

    std::vector<std::byte> out;
    // Reserve a generous initial chunk — typical Q75 RGB output
    // is ~0.2-0.5 byte/pixel, so width*height suffices for most
    // images and avoids the first few reallocs.
    out.reserve(std::size_t(width) * height + 1024u);

    EmitMarker(out, kSOI);
    EmitDQT(out,
            std::span<const std::uint8_t, 64>(q_lum.data(), 64),
            grayscale ? nullptr : &q_chrom);
    EmitSOF0(out, std::uint16_t(width), std::uint16_t(height), grayscale);
    EmitDHT(out, grayscale);
    EmitSOS(out, grayscale);

    if (grayscale) {
        EncodeGrayscale(out, width, height, rgba8, q_lum, dc_l, ac_l);
    } else {
        EncodeRgb(out, width, height, rgba8, q_lum, q_chrom,
                  dc_l, ac_l, dc_c, ac_c);
    }

    EmitMarker(out, kEOI);
    return out;
}

}  // namespace foundation::dct_encoder
