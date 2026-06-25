#include "dctdecode.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::dctdecode {

namespace {

// ──────────────────────────────────────────────────────────────────────────
// Constants
// ──────────────────────────────────────────────────────────────────────────

// JPEG zig-zag scan order (T.81 Annex F.1.3): zig-zag index → row-major.
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

// JPEG marker codes (T.81 Table B.1). Marker bytes appear as 0xFF MM.
constexpr std::uint8_t kSOF0 = 0xC0;
constexpr std::uint8_t kSOF1 = 0xC1;
constexpr std::uint8_t kSOF2 = 0xC2;
constexpr std::uint8_t kSOF3 = 0xC3;
constexpr std::uint8_t kDHT  = 0xC4;
constexpr std::uint8_t kSOI  = 0xD8;
constexpr std::uint8_t kEOI  = 0xD9;
constexpr std::uint8_t kSOS  = 0xDA;
constexpr std::uint8_t kDQT  = 0xDB;
constexpr std::uint8_t kDRI  = 0xDD;
constexpr std::uint8_t kCOM  = 0xFE;
constexpr std::uint8_t kRST0 = 0xD0;
constexpr std::uint8_t kRST7 = 0xD7;
constexpr std::uint8_t kAPP0 = 0xE0;
constexpr std::uint8_t kAPP14 = 0xEE;  // "Adobe" colour-transform marker
constexpr std::uint8_t kAPP15 = 0xEF;

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("dctdecode: " + msg);
}

[[noreturn]] void Fail(const char* msg) {
    throw std::runtime_error(std::string("dctdecode: ") + msg);
}

// ──────────────────────────────────────────────────────────────────────────
// Marker stream — walks the byte stream a marker at a time
// ──────────────────────────────────────────────────────────────────────────

class MarkerStream {
public:
    MarkerStream(const std::byte* data, std::size_t size)
        : data_(reinterpret_cast<const std::uint8_t*>(data)), size_(size), pos_(0) {}

    // Find the next 0xFF marker prefix, skip fill 0xFFs, return the marker
    // byte. T.81 B.1.1.2 permits any number of 0xFF fill bytes between
    // markers; exactly one non-0xFF byte after the run is the marker.
    std::uint8_t NextMarker() {
        while (pos_ < size_ && data_[pos_] != 0xFF) ++pos_;
        if (pos_ >= size_) Fail("end of stream looking for marker");
        while (pos_ < size_ && data_[pos_] == 0xFF) ++pos_;
        if (pos_ >= size_) Fail("end of stream after 0xFF fill");
        return data_[pos_++];
    }

    // Read a 16-bit big-endian length prefix and return the segment payload
    // (bytes after the length field, length-2 bytes long).
    std::vector<std::uint8_t> ReadSegment() {
        if (pos_ + 2 > size_) Fail("truncated segment length");
        std::uint16_t len = (std::uint16_t(data_[pos_]) << 8) | data_[pos_+1];
        if (len < 2) Fail("segment length < 2");
        std::uint16_t payload = std::uint16_t(len - 2);
        pos_ += 2;
        if (pos_ + payload > size_) Fail("truncated segment payload");
        std::vector<std::uint8_t> out(data_ + pos_, data_ + pos_ + payload);
        pos_ += payload;
        return out;
    }

    void SkipSegment() { (void)ReadSegment(); }

    const std::uint8_t* Data() const { return data_; }
    std::size_t Size() const { return size_; }
    std::size_t Pos() const { return pos_; }
    void SetPos(std::size_t p) { pos_ = p; }

private:
    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t pos_;
};

// ──────────────────────────────────────────────────────────────────────────
// Quantization tables (DQT)
// ──────────────────────────────────────────────────────────────────────────

struct QuantTable {
    bool present = false;
    std::array<std::uint16_t, 64> values{};  // de-zigzagged (row-major slot)
};

void ReadDQT(const std::vector<std::uint8_t>& seg, std::array<QuantTable, 4>& tables) {
    std::size_t i = 0;
    while (i < seg.size()) {
        std::uint8_t pq_tq = seg[i++];
        std::uint8_t precision = std::uint8_t(pq_tq >> 4);
        std::uint8_t tq = std::uint8_t(pq_tq & 0x0F);
        if (precision > 1) Fail("DQT: bad precision");
        if (tq >= 4) Fail("DQT: table id >= 4");
        std::size_t entry = (precision == 0) ? 1u : 2u;
        if (i + 64 * entry > seg.size()) Fail("DQT: payload truncated");
        QuantTable& t = tables[tq];
        for (int k = 0; k < 64; ++k) {
            std::uint16_t v;
            if (precision == 0) { v = seg[i++]; }
            else { v = std::uint16_t((seg[i] << 8) | seg[i+1]); i += 2; }
            // Stream order is zig-zag; store at the row-major slot.
            t.values[kZigZag[k]] = v;
        }
        t.present = true;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Huffman tables (DHT) and canonical decode (T.81 Annex C + F.2.2)
// ──────────────────────────────────────────────────────────────────────────

struct HuffTable {
    bool present = false;
    std::array<std::uint8_t, 17> bits{};
    std::vector<std::uint8_t> huffval;
    std::array<std::int32_t, 17> mincode{};
    std::array<std::int32_t, 17> maxcode{};
    std::array<std::int32_t, 17> valptr{};

    // Annex C: Generate_size_table + Generate_code_table; then build the
    // mincode/maxcode/valptr per code-length used by the F.2.2.3 decoder.
    void Build() {
        std::array<std::uint8_t, 257> huffsize{};
        std::size_t k = 0;
        for (int i = 1; i <= 16; ++i) {
            for (int j = 0; j < bits[i]; ++j) {
                if (k >= 256) Fail("DHT: > 256 codes");
                huffsize[k++] = std::uint8_t(i);
            }
        }
        huffsize[k] = 0;
        std::array<std::uint32_t, 257> huffcode{};
        if (k > 0) {
            std::uint32_t code = 0;
            std::uint8_t si = huffsize[0];
            std::size_t p = 0;
            while (huffsize[p] != 0) {
                while (huffsize[p] == si) {
                    huffcode[p++] = code;
                    ++code;
                }
                if (huffsize[p] == 0) break;
                do { code <<= 1; ++si; } while (huffsize[p] != si);
            }
        }
        std::size_t j = 0;
        for (int l = 1; l <= 16; ++l) {
            if (bits[l] == 0) {
                maxcode[l] = -1;
            } else {
                valptr[l] = std::int32_t(j);
                mincode[l] = std::int32_t(huffcode[j]);
                j += bits[l];
                maxcode[l] = std::int32_t(huffcode[j - 1]);
            }
        }
    }
};

void ReadDHT(const std::vector<std::uint8_t>& seg,
             std::array<HuffTable, 4>& dc_tables,
             std::array<HuffTable, 4>& ac_tables) {
    std::size_t i = 0;
    while (i < seg.size()) {
        if (i + 17 > seg.size()) Fail("DHT: header truncated");
        std::uint8_t tc_th = seg[i++];
        std::uint8_t tc = std::uint8_t(tc_th >> 4);
        std::uint8_t th = std::uint8_t(tc_th & 0x0F);
        if (tc > 1 || th >= 4) Fail("DHT: bad table class/id");
        HuffTable& t = (tc == 0) ? dc_tables[th] : ac_tables[th];
        std::size_t total = 0;
        for (int l = 1; l <= 16; ++l) {
            t.bits[l] = seg[i++];
            total += t.bits[l];
        }
        if (total > 256) Fail("DHT: > 256 codes");
        if (i + total > seg.size()) Fail("DHT: HUFFVAL truncated");
        t.huffval.assign(seg.begin() + std::ptrdiff_t(i),
                         seg.begin() + std::ptrdiff_t(i + total));
        i += total;
        t.Build();
        t.present = true;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Bit reader for entropy-coded data
// ──────────────────────────────────────────────────────────────────────────
//
// Handles the 0xFF/0x00 byte-stuffing rule (T.81 F.1.2.3): inside an
// entropy segment, 0xFF data bytes are followed by a 0x00 stuffing byte
// to distinguish them from a marker prefix. Any 0xFF M with M ≠ 0x00
// terminates the entropy segment and is signalled to the caller via a
// pending marker; the underlying MarkerStream position is rewound to the
// 0xFF prefix so the outer parser can pick it up on the next
// MarkerStream::NextMarker() call.

class BitReader {
public:
    explicit BitReader(MarkerStream& ms)
        : ms_(ms), buf_(0), bits_held_(0), marker_(0) {}

    // Receive `n` bits (n in 0..16) as the low bits of an unsigned value.
    std::int32_t Receive(int n) {
        EnsureBits(n);
        std::int32_t v = std::int32_t((buf_ >> (bits_held_ - n))
                                      & ((std::uint32_t(1) << n) - 1));
        bits_held_ -= n;
        return v;
    }

    // Decode one Huffman symbol per Annex F.2.2.3.
    std::uint8_t DecodeHuff(const HuffTable& t) {
        EnsureBits(1);
        std::int32_t code = std::int32_t((buf_ >> (bits_held_ - 1)) & 1u);
        bits_held_ -= 1;
        for (int l = 1; l <= 16; ++l) {
            if (code <= t.maxcode[l]) {
                std::size_t idx = std::size_t(t.valptr[l] + (code - t.mincode[l]));
                if (idx >= t.huffval.size()) Fail("Huffman: bad index");
                return t.huffval[idx];
            }
            EnsureBits(1);
            code = (code << 1) | std::int32_t((buf_ >> (bits_held_ - 1)) & 1u);
            bits_held_ -= 1;
        }
        Fail("Huffman: code not found within 16 bits");
    }

    // Sign-extend per Annex F.1.2.1.1 (EXTEND).
    static std::int32_t Extend(std::int32_t v, int n) {
        if (n == 0) return 0;
        std::int32_t vt = std::int32_t(1) << (n - 1);
        if (v < vt) return v + (std::int32_t(-1) << n) + 1;
        return v;
    }

    // True once a non-RST marker has been encountered; entropy data is
    // exhausted and the caller should stop pulling bits.
    bool MarkerSeen() const { return marker_ != 0; }

    // Discard any partial-byte bits, validate that the next marker on the
    // stream is the expected RST(N), consume it, and reset internal state.
    // Used between restart-interval MCU groups.
    void ConsumeRestart(std::uint8_t expected_rst) {
        bits_held_ = 0;
        buf_ = 0;
        std::uint8_t m;
        if (marker_ != 0) {
            m = marker_;
            marker_ = 0;
            // Position the underlying stream past the marker bytes.
            // ms_.pos_ is parked at the 0xFF prefix; advance past it + fill.
            const std::uint8_t* d = ms_.Data();
            std::size_t p = ms_.Pos();
            if (p >= ms_.Size() || d[p] != 0xFF)
                Fail("ConsumeRestart: stream state inconsistent");
            ++p;
            while (p < ms_.Size() && d[p] == 0xFF) ++p;
            if (p >= ms_.Size() || d[p] != m)
                Fail("ConsumeRestart: marker disappeared");
            ms_.SetPos(p + 1);
        } else {
            m = ms_.NextMarker();
        }
        if (m != expected_rst) {
            Fail("expected RST" + std::to_string(expected_rst - kRST0)
                 + ", got marker 0x" + std::to_string(int(m)));
        }
    }

    void Reset() { buf_ = 0; bits_held_ = 0; marker_ = 0; }

private:
    void EnsureBits(int n) {
        while (bits_held_ < n) {
            if (marker_ != 0) Fail("entropy bits exhausted at marker");
            std::uint8_t b = ReadByte();
            buf_ = (buf_ << 8) | b;
            bits_held_ += 8;
        }
    }

    std::uint8_t ReadByte() {
        const std::uint8_t* d = ms_.Data();
        std::size_t p = ms_.Pos();
        if (p >= ms_.Size()) Fail("entropy stream truncated");
        std::uint8_t b = d[p];
        if (b == 0xFF) {
            std::size_t q = p + 1;
            while (q < ms_.Size() && d[q] == 0xFF) ++q;
            if (q >= ms_.Size()) Fail("end of stream after 0xFF in entropy");
            std::uint8_t next = d[q];
            if (next == 0x00) {
                // Stuffed 0xFF data byte; consume the 0x00 and emit 0xFF.
                ms_.SetPos(q + 1);
                return 0xFF;
            }
            // Real marker — leave pos at the 0xFF prefix and stash.
            marker_ = next;
            ms_.SetPos(p);
            return 0;
        }
        ms_.SetPos(p + 1);
        return b;
    }

    MarkerStream& ms_;
    std::uint32_t buf_;
    int bits_held_;
    std::uint8_t marker_;
};

// ──────────────────────────────────────────────────────────────────────────
// Frame header (SOF) and component info
// ──────────────────────────────────────────────────────────────────────────

struct ComponentInfo {
    std::uint8_t id = 0;
    std::uint8_t h = 0;
    std::uint8_t v = 0;
    std::uint8_t tq = 0;
    std::uint32_t blocks_x = 0;
    std::uint32_t blocks_y = 0;
    std::uint32_t width = 0;          // padded sample-plane width (blocks_x*8)
    std::uint32_t height = 0;         // padded sample-plane height (blocks_y*8)
    std::uint32_t actual_width = 0;   // ceil(frame.width  * h / max_h)
    std::uint32_t actual_height = 0;  // ceil(frame.height * v / max_v)
    std::vector<std::array<std::int16_t, 64>> coefs;  // row-major per-block
    std::vector<std::uint8_t> samples;                // post-IDCT, level-shifted
};

struct FrameInfo {
    std::uint8_t precision = 0;
    std::uint32_t height = 0;
    std::uint32_t width = 0;
    bool baseline = true;
    std::vector<ComponentInfo> components;
    std::uint32_t mcu_cols = 0;
    std::uint32_t mcu_rows = 0;
    std::uint32_t max_h = 0;
    std::uint32_t max_v = 0;
    // Adobe APP14 colour-transform hint (T.871 / Adobe TN 5116).
    // adobe = an "Adobe" APP14 segment was present; adobe_transform
    // is its transform byte: 0 = unknown (RGB or CMYK), 1 = YCbCr,
    // 2 = YCCK. Drives 4-component (CMYK/YCCK) reconstruction.
    bool adobe = false;
    int adobe_transform = -1;

    int FindComponent(std::uint8_t id) const {
        for (std::size_t i = 0; i < components.size(); ++i) {
            if (components[i].id == id) return int(i);
        }
        return -1;
    }
};

void ReadSOF(const std::vector<std::uint8_t>& seg, FrameInfo& f, bool baseline) {
    if (seg.size() < 6) Fail("SOF: header truncated");
    f.baseline = baseline;
    f.precision = seg[0];
    if (f.precision != 8) {
        Fail("SOF: unsupported sample precision " + std::to_string(int(f.precision)));
    }
    f.height = (std::uint32_t(seg[1]) << 8) | seg[2];
    f.width = (std::uint32_t(seg[3]) << 8) | seg[4];
    if (f.width == 0 || f.height == 0) Fail("SOF: zero dimension");
    if (f.width > 65535u || f.height > 65535u) Fail("SOF: oversized image");
    std::uint8_t nf = seg[5];
    // 1 = grayscale, 3 = YCbCr/RGB, 4 = CMYK / Adobe YCCK. The
    // 4-component frame decodes through the same per-component
    // machinery; AssembleRGBA converts it to RGB (Adobe APP14).
    if (nf != 1 && nf != 3 && nf != 4) {
        Fail("SOF: unsupported component count " + std::to_string(int(nf)));
    }
    if (seg.size() < std::size_t(6 + 3 * nf)) Fail("SOF: component table truncated");
    f.components.assign(nf, ComponentInfo{});
    f.max_h = 0;
    f.max_v = 0;
    for (int i = 0; i < nf; ++i) {
        ComponentInfo& c = f.components[i];
        c.id = seg[6 + 3*i];
        std::uint8_t hv = seg[7 + 3*i];
        c.h = std::uint8_t(hv >> 4);
        c.v = std::uint8_t(hv & 0x0F);
        c.tq = seg[8 + 3*i];
        if (c.h < 1 || c.h > 4) Fail("SOF: bad H sampling factor");
        if (c.v < 1 || c.v > 4) Fail("SOF: bad V sampling factor");
        if (c.tq >= 4) Fail("SOF: bad Tq");
        if (c.h > f.max_h) f.max_h = c.h;
        if (c.v > f.max_v) f.max_v = c.v;
    }
    // MCU width = max_h * 8; MCU height = max_v * 8.
    f.mcu_cols = (f.width  + f.max_h * 8u - 1u) / (f.max_h * 8u);
    f.mcu_rows = (f.height + f.max_v * 8u - 1u) / (f.max_v * 8u);
    for (auto& c : f.components) {
        c.blocks_x = f.mcu_cols * c.h;
        c.blocks_y = f.mcu_rows * c.v;
        c.width  = c.blocks_x * 8u;
        c.height = c.blocks_y * 8u;
        c.actual_width  = (f.width  * c.h + f.max_h - 1u) / f.max_h;
        c.actual_height = (f.height * c.v + f.max_v - 1u) / f.max_v;
        c.coefs.assign(std::size_t(c.blocks_x) * c.blocks_y,
                       std::array<std::int16_t, 64>{});
        c.samples.assign(std::size_t(c.width) * c.height, std::uint8_t(0));
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Scan header (SOS)
// ──────────────────────────────────────────────────────────────────────────

struct ScanComponent {
    std::uint8_t cs = 0;
    std::uint8_t td = 0;
    std::uint8_t ta = 0;
    int comp_index = -1;
};

struct ScanInfo {
    std::vector<ScanComponent> components;
    std::uint8_t ss = 0;
    std::uint8_t se = 63;
    std::uint8_t ah = 0;
    std::uint8_t al = 0;
};

void ReadSOS(const std::vector<std::uint8_t>& seg, const FrameInfo& f, ScanInfo& s) {
    if (seg.empty()) Fail("SOS: empty");
    std::uint8_t ns = seg[0];
    if (ns < 1 || ns > 4) Fail("SOS: bad component count");
    if (seg.size() < std::size_t(1 + 2 * ns + 3)) Fail("SOS: header truncated");
    s.components.assign(ns, ScanComponent{});
    for (int i = 0; i < ns; ++i) {
        ScanComponent& c = s.components[i];
        c.cs = seg[1 + 2*i];
        std::uint8_t tdta = seg[2 + 2*i];
        c.td = std::uint8_t(tdta >> 4);
        c.ta = std::uint8_t(tdta & 0x0F);
        if (c.td >= 4 || c.ta >= 4) Fail("SOS: bad table selector");
        c.comp_index = f.FindComponent(c.cs);
        if (c.comp_index < 0) Fail("SOS: unknown component selector");
    }
    std::size_t off = std::size_t(1 + 2 * ns);
    s.ss = seg[off];
    s.se = seg[off + 1];
    std::uint8_t ahal = seg[off + 2];
    s.ah = std::uint8_t(ahal >> 4);
    s.al = std::uint8_t(ahal & 0x0F);
    if (s.ss > 63 || s.se > 63 || s.ss > s.se) Fail("SOS: bad spectral selection");
}

// ──────────────────────────────────────────────────────────────────────────
// 8×8 inverse DCT
// ──────────────────────────────────────────────────────────────────────────
//
// Direct float-precision separable 1-D IDCT. T.81 Annex A.3.3 defines:
//     S(x,y) = 1/4 sum_{u,v} C(u)C(v) F(u,v) cos((2x+1)uπ/16) cos((2y+1)vπ/16)
//     where C(0) = 1/√2, C(k>0) = 1.
// We precompute a cosine table with C(u) folded in, apply the 1-D IDCT
// row-wise, then column-wise. Output is level-shifted by +128 and
// clamped to [0, 255]. Float precision agrees with libjpeg's ISLOW
// integer IDCT to ≥65 dB on natural images, well above the 50 dB gate
// floor.

struct CosineTable {
    std::array<std::array<float, 8>, 8> v{};  // v[x][u] = C(u) * cos((2x+1)uπ/16)
};

const CosineTable& Cos() {
    static const CosineTable t = []{
        CosineTable r;
        const float pi = 3.14159265358979323846f;
        const float invsqrt2 = 1.0f / std::sqrt(2.0f);
        for (int x = 0; x < 8; ++x) {
            for (int u = 0; u < 8; ++u) {
                float cu = (u == 0) ? invsqrt2 : 1.0f;
                r.v[x][u] = cu * std::cos((2.0f * float(x) + 1.0f) * float(u) * pi / 16.0f);
            }
        }
        return r;
    }();
    return t;
}

void InverseDCT8x8(const std::int16_t* coef, std::uint8_t* out, std::ptrdiff_t out_stride) {
    const auto& c = Cos();
    float tmp[64];
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            float s = 0.0f;
            for (int u = 0; u < 8; ++u) s += c.v[x][u] * float(coef[y * 8 + u]);
            tmp[y * 8 + x] = s * 0.5f;
        }
    }
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            float s = 0.0f;
            for (int v = 0; v < 8; ++v) s += c.v[y][v] * tmp[v * 8 + x];
            float p = s * 0.5f + 128.0f;
            int iv = int(std::lroundf(p));
            if (iv < 0) iv = 0; else if (iv > 255) iv = 255;
            out[y * out_stride + x] = std::uint8_t(iv);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Block decoders
// ──────────────────────────────────────────────────────────────────────────

// Baseline (SOF0): one full-coefficient pass per block per scan.
// Annex F.2.2.1 — DC differential then 63 AC coefficients in zig-zag.
void DecodeBaselineBlock(BitReader& br,
                         const HuffTable& dc_t, const HuffTable& ac_t,
                         std::int16_t* coef_rowmajor,
                         std::int16_t& dc_pred) {
    std::uint8_t s = br.DecodeHuff(dc_t);
    if (s > 11) Fail("DC: magnitude bits > 11");
    std::int32_t dv = BitReader::Extend(br.Receive(s), s);
    dc_pred = std::int16_t(dc_pred + dv);

    std::array<std::int16_t, 64> zz{};
    zz[0] = dc_pred;
    int k = 1;
    while (k < 64) {
        std::uint8_t rs = br.DecodeHuff(ac_t);
        std::uint8_t r = std::uint8_t(rs >> 4);
        std::uint8_t s_ac = std::uint8_t(rs & 0x0F);
        if (s_ac == 0) {
            if (r == 15) { k += 16; continue; }  // ZRL
            break;  // EOB
        }
        if (s_ac > 10) Fail("AC: magnitude bits > 10");
        k += r;
        if (k >= 64) Fail("AC: zig-zag overrun");
        std::int32_t av = BitReader::Extend(br.Receive(s_ac), s_ac);
        zz[k] = std::int16_t(av);
        ++k;
    }
    for (int i = 0; i < 64; ++i) coef_rowmajor[kZigZag[i]] = zz[i];
}

// Progressive scan state — EOBRUN is per-scan, per Annex G.1.2.2.
struct ProgState {
    std::uint32_t eob_run = 0;
};

void DecodeProgDcInitial(BitReader& br, const HuffTable& dc_t,
                         std::int16_t* coef_rowmajor, std::int16_t& dc_pred,
                         int al) {
    std::uint8_t s = br.DecodeHuff(dc_t);
    std::int32_t dv = BitReader::Extend(br.Receive(s), s);
    dc_pred = std::int16_t(dc_pred + dv);
    coef_rowmajor[0] = std::int16_t(dc_pred << al);
}

void DecodeProgDcRefine(BitReader& br, std::int16_t* coef_rowmajor, int al) {
    if (br.Receive(1)) {
        coef_rowmajor[0] = std::int16_t(coef_rowmajor[0] | (1 << al));
    }
}

// AC initial scan (Ah == 0): writes new non-zero coefficients into the
// band [Ss..Se] at bit position Al. Tracks a per-scan EOBRUN.
void DecodeProgAcInitial(BitReader& br, const HuffTable& ac_t,
                         int ss, int se, int al,
                         ProgState& ps,
                         std::int16_t* coef_rowmajor) {
    if (ps.eob_run > 0) { --ps.eob_run; return; }
    int k = ss;
    while (k <= se) {
        std::uint8_t rs = br.DecodeHuff(ac_t);
        std::uint8_t r = std::uint8_t(rs >> 4);
        std::uint8_t s_ac = std::uint8_t(rs & 0x0F);
        if (s_ac == 0) {
            if (r != 15) {
                std::uint32_t add = (r > 0) ? std::uint32_t(br.Receive(r)) : 0u;
                ps.eob_run = (std::uint32_t(1) << r) + add - 1u;
                return;
            }
            k += 16;  // ZRL
            continue;
        }
        k += r;
        if (k > se) Fail("Progressive AC initial: band overrun");
        std::int32_t av = BitReader::Extend(br.Receive(s_ac), s_ac);
        coef_rowmajor[kZigZag[k]] = std::int16_t(av << al);
        ++k;
    }
}

// AC refinement scan (Ah > 0). Per Annex G.1.2.3, each existing non-zero
// coefficient gets one corrective bit (added in the direction of its
// sign). New non-zero coefficients are introduced via SSSS=1 sequences.
// ZRL skips 16 zeros; EOB enters EOBRUN — but during EOBRUN, refinement
// bits still apply to existing non-zero coefs in the band.
void DecodeProgAcRefine(BitReader& br, const HuffTable& ac_t,
                        int ss, int se, int al,
                        ProgState& ps,
                        std::int16_t* coef_rowmajor) {
    const int p1 = 1 << al;
    const int m1 = -p1;

    auto ApplyRefine = [&](std::int16_t& c) {
        if (br.Receive(1)) {
            if (c > 0) c = std::int16_t(c + p1);
            else       c = std::int16_t(c + m1);
        }
    };

    int k = ss;
    if (ps.eob_run == 0) {
        while (k <= se) {
            std::uint8_t rs = br.DecodeHuff(ac_t);
            std::uint8_t r = std::uint8_t(rs >> 4);
            std::uint8_t s_ac = std::uint8_t(rs & 0x0F);
            std::int32_t newval = 0;
            if (s_ac == 0) {
                if (r != 15) {
                    std::uint32_t ext = (r > 0) ? std::uint32_t(br.Receive(r)) : 0u;
                    ps.eob_run = (std::uint32_t(1) << r) + ext;
                    break;
                }
                // ZRL: r becomes 16 — see zero-skip loop below.
            } else {
                if (s_ac != 1) Fail("AC refine: s != 0/1");
                newval = br.Receive(1) ? p1 : m1;
            }
            // Skip r zero coefficients (counting only zeros), refining
            // any non-zeros encountered along the way.
            while (true) {
                if (k > se) Fail("AC refine: band overrun");
                std::int16_t& c = coef_rowmajor[kZigZag[k]];
                if (c != 0) {
                    ApplyRefine(c);
                    ++k;
                } else {
                    if (r == 0) break;
                    --r;
                    ++k;
                }
            }
            if (s_ac == 1) {
                if (k > se) Fail("AC refine: band overrun placing new coef");
                coef_rowmajor[kZigZag[k]] = std::int16_t(newval);
                ++k;
            }
        }
    }
    if (ps.eob_run > 0) {
        // Apply refinement to non-zero coefs in remaining band.
        while (k <= se) {
            std::int16_t& c = coef_rowmajor[kZigZag[k]];
            if (c != 0) ApplyRefine(c);
            ++k;
        }
        --ps.eob_run;
    }
}

// ──────────────────────────────────────────────────────────────────────────
// MCU / scan iteration
// ──────────────────────────────────────────────────────────────────────────

// Walk MCUs row-major, invoking `block_fn(scan_idx, comp_index, block_x, block_y)`
// for each data unit. Handles restart markers between MCU groups.
template <class Fn>
void IterateScan(MarkerStream& ms, BitReader& br,
                 const FrameInfo& f, const ScanInfo& s,
                 std::uint32_t restart_interval,
                 Fn&& block_fn,
                 std::array<std::int16_t, 4>& dc_pred,
                 ProgState* ps_array /* one per scan-component, may be nullptr */) {
    const bool interleaved = s.components.size() > 1;
    std::uint32_t mcus = 0;
    std::uint32_t expected_rst = 0;
    // Non-interleaved (single component) iterates over that component's
    // entire block grid 1 data unit at a time, in row-major order — not
    // the frame's MCU grid (T.81 A.2.3).
    if (interleaved) {
        for (std::uint32_t my = 0; my < f.mcu_rows; ++my) {
            for (std::uint32_t mx = 0; mx < f.mcu_cols; ++mx) {
                for (std::size_t ci = 0; ci < s.components.size(); ++ci) {
                    const ScanComponent& sc = s.components[ci];
                    const ComponentInfo& comp = f.components[sc.comp_index];
                    for (int by = 0; by < comp.v; ++by) {
                        for (int bx = 0; bx < comp.h; ++bx) {
                            std::uint32_t bxg = mx * comp.h + bx;
                            std::uint32_t byg = my * comp.v + by;
                            block_fn(int(ci), sc, bxg, byg, dc_pred[ci],
                                     ps_array ? &ps_array[ci] : nullptr);
                        }
                    }
                }
                ++mcus;
                bool last = (my == f.mcu_rows - 1) && (mx == f.mcu_cols - 1);
                if (restart_interval > 0 && mcus % restart_interval == 0 && !last) {
                    br.ConsumeRestart(std::uint8_t(kRST0 + (expected_rst & 7)));
                    ++expected_rst;
                    br.Reset();
                    for (auto& d : dc_pred) d = 0;
                    if (ps_array) {
                        for (std::size_t i = 0; i < s.components.size(); ++i) {
                            ps_array[i] = ProgState{};
                        }
                    }
                }
            }
        }
    } else {
        const ScanComponent& sc = s.components[0];
        const ComponentInfo& comp = f.components[sc.comp_index];
        std::uint32_t total = comp.blocks_x * comp.blocks_y;
        for (std::uint32_t i = 0; i < total; ++i) {
            std::uint32_t bxg = i % comp.blocks_x;
            std::uint32_t byg = i / comp.blocks_x;
            block_fn(0, sc, bxg, byg, dc_pred[0],
                     ps_array ? &ps_array[0] : nullptr);
            ++mcus;
            bool last = (i == total - 1);
            if (restart_interval > 0 && mcus % restart_interval == 0 && !last) {
                br.ConsumeRestart(std::uint8_t(kRST0 + (expected_rst & 7)));
                ++expected_rst;
                br.Reset();
                for (auto& d : dc_pred) d = 0;
                if (ps_array) ps_array[0] = ProgState{};
            }
        }
    }
}

void DecodeBaselineScan(MarkerStream& ms, BitReader& br,
                        FrameInfo& f, const ScanInfo& s,
                        const std::array<HuffTable, 4>& dc_tables,
                        const std::array<HuffTable, 4>& ac_tables,
                        std::uint32_t restart_interval) {
    std::array<std::int16_t, 4> dc_pred{};
    IterateScan(ms, br, f, s, restart_interval,
        [&](int /*ci*/, const ScanComponent& sc,
            std::uint32_t bxg, std::uint32_t byg,
            std::int16_t& pred, ProgState* /*ps*/) {
            ComponentInfo& comp = f.components[sc.comp_index];
            std::int16_t* coef = comp.coefs[byg * comp.blocks_x + bxg].data();
            DecodeBaselineBlock(br, dc_tables[sc.td], ac_tables[sc.ta], coef, pred);
        },
        dc_pred, nullptr);
}

void DecodeProgressiveScan(MarkerStream& ms, BitReader& br,
                           FrameInfo& f, const ScanInfo& s,
                           const std::array<HuffTable, 4>& dc_tables,
                           const std::array<HuffTable, 4>& ac_tables,
                           std::uint32_t restart_interval) {
    std::array<std::int16_t, 4> dc_pred{};
    std::array<ProgState, 4> ps{};
    const bool dc_scan = (s.ss == 0);
    const bool refine  = (s.ah != 0);
    IterateScan(ms, br, f, s, restart_interval,
        [&](int /*ci*/, const ScanComponent& sc,
            std::uint32_t bxg, std::uint32_t byg,
            std::int16_t& pred, ProgState* psp) {
            ComponentInfo& comp = f.components[sc.comp_index];
            std::int16_t* coef = comp.coefs[byg * comp.blocks_x + bxg].data();
            if (dc_scan) {
                if (refine) DecodeProgDcRefine(br, coef, s.al);
                else        DecodeProgDcInitial(br, dc_tables[sc.td], coef, pred, s.al);
            } else {
                if (refine) DecodeProgAcRefine(br, ac_tables[sc.ta], s.ss, s.se, s.al, *psp, coef);
                else        DecodeProgAcInitial(br, ac_tables[sc.ta], s.ss, s.se, s.al, *psp, coef);
            }
        },
        dc_pred, ps.data());
}

// ──────────────────────────────────────────────────────────────────────────
// Finalize: dequantize + IDCT every block of every component
// ──────────────────────────────────────────────────────────────────────────

void Finalize(FrameInfo& f, const std::array<QuantTable, 4>& quant) {
    for (auto& comp : f.components) {
        const QuantTable& q = quant[comp.tq];
        if (!q.present) Fail("Finalize: quant table " + std::to_string(int(comp.tq)) + " missing");
        std::int16_t dq[64];
        for (std::uint32_t by = 0; by < comp.blocks_y; ++by) {
            for (std::uint32_t bx = 0; bx < comp.blocks_x; ++bx) {
                const auto& blk = comp.coefs[by * comp.blocks_x + bx];
                for (int k = 0; k < 64; ++k) {
                    dq[k] = std::int16_t(int(blk[k]) * int(q.values[k]));
                }
                std::uint8_t* out = comp.samples.data()
                                  + std::size_t(by * 8u) * comp.width
                                  + std::size_t(bx * 8u);
                InverseDCT8x8(dq, out, std::ptrdiff_t(comp.width));
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Upsample + color convert → RGBA8
// ──────────────────────────────────────────────────────────────────────────
//
// Chroma upsampling: bilinear with JFIF-centered chroma siting (JFIF
// Appendix A: "the 2x2 sample area for one chrominance sample is
// centered (vertically and horizontally) over the 2x2 sample area" of
// luma pixels). For h=v=1 with max_h=max_v=2 (4:2:0), this is the
// same 9/3/3/1 weighting as libjpeg's h2v2_fancy_upsample. Edge
// pixels clamp the chroma index, equivalent to extending the edge
// chroma sample.
//
// For each output luma pixel (out_x, out_y) at center (out_x+0.5,
// out_y+0.5) and a component with sampling factors (h, v), the
// chroma sample (cx, cy) sits at luma center coordinate
// (cx*max_h/h + max_h/(2h), cy*max_v/v + max_v/(2v)). Inverting:
//     cxf = (out_x + 0.5) * h / max_h - 0.5
//     cyf = (out_y + 0.5) * v / max_v - 0.5
// then bilinearly interpolate the 4 nearest chroma samples
// (cx0..cx0+1, cy0..cy0+1), clamped to [0, actual-1].
//
// Color conversion: BT.601 per JFIF Appendix A.
//     R = Y + 1.402   * (Cr - 128)
//     G = Y - 0.34414 * (Cb - 128) - 0.71414 * (Cr - 128)
//     B = Y + 1.772   * (Cb - 128)

float SampleBilinear(const ComponentInfo& c, float out_x, float out_y,
                     std::uint32_t max_h, std::uint32_t max_v) {
    float cxf = (out_x + 0.5f) * float(c.h) / float(max_h) - 0.5f;
    float cyf = (out_y + 0.5f) * float(c.v) / float(max_v) - 0.5f;
    int cx0 = int(std::floor(cxf));
    int cy0 = int(std::floor(cyf));
    float fx = cxf - float(cx0);
    float fy = cyf - float(cy0);
    int cx1 = cx0 + 1;
    int cy1 = cy0 + 1;
    int max_cx = int(c.actual_width)  - 1;
    int max_cy = int(c.actual_height) - 1;
    if (cx0 < 0) cx0 = 0; else if (cx0 > max_cx) cx0 = max_cx;
    if (cy0 < 0) cy0 = 0; else if (cy0 > max_cy) cy0 = max_cy;
    if (cx1 < 0) cx1 = 0; else if (cx1 > max_cx) cx1 = max_cx;
    if (cy1 < 0) cy1 = 0; else if (cy1 > max_cy) cy1 = max_cy;
    float v00 = float(c.samples[std::size_t(cy0) * c.width + std::size_t(cx0)]);
    float v10 = float(c.samples[std::size_t(cy0) * c.width + std::size_t(cx1)]);
    float v01 = float(c.samples[std::size_t(cy1) * c.width + std::size_t(cx0)]);
    float v11 = float(c.samples[std::size_t(cy1) * c.width + std::size_t(cx1)]);
    return (1.0f - fx) * (1.0f - fy) * v00
         +         fx  * (1.0f - fy) * v10
         + (1.0f - fx) *         fy  * v01
         +         fx  *         fy  * v11;
}

std::vector<std::byte> AssembleRGBA(const FrameInfo& f) {
    std::vector<std::byte> rgba(std::size_t(f.width) * f.height * 4);
    if (f.components.size() == 1) {
        const ComponentInfo& y = f.components[0];
        for (std::uint32_t row = 0; row < f.height; ++row) {
            const std::uint8_t* yrow = y.samples.data() + std::size_t(row) * y.width;
            std::byte* dst = rgba.data() + std::size_t(row) * f.width * 4u;
            for (std::uint32_t col = 0; col < f.width; ++col) {
                std::uint8_t v = yrow[col];
                dst[col*4 + 0] = std::byte(v);
                dst[col*4 + 1] = std::byte(v);
                dst[col*4 + 2] = std::byte(v);
                dst[col*4 + 3] = std::byte(0xFF);
            }
        }
        return rgba;
    }
    auto clamp255 = [](float v) -> int {
        int i = int(std::lroundf(v));
        return i < 0 ? 0 : (i > 255 ? 255 : i);
    };

    // 4-component frames are CMYK or Adobe YCCK (§dctdecode intent,
    // PDF /DCTDecode over a DeviceCMYK / ICCBased-4 image). Convert
    // to RGB so the decoder keeps its "always RGBA" output contract.
    if (f.components.size() >= 4) {
        const ComponentInfo& c0 = f.components[0];
        const ComponentInfo& c1 = f.components[1];
        const ComponentInfo& c2 = f.components[2];
        const ComponentInfo& c3 = f.components[3];
        const bool ycck = (f.adobe_transform == 2);
        for (std::uint32_t row = 0; row < f.height; ++row) {
            std::byte* dst = rgba.data() + std::size_t(row) * f.width * 4u;
            for (std::uint32_t col = 0; col < f.width; ++col) {
                const float s0 = SampleBilinear(
                    c0, float(col), float(row), f.max_h, f.max_v);
                const float s1 = SampleBilinear(
                    c1, float(col), float(row), f.max_h, f.max_v);
                const float s2 = SampleBilinear(
                    c2, float(col), float(row), f.max_h, f.max_v);
                const float s3 = SampleBilinear(
                    c3, float(col), float(row), f.max_h, f.max_v);
                // Recover C, M, Y, K in [0, 255].
                float C, M, Yk, K;
                if (ycck) {
                    // YCCK: first three are YCbCr of (255 - CMY);
                    // K is stored directly. CMY = 255 - (YCbCr→RGB).
                    const float Cb = s1 - 128.0f;
                    const float Cr = s2 - 128.0f;
                    C  = 255.0f - (s0 + 1.402f * Cr);
                    M  = 255.0f - (s0 - 0.34414f * Cb - 0.71414f * Cr);
                    Yk = 255.0f - (s0 + 1.772f * Cb);
                    K  = s3;
                } else if (f.adobe) {
                    // Adobe CMYK is stored inverted — de-invert.
                    C = 255.0f - s0; M = 255.0f - s1;
                    Yk = 255.0f - s2; K = 255.0f - s3;
                } else {
                    C = s0; M = s1; Yk = s2; K = s3;
                }
                // CMYK → RGB, subtractive (§8.6.4.4): chan =
                // (1 - c) * (1 - k), matching foundation::colorspace.
                const float ck = (1.0f - K / 255.0f);
                const int ir = clamp255((1.0f - C  / 255.0f) * ck * 255.0f);
                const int ig = clamp255((1.0f - M  / 255.0f) * ck * 255.0f);
                const int ib = clamp255((1.0f - Yk / 255.0f) * ck * 255.0f);
                dst[col*4 + 0] = std::byte(ir);
                dst[col*4 + 1] = std::byte(ig);
                dst[col*4 + 2] = std::byte(ib);
                dst[col*4 + 3] = std::byte(0xFF);
            }
        }
        return rgba;
    }

    const ComponentInfo& yC  = f.components[0];
    const ComponentInfo& cbC = f.components[1];
    const ComponentInfo& crC = f.components[2];
    for (std::uint32_t row = 0; row < f.height; ++row) {
        std::byte* dst = rgba.data() + std::size_t(row) * f.width * 4u;
        for (std::uint32_t col = 0; col < f.width; ++col) {
            // Luma at (col, row) — Y is at full resolution; sampling factors
            // are (yC.h, yC.v) = (max_h, max_v) so bilinear collapses to
            // exact-pixel readout. Use direct readout when h=max_h, v=max_v.
            float Y;
            if (yC.h == f.max_h && yC.v == f.max_v) {
                Y = float(yC.samples[std::size_t(row) * yC.width + col]);
            } else {
                Y = SampleBilinear(yC, float(col), float(row), f.max_h, f.max_v);
            }
            float Cb = SampleBilinear(cbC, float(col), float(row), f.max_h, f.max_v) - 128.0f;
            float Cr = SampleBilinear(crC, float(col), float(row), f.max_h, f.max_v) - 128.0f;
            float R = Y + 1.402f * Cr;
            float G = Y - 0.34414f * Cb - 0.71414f * Cr;
            float B = Y + 1.772f * Cb;
            dst[col*4 + 0] = std::byte(clamp255(R));
            dst[col*4 + 1] = std::byte(clamp255(G));
            dst[col*4 + 2] = std::byte(clamp255(B));
            dst[col*4 + 3] = std::byte(0xFF);
        }
    }
    return rgba;
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────
// Public Decode
// ──────────────────────────────────────────────────────────────────────────

DecodedImage Decode(std::span<const std::byte> input) {
    if (input.empty()) Fail("input is empty");

    MarkerStream ms(input.data(), input.size());

    // The first marker must be SOI.
    std::uint8_t m = ms.NextMarker();
    if (m != kSOI) Fail("first marker is not SOI");

    FrameInfo frame;
    std::array<QuantTable, 4> quant{};
    std::array<HuffTable, 4> dc_tables{};
    std::array<HuffTable, 4> ac_tables{};
    std::uint32_t restart_interval = 0;
    bool sof_seen = false;
    bool eoi_seen = false;

    while (!eoi_seen) {
        m = ms.NextMarker();
        switch (m) {
            case kSOI:
                Fail("duplicate SOI");
            case kEOI:
                eoi_seen = true;
                break;
            case kSOF0:
            case kSOF2: {
                if (sof_seen) Fail("duplicate SOF");
                auto seg = ms.ReadSegment();
                ReadSOF(seg, frame, /*baseline=*/m == kSOF0);
                sof_seen = true;
                break;
            }
            case kSOF1:
                Fail("SOF1 (extended sequential) unsupported");
            case kSOF3:
                Fail("SOF3 (lossless) unsupported");
            case kDQT: {
                auto seg = ms.ReadSegment();
                ReadDQT(seg, quant);
                break;
            }
            case kDHT: {
                auto seg = ms.ReadSegment();
                ReadDHT(seg, dc_tables, ac_tables);
                break;
            }
            case kDRI: {
                auto seg = ms.ReadSegment();
                if (seg.size() != 2) Fail("DRI: payload must be 2 bytes");
                restart_interval = (std::uint32_t(seg[0]) << 8) | seg[1];
                break;
            }
            case kSOS: {
                if (!sof_seen) Fail("SOS before SOF");
                auto seg = ms.ReadSegment();
                ScanInfo scan;
                ReadSOS(seg, frame, scan);
                BitReader br(ms);
                if (frame.baseline) {
                    DecodeBaselineScan(ms, br, frame, scan, dc_tables, ac_tables,
                                       restart_interval);
                } else {
                    DecodeProgressiveScan(ms, br, frame, scan, dc_tables, ac_tables,
                                          restart_interval);
                }
                break;
            }
            case kCOM:
                ms.SkipSegment();
                break;
            case kAPP14: {
                // Parse the "Adobe" APP14 segment for its colour
                // transform; any other APP14 payload is ignored.
                const auto seg = ms.ReadSegment();
                if (seg.size() >= 12
                        && std::memcmp(seg.data(), "Adobe", 5) == 0) {
                    frame.adobe = true;
                    frame.adobe_transform = seg[11];
                }
                break;
            }
            default:
                if (m >= kAPP0 && m <= kAPP15) {
                    ms.SkipSegment();
                } else if (m >= kRST0 && m <= kRST7) {
                    // Stray restart outside an entropy stream — ignore per
                    // libjpeg convention; some encoders emit redundant ones.
                } else if (m >= 0xF0) {
                    // T.81 reserves these; skip over a length-prefixed segment
                    // if one is present, else fail. Be conservative and fail.
                    Fail("unsupported marker 0x" + std::to_string(int(m)));
                } else {
                    Fail("unexpected marker 0x" + std::to_string(int(m)));
                }
                break;
        }
    }

    if (!sof_seen) Fail("no SOF marker");

    Finalize(frame, quant);
    auto rgba = AssembleRGBA(frame);

    DecodedImage out;
    out.width = frame.width;
    out.height = frame.height;
    out.components = 4;
    out.pixels = std::move(rgba);
    return out;
}

}  // namespace foundation::dctdecode
