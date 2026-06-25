#include "jpx.hpp"
#include "jpx_detail.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace foundation::jpx {

namespace detail {

// Probability-estimation state table — ISO/IEC 15444-1 Table C.2 (identical
// to ITU-T T.88 Table E.1). Each row: Qe value, NMPS (next index after a
// renormalisation on the more-probable symbol), NLPS (next index after a
// renormalisation on the less-probable symbol), SWITCH (toggle the
// more-probable-symbol sense when set).
namespace {
struct QeEntry { std::uint16_t qe; std::uint8_t nmps; std::uint8_t nlps; std::uint8_t sw; };
const QeEntry kQe[47] = {
    {0x5601, 1, 1, 1}, {0x3401, 2, 6, 0}, {0x1801, 3, 9, 0}, {0x0AC1, 4,12, 0},
    {0x0521, 5,29, 0}, {0x0221,38,33, 0}, {0x5601, 7, 6, 1}, {0x5401, 8,14, 0},
    {0x4801, 9,14, 0}, {0x3801,10,14, 0}, {0x3001,11,17, 0}, {0x2401,12,18, 0},
    {0x1C01,13,20, 0}, {0x1601,29,21, 0}, {0x5601,15,14, 1}, {0x5401,16,14, 0},
    {0x5101,17,15, 0}, {0x4801,18,16, 0}, {0x3801,19,17, 0}, {0x3401,20,18, 0},
    {0x3001,21,19, 0}, {0x2801,22,19, 0}, {0x2401,23,20, 0}, {0x2201,24,21, 0},
    {0x1C01,25,22, 0}, {0x1801,26,23, 0}, {0x1601,27,24, 0}, {0x1401,28,25, 0},
    {0x1201,29,26, 0}, {0x1101,30,27, 0}, {0x0AC1,31,28, 0}, {0x09C1,32,29, 0},
    {0x08A1,33,30, 0}, {0x0521,34,31, 0}, {0x0441,35,32, 0}, {0x02A1,36,33, 0},
    {0x0221,37,34, 0}, {0x0141,38,35, 0}, {0x0111,39,36, 0}, {0x0085,40,37, 0},
    {0x0049,41,38, 0}, {0x0025,42,39, 0}, {0x0015,43,40, 0}, {0x0009,44,41, 0},
    {0x0005,45,42, 0}, {0x0001,45,43, 0}, {0x5601,46,46, 0},
};
}  // namespace

// INITDEC — ISO/IEC 15444-1 Figure C.17.
MqDecoder::MqDecoder(const std::uint8_t* data, std::size_t len)
    : data_(data), len_(len), bp_(0) {
    c_ = static_cast<std::uint32_t>(ByteAt(bp_)) << 16;
    ByteIn();
    c_ <<= 7;
    ct_ -= 7;
    a_ = 0x8000;
}

// BYTEIN — ISO/IEC 15444-1 Figure C.18.
void MqDecoder::ByteIn() {
    if (ByteAt(bp_) == 0xFF) {
        if (ByteAt(bp_ + 1) > 0x8F) {
            c_ += 0xFF00;
            ct_ = 8;
        } else {
            ++bp_;
            c_ += static_cast<std::uint32_t>(ByteAt(bp_)) << 9;
            ct_ = 7;
        }
    } else {
        ++bp_;
        c_ += static_cast<std::uint32_t>(ByteAt(bp_)) << 8;
        ct_ = 8;
    }
}

// RENORMD — ISO/IEC 15444-1 Figure C.16.
void MqDecoder::Renorm() {
    do {
        if (ct_ == 0) ByteIn();
        a_ <<= 1;
        c_ <<= 1;
        --ct_;
    } while ((a_ & 0x8000) == 0);
}

// DECODE — ISO/IEC 15444-1 Figure C.15, with the MPS_EXCHANGE and
// LPS_EXCHANGE procedures (Figures C.34, C.35).
int MqDecoder::Decode(MqContext& cx) {
    const QeEntry& q = kQe[cx.index];
    const std::uint32_t qe = q.qe;
    a_ -= qe;

    int d;
    if ((c_ >> 16) < qe) {
        // LPS exchange.
        if (a_ < qe) {
            d = cx.mps;
            cx.index = q.nmps;
        } else {
            d = 1 - cx.mps;
            if (q.sw) cx.mps = static_cast<std::uint8_t>(1 - cx.mps);
            cx.index = q.nlps;
        }
        a_ = qe;
        Renorm();
    } else {
        c_ -= qe << 16;
        if ((a_ & 0x8000) == 0) {
            // MPS exchange.
            if (a_ < qe) {
                d = 1 - cx.mps;
                if (q.sw) cx.mps = static_cast<std::uint8_t>(1 - cx.mps);
                cx.index = q.nlps;
            } else {
                d = cx.mps;
                cx.index = q.nmps;
            }
            Renorm();
        } else {
            d = cx.mps;
        }
    }
    return d;
}

// Floor division for the lifting steps' (a + b [+ r]) / 2^k terms — ISO/IEC
// 15444-1 Annex F requires true mathematical floor, not C++ truncation
// toward zero, for the negative operands that arise at signal boundaries.
// Also used by the inverse RCT (Annex G.2, Eq G-7) and the subband
// dimension formula (Eq B-15) below.
std::int32_t FloorDiv(std::int32_t num, std::int32_t den) {
    std::int32_t q = num / den;
    std::int32_t r = num % den;
    if (r != 0 && ((r < 0) != (den < 0))) --q;
    return q;
}

namespace {

// Symmetric boundary extension index map — ISO/IEC 15444-1 Annex F.3.3:
// s[-1] = s[1], s[N] = s[N-2], reflecting repeatedly for indices further
// out of range (sufficient for the single-tap lookups the lifting steps
// need, even on length-2 signals).
std::size_t Reflect(std::int64_t i, std::size_t n) {
    if (n == 1) return 0;
    const std::int64_t period = 2 * static_cast<std::int64_t>(n) - 2;
    std::int64_t m = i % period;
    if (m < 0) m += period;
    if (m >= static_cast<std::int64_t>(n)) m = period - m;
    return static_cast<std::size_t>(m);
}

}  // namespace

// Forward 5/3 DWT, one level, in place (interleaved even/odd) — ISO/IEC
// 15444-1 Annex F.3.8, step 1 (predict, odd samples) then step 2 (update,
// even samples), Table F.4.
void Fdwt53_1d(std::vector<std::int32_t>& s) {
    const std::size_t n = s.size();
    if (n < 2) return;

    // Step 1 (predict): odd samples become high-pass coefficients.
    std::vector<std::int32_t> d(n);
    for (std::size_t i = 1; i < n; i += 2) {
        const std::int32_t a = s[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const std::int32_t b = s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        d[i] = s[i] - FloorDiv(a + b, 2);
    }
    // Step 2 (update): even samples become low-pass coefficients, using
    // the already-updated (high-pass) odd neighbours.
    for (std::size_t i = 0; i < n; i += 2) {
        const std::int32_t a = (i == 0) ? d[Reflect(-1, n)] : d[i - 1];
        const std::int32_t b = (i + 1 < n) ? d[i + 1] : d[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        d[i] = s[i] + FloorDiv(a + b + 2, 4);
    }
    s = d;
}

// Inverse 5/3 DWT, one level, in place (interleaved even/odd) — ISO/IEC
// 15444-1 Annex F.3.7, step 1 (undo update, even samples) then step 2 (undo
// predict, odd samples), Table F.4.
void Idwt53_1d(std::vector<std::int32_t>& s) {
    const std::size_t n = s.size();
    if (n < 2) return;

    std::vector<std::int32_t> d = s;
    // Step 1 (undo update): restore even samples using the (still
    // high-pass) odd neighbours.
    for (std::size_t i = 0; i < n; i += 2) {
        const std::int32_t a = (i == 0) ? s[Reflect(-1, n)] : s[i - 1];
        const std::int32_t b = (i + 1 < n) ? s[i + 1] : s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        d[i] = s[i] - FloorDiv(a + b + 2, 4);
    }
    // Step 2 (undo predict): restore odd samples using the now-restored
    // even neighbours.
    for (std::size_t i = 1; i < n; i += 2) {
        const std::int32_t a = d[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const std::int32_t b = d[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        d[i] = s[i] + FloorDiv(a + b, 2);
    }
    s = d;
}

namespace {

// 9/7 lifting coefficients and low/high-pass scaling factor — ISO/IEC
// 15444-1 Table F.4.
constexpr double kAlpha = -1.586134342059924;
constexpr double kBeta = -0.052980118572961;
constexpr double kGamma = 0.882911075530934;
constexpr double kDelta = 0.443506852043971;
constexpr double kKappa = 1.230174104914001;

}  // namespace

// Forward 9/7 DWT, one level, in place (interleaved even/odd) — ISO/IEC
// 15444-1 Annex F.3.8 lifting steps (alpha, beta, gamma, delta) followed by
// the Table F.4 scaling, the inverse of the steps in Idwt97_1d.
void Fdwt97_1d(std::vector<double>& s) {
    const std::size_t n = s.size();
    if (n < 2) return;

    // Step alpha: odd[n] += alpha * (even[n] + even[n+1]).
    for (std::size_t i = 1; i < n; i += 2) {
        const double a = s[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const double b = s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] += kAlpha * (a + b);
    }
    // Step beta: even[n] += beta * (odd[n-1] + odd[n]).
    for (std::size_t i = 0; i < n; i += 2) {
        const double a = (i == 0) ? s[Reflect(-1, n)] : s[i - 1];
        const double b = (i + 1 < n) ? s[i + 1] : s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] += kBeta * (a + b);
    }
    // Step gamma: odd[n] += gamma * (even[n] + even[n+1]).
    for (std::size_t i = 1; i < n; i += 2) {
        const double a = s[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const double b = s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] += kGamma * (a + b);
    }
    // Step delta: even[n] += delta * (odd[n-1] + odd[n]).
    for (std::size_t i = 0; i < n; i += 2) {
        const double a = (i == 0) ? s[Reflect(-1, n)] : s[i - 1];
        const double b = (i + 1 < n) ? s[i + 1] : s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] += kDelta * (a + b);
    }
    // Scale: even *= 1/K, odd *= K.
    for (std::size_t i = 0; i < n; ++i) {
        s[i] *= (i % 2 == 0) ? (1.0 / kKappa) : kKappa;
    }
}

// Inverse 9/7 DWT, one level, in place (interleaved even/odd) — ISO/IEC
// 15444-1 Annex F.3.7: undo the Table F.4 scaling, then undo the lifting
// steps delta, gamma, beta, alpha in reverse.
void Idwt97_1d(std::vector<double>& s) {
    const std::size_t n = s.size();
    if (n < 2) return;

    // Undo scale: even *= K, odd *= 1/K.
    for (std::size_t i = 0; i < n; ++i) {
        s[i] *= (i % 2 == 0) ? kKappa : (1.0 / kKappa);
    }
    // Undo step delta: even[n] -= delta * (odd[n-1] + odd[n]).
    for (std::size_t i = 0; i < n; i += 2) {
        const double a = (i == 0) ? s[Reflect(-1, n)] : s[i - 1];
        const double b = (i + 1 < n) ? s[i + 1] : s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] -= kDelta * (a + b);
    }
    // Undo step gamma: odd[n] -= gamma * (even[n] + even[n+1]).
    for (std::size_t i = 1; i < n; i += 2) {
        const double a = s[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const double b = s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] -= kGamma * (a + b);
    }
    // Undo step beta: even[n] -= beta * (odd[n-1] + odd[n]).
    for (std::size_t i = 0; i < n; i += 2) {
        const double a = (i == 0) ? s[Reflect(-1, n)] : s[i - 1];
        const double b = (i + 1 < n) ? s[i + 1] : s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] -= kBeta * (a + b);
    }
    // Undo step alpha: odd[n] -= alpha * (even[n] + even[n+1]).
    for (std::size_t i = 1; i < n; i += 2) {
        const double a = s[Reflect(static_cast<std::int64_t>(i) - 1, n)];
        const double b = s[Reflect(static_cast<std::int64_t>(i) + 1, n)];
        s[i] -= kAlpha * (a + b);
    }
}

namespace {

// Extract a strided sub-sequence (a row, or a column of a row-major plane),
// run a 1-D in-place transform on it, and write the result back to the same
// strided positions — ISO/IEC 15444-1 Annex F.3.1 (the 2-D transform is
// separable: 1-D transforms applied independently along each dimension).
template <typename T, typename Fn>
void TransformStrided(std::vector<T>& plane, std::size_t start, std::size_t count, std::size_t stride, Fn fn) {
    std::vector<T> line(count);
    for (std::size_t i = 0; i < count; ++i) line[i] = plane[start + i * stride];
    fn(line);
    for (std::size_t i = 0; i < count; ++i) plane[start + i * stride] = line[i];
}

// Two-dimensional inverse (synthesis) transform — ISO/IEC 15444-1 Annex F.3.1
// (separable rows/columns) combined with the Mallat multiresolution order of
// Annex F.4: the synthesis is applied from the coarsest level (smallest LL
// region) outward, doubling the active w*h region each level, and at each
// level columns are synthesised before rows (the reverse of the forward
// transform's row-then-column order).
template <typename T, typename Fn>
void Idwt2dImpl(std::vector<T>& plane, std::uint32_t w, std::uint32_t h, int levels, Fn idwt1d) {
    if (levels < 1) return;

    // Active region sizes at each level, coarsest (level `levels`) down to
    // the full image (level 0), per the standard dyadic subsampling.
    std::vector<std::uint32_t> widths(levels + 1), heights(levels + 1);
    widths[0] = w;
    heights[0] = h;
    for (int l = 1; l <= levels; ++l) {
        widths[l] = (widths[l - 1] + 1) / 2;
        heights[l] = (heights[l - 1] + 1) / 2;
    }

    for (int l = levels; l >= 1; --l) {
        const std::uint32_t cw = widths[l - 1];
        const std::uint32_t ch = heights[l - 1];
        // Columns first, then rows — the reverse of the forward
        // transform's row-then-column order (Annex F.3.1).
        if (ch >= 2) {
            for (std::uint32_t x = 0; x < cw; ++x) {
                TransformStrided(plane, x, ch, w, idwt1d);
            }
        }
        if (cw >= 2) {
            for (std::uint32_t y = 0; y < ch; ++y) {
                TransformStrided(plane, static_cast<std::size_t>(y) * w, cw, 1, idwt1d);
            }
        }
    }
}

// Re-arrange the top-left cw x ch region of a row-major w-stride plane from
// the Mallat quadrant arrangement (LL block of size llw x llh at (0,0); HL at
// (llw,0); LH at (0,llh); HH at (llw,llh) — Annex F.4) into the checkerboard
// (even/odd index) interleaving that Idwt53_1d/Idwt97_1d expect (Annex F.3.7:
// even positions hold the low-pass/LL-derived samples, odd positions the
// high-pass/detail samples).
template <typename T>
void InterleaveMallatRegion(std::vector<T>& plane, std::uint32_t w, std::uint32_t cw, std::uint32_t ch,
                             std::uint32_t llw, std::uint32_t llh) {
    std::vector<T> tmp(static_cast<std::size_t>(cw) * ch);
    for (std::uint32_t y = 0; y < ch; ++y) {
        for (std::uint32_t x = 0; x < cw; ++x) {
            const bool left = x < llw;
            const bool top = y < llh;
            T v;
            if (left && top) {
                v = plane[static_cast<std::size_t>(y) * w + x];                                  // LL
            } else if (!left && top) {
                v = plane[static_cast<std::size_t>(y) * w + (llw + (x - llw))];                  // HL
            } else if (left && !top) {
                v = plane[static_cast<std::size_t>(llh + (y - llh)) * w + x];                    // LH
            } else {
                v = plane[static_cast<std::size_t>(llh + (y - llh)) * w + (llw + (x - llw))];    // HH
            }
            const std::uint32_t ox = left ? 2 * x : 2 * (x - llw) + 1;
            const std::uint32_t oy = top ? 2 * y : 2 * (y - llh) + 1;
            tmp[static_cast<std::size_t>(oy) * cw + ox] = v;
        }
    }
    for (std::uint32_t y = 0; y < ch; ++y) {
        for (std::uint32_t x = 0; x < cw; ++x) {
            plane[static_cast<std::size_t>(y) * w + x] = tmp[static_cast<std::size_t>(y) * cw + x];
        }
    }
}

// Multi-level inverse (synthesis) transform over a plane laid out in the
// standard Mallat quadrant arrangement (Annex F.4): at each level, from
// coarsest to finest, the active cw x ch region (whose top-left llw x llh
// sub-region is the LL band — either the coarsest LL or the previous level's
// synthesis result — and whose remaining quadrants are that level's HL/LH/HH)
// is converted to the checkerboard interleaving (Annex F.3.7) and then
// synthesised in place by the horizontal (row) 1-D pass followed by the
// vertical (column) 1-D pass. Because the 5/3 lifting is non-linear (floored
// integer rounding), the two passes do not commute, so the synthesis order
// must match the analysis convention exactly to round-trip byte-for-byte.
template <typename T, typename Fn>
void Idwt2dMallatImpl(std::vector<T>& plane, std::uint32_t w, std::uint32_t h, int levels, Fn idwt1d) {
    if (levels < 1) return;

    std::vector<std::uint32_t> widths(levels + 1), heights(levels + 1);
    widths[0] = w;
    heights[0] = h;
    for (int l = 1; l <= levels; ++l) {
        widths[l] = (widths[l - 1] + 1) / 2;
        heights[l] = (heights[l - 1] + 1) / 2;
    }

    for (int l = levels; l >= 1; --l) {
        const std::uint32_t cw = widths[l - 1];
        const std::uint32_t ch = heights[l - 1];
        const std::uint32_t llw = widths[l];
        const std::uint32_t llh = heights[l];

        InterleaveMallatRegion(plane, w, cw, ch, llw, llh);

        if (cw >= 2) {
            for (std::uint32_t y = 0; y < ch; ++y) {
                TransformStrided(plane, static_cast<std::size_t>(y) * w, cw, 1, idwt1d);
            }
        }
        if (ch >= 2) {
            for (std::uint32_t x = 0; x < cw; ++x) {
                TransformStrided(plane, x, ch, w, idwt1d);
            }
        }
    }
}

}  // namespace

void Idwt2d_53(std::vector<std::int32_t>& plane, std::uint32_t w, std::uint32_t h, int levels) {
    Idwt2dImpl(plane, w, h, levels, [](std::vector<std::int32_t>& line) { Idwt53_1d(line); });
}

void Idwt2d_97(std::vector<double>& plane, std::uint32_t w, std::uint32_t h, int levels) {
    Idwt2dImpl(plane, w, h, levels, [](std::vector<double>& line) { Idwt97_1d(line); });
}

void Idwt2dMallat53(std::vector<std::int32_t>& plane, std::uint32_t w, std::uint32_t h, int levels) {
    Idwt2dMallatImpl(plane, w, h, levels, [](std::vector<std::int32_t>& line) { Idwt53_1d(line); });
}

void Idwt2dMallat97(std::vector<double>& plane, std::uint32_t w, std::uint32_t h, int levels) {
    Idwt2dMallatImpl(plane, w, h, levels, [](std::vector<double>& line) { Idwt97_1d(line); });
}

namespace {

// Marker codes — ISO/IEC 15444-1 Table A.2 (delimiting and fixed-information
// markers) and Table A.3 (functional / pointer markers).
constexpr std::uint16_t kSOC = 0xFF4F;  // Start of codestream (Annex A.4.1)
constexpr std::uint16_t kSIZ = 0xFF51;  // Image and tile size (Annex A.5.1)
constexpr std::uint16_t kCOD = 0xFF52;  // Coding style default (Annex A.6.1)
constexpr std::uint16_t kCOC = 0xFF53;  // Coding style component (Annex A.6.2)
constexpr std::uint16_t kTLM = 0xFF55;  // Tile-part lengths (Annex A.7.1)
constexpr std::uint16_t kPLM = 0xFF57;  // Packet length, main header (Annex A.7.2)
constexpr std::uint16_t kPLT = 0xFF58;  // Packet length, tile-part header (Annex A.7.3)
constexpr std::uint16_t kQCD = 0xFF5C;  // Quantization default (Annex A.6.4)
constexpr std::uint16_t kQCC = 0xFF5D;  // Quantization component (Annex A.6.5)
constexpr std::uint16_t kRGN = 0xFF5E;  // Region of interest (Annex A.6.6)
constexpr std::uint16_t kPOC = 0xFF5F;  // Progression order change (Annex A.6.3)
constexpr std::uint16_t kPPM = 0xFF60;  // Packed packet headers, main header (Annex A.7.4)
constexpr std::uint16_t kPPT = 0xFF61;  // Packed packet headers, tile-part header (Annex A.7.5)
constexpr std::uint16_t kSOT = 0xFF90;  // Start of tile-part (Annex A.4.2)
constexpr std::uint16_t kSOD = 0xFF93;  // Start of data (Annex A.4.3)
constexpr std::uint16_t kEOC = 0xFFD9;  // End of codestream (Annex A.4.4)
constexpr std::uint16_t kCOM = 0xFF64;  // Comment (Annex A.6.7)

// Big-endian fixed-width reads with bounds checking — every multi-byte field
// in Annex A is big-endian (Annex A.2).
class Reader {
public:
    Reader(const std::byte* data, std::size_t size) : data_(data), size_(size) {}

    std::size_t pos() const { return pos_; }
    std::size_t size() const { return size_; }
    bool AtEnd() const { return pos_ >= size_; }
    std::size_t Remaining() const { return size_ - pos_; }

    std::uint8_t U8() {
        Require(1);
        return static_cast<std::uint8_t>(data_[pos_++]);
    }
    std::uint16_t U16() {
        Require(2);
        const auto hi = static_cast<std::uint16_t>(static_cast<std::uint8_t>(data_[pos_]));
        const auto lo = static_cast<std::uint16_t>(static_cast<std::uint8_t>(data_[pos_ + 1]));
        pos_ += 2;
        return static_cast<std::uint16_t>((hi << 8) | lo);
    }
    std::uint32_t U32() {
        Require(4);
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8) | static_cast<std::uint8_t>(data_[pos_ + static_cast<std::size_t>(i)]);
        }
        pos_ += 4;
        return v;
    }
    void Skip(std::size_t n) {
        Require(n);
        pos_ += n;
    }
    void SeekTo(std::size_t p) {
        if (p > size_) throw std::runtime_error("jpx: seek past end of codestream");
        pos_ = p;
    }
    std::span<const std::byte> SpanFrom(std::size_t start, std::size_t end) const {
        if (end > size_ || start > end) throw std::runtime_error("jpx: invalid span");
        return std::span<const std::byte>(data_ + start, end - start);
    }

private:
    void Require(std::size_t n) const {
        if (pos_ + n > size_) throw std::runtime_error("jpx: truncated codestream");
    }
    const std::byte* data_;
    std::size_t size_;
    std::size_t pos_ = 0;
};

// Read a marker code (2 bytes, must be 0xFFxx — Annex A.2).
std::uint16_t ReadMarker(Reader& r) {
    const std::uint16_t m = r.U16();
    if ((m & 0xFF00) != 0xFF00) {
        throw std::runtime_error("jpx: expected marker, found non-marker bytes");
    }
    return m;
}

// SIZ marker — ISO/IEC 15444-1 Annex A.5.1. Populates image geometry,
// per-component bit depth/signedness, and the tile grid (Xsiz/Ysiz vs.
// XTsiz/YTsiz) used for the single-tile check.
struct SizInfo {
    std::uint32_t xsiz, ysiz, xosiz, yosiz;
    std::uint32_t xtsiz, ytsiz, xtosiz, ytosiz;
    std::vector<detail::Component> components;
};

SizInfo ParseSiz(Reader seg) {
    SizInfo siz{};
    seg.U16();  // Rsiz — capabilities, not needed for v1 scope checks.
    siz.xsiz = seg.U32();
    siz.ysiz = seg.U32();
    siz.xosiz = seg.U32();
    siz.yosiz = seg.U32();
    siz.xtsiz = seg.U32();
    siz.ytsiz = seg.U32();
    siz.xtosiz = seg.U32();
    siz.ytosiz = seg.U32();
    const std::uint16_t csiz = seg.U16();
    if (csiz != 1 && csiz != 3 && csiz != 4) {
        throw std::runtime_error("jpx: unsupported component count (expected 1, 3, or 4)");
    }
    siz.components.reserve(csiz);
    for (std::uint16_t c = 0; c < csiz; ++c) {
        const std::uint8_t ssiz = seg.U8();
        const std::uint8_t xrsiz = seg.U8();
        const std::uint8_t yrsiz = seg.U8();
        if (xrsiz != 1 || yrsiz != 1) {
            throw std::runtime_error("jpx: unsupported component sub-sampling (SIZ XRsiz/YRsiz != 1)");
        }
        const int bit_depth = (ssiz & 0x7F) + 1;
        const bool is_signed = (ssiz & 0x80) != 0;
        // 1..16 bpc supported: the decode pipeline carries bit_depth through
        // the DC level-shift and dequant, and the final pack scales the
        // [0, 2^depth-1] sample down to 8-bit RGBA (field 33342_*_16bit_*).
        // >16 risks int32 overflow in the level-shift / step-size math.
        if (bit_depth < 1 || bit_depth > 16) {
            throw std::runtime_error("jpx: unsupported bit depth (SIZ Ssiz must be 1..16)");
        }
        siz.components.push_back(detail::Component{bit_depth, is_signed});
    }
    return siz;
}

// SPcod fixed portion (excluding optional precinct sizes) — ISO/IEC
// 15444-1 Table A.19/A.20, packaged with the SGcod fields that precede it.
struct CodInfo {
    std::uint8_t progression;
    std::uint16_t layers;
    bool mct;
    int levels;
    int cb_w_exp, cb_h_exp;
    std::uint8_t cbstyle;
    detail::Wavelet wavelet;
};

// COD marker — ISO/IEC 15444-1 Annex A.6.1. `seg` covers the marker body
// (after Lcod). Scod bit 0 (precinct sizes present) is checked but the
// optional per-level precinct-size bytes are simply not read further (no
// fields after SPcod are needed for v1; the segment length already bounds
// the marker so trailing bytes are harmless).
CodInfo ParseCod(Reader seg) {
    CodInfo cod{};
    const std::uint8_t scod = seg.U8();
    cod.progression = seg.U8();
    cod.layers = seg.U16();
    const std::uint8_t mct_flag = seg.U8();
    cod.mct = (mct_flag & 0x01) != 0;

    cod.levels = seg.U8();
    cod.cb_w_exp = seg.U8();
    cod.cb_h_exp = seg.U8();
    cod.cbstyle = seg.U8();
    const std::uint8_t transform = seg.U8();
    cod.wavelet = (transform == 1) ? detail::Wavelet::Reversible53 : detail::Wavelet::Irreversible97;

    (void)scod;  // Precinct-size override (Scod bit 0) is not v1-rejected;
                 // trailing SPcod bytes (if any) are inside the bounded
                 // segment and are simply unused.
    return cod;
}

// QCD marker — ISO/IEC 15444-1 Annex A.6.4. Returns (style, guard bits,
// per-subband quant entries). Style 0 (no quantization): one byte per
// subband, exponent = byte >> 3. Styles 1/2 (derived/expounded): 2 bytes
// per entry, exponent = value >> 11, mantissa = value & 0x7FF.
void ParseQcd(Reader seg, int& quant_style, int& guard_bits, std::vector<detail::SubbandQuant>& quant) {
    const std::uint8_t sqcd = seg.U8();
    quant_style = sqcd & 0x1F;
    guard_bits = sqcd >> 5;
    if (quant_style == 0) {
        while (!seg.AtEnd()) {
            const std::uint8_t v = seg.U8();
            quant.push_back(detail::SubbandQuant{v >> 3, 0});
        }
    } else {
        while (seg.Remaining() >= 2) {
            const std::uint16_t v = seg.U16();
            quant.push_back(detail::SubbandQuant{v >> 11, v & 0x7FF});
        }
    }
}

}  // namespace

// Parse a JPEG 2000 codestream main header plus its single tile-part —
// ISO/IEC 15444-1 Annex A.
Image ParseCodestream(std::span<const std::byte> cs) {
    Reader r(cs.data(), cs.size());

    // SOC (Annex A.4.1) — first marker, no segment body.
    if (ReadMarker(r) != kSOC) {
        throw std::runtime_error("jpx: codestream does not start with SOC");
    }

    std::optional<SizInfo> siz;
    std::optional<CodInfo> cod;
    int quant_style = -1, guard_bits = 0;
    std::vector<SubbandQuant> quant;
    bool saw_sot = false;

    while (!saw_sot) {
        if (r.Remaining() < 2) throw std::runtime_error("jpx: truncated main header (missing SOT)");
        const std::uint16_t marker = ReadMarker(r);

        if (marker == kSOT) {
            saw_sot = true;
            break;
        }

        // Every other main-header marker carries a 2-byte big-endian
        // length (Lmarker) that includes the length field itself but not
        // the marker code (Annex A.2).
        const std::uint16_t lmarker = r.U16();
        if (lmarker < 2) throw std::runtime_error("jpx: invalid marker segment length");
        const std::size_t body_len = static_cast<std::size_t>(lmarker) - 2;
        const std::size_t body_start = r.pos();
        const std::size_t body_end = body_start + body_len;
        if (body_end > r.size()) throw std::runtime_error("jpx: marker segment runs past end of codestream");
        Reader seg(cs.data() + body_start, body_len);

        switch (marker) {
            case kSIZ:
                if (siz.has_value()) throw std::runtime_error("jpx: duplicate SIZ marker");
                siz = ParseSiz(seg);
                break;
            case kCOD:
                if (!siz.has_value()) throw std::runtime_error("jpx: COD before SIZ");
                cod = ParseCod(seg);
                break;
            case kQCD:
                ParseQcd(seg, quant_style, guard_bits, quant);
                break;
            case kRGN:
                throw std::runtime_error("jpx: region-of-interest (RGN) is unsupported");
            case kCOC:
            case kQCC:
            case kPOC:
                // Per-component overrides — Annex A.6.2/A.6.5/A.6.3. v1
                // decodes using the COD/QCD defaults only; step over.
                break;
            case kCOM:
            case kTLM:
            case kPLM:
            case kPLT:
            case kPPM:
            case kPPT:
                // Informational / pointer markers — Annex A.6.7, A.7.1-5.
                // Not needed for v1 decode; length-step over.
                break;
            default:
                throw std::runtime_error("jpx: unknown or unsupported marker in main header");
        }

        r.SeekTo(body_end);
    }

    if (!siz.has_value()) throw std::runtime_error("jpx: missing SIZ marker");
    if (!cod.has_value()) throw std::runtime_error("jpx: missing COD marker");
    if (quant_style < 0) throw std::runtime_error("jpx: missing QCD marker");

    // LRCP (0) and RLCP (1) progressions are supported, with any number of
    // quality layers — the packet walk accumulates each code-block's coding
    // passes across layers (Annex B.10/B.12). Other progression orders are
    // out of scope.
    if (cod->progression != 0 && cod->progression != 1) {
        throw std::runtime_error("jpx: unsupported progression order (LRCP/RLCP only)");
    }
    // v1 scope: code-block style must be the default (no bypass, reset,
    // termination, vertically-causal, predictable-termination, or
    // segmentation-symbol bits set — Annex A.6.1, Table A.19).
    if (cod->cbstyle != 0) {
        throw std::runtime_error("jpx: unsupported non-default code-block style");
    }

    // Tile grid — ISO/IEC 15444-1 Annex B.3.
    const SizInfo& sz = *siz;
    const std::uint32_t image_w = sz.xsiz - sz.xosiz;
    const std::uint32_t image_h = sz.ysiz - sz.yosiz;
    const std::uint32_t tiles_x = (sz.xsiz - sz.xtosiz + sz.xtsiz - 1) / sz.xtsiz;
    const std::uint32_t tiles_y = (sz.ysiz - sz.ytosiz + sz.ytsiz - 1) / sz.ytsiz;
    const std::uint32_t num_tiles = tiles_x * tiles_y;

    Image image{};
    image.width = image_w;
    image.height = image_h;
    image.x0 = sz.xosiz;
    image.y0 = sz.yosiz;
    image.components = sz.components;
    image.wavelet = cod->wavelet;
    image.levels = cod->levels;
    image.cb_w_exp = cod->cb_w_exp;
    image.cb_h_exp = cod->cb_h_exp;
    image.mct = cod->mct;
    image.layers = cod->layers;
    image.progression = cod->progression;
    image.quant_style = quant_style;
    image.guard_bits = guard_bits;
    image.quant = std::move(quant);
    image.tiles_x = tiles_x;
    image.tiles_y = tiles_y;
    image.tiles.resize(num_tiles);

    // Per-tile canvas bounds (Annex B.3): tile (col,row) spans
    // [XTOsiz+col*XTsiz, Xsiz) clipped, mapped to image-origin coordinates.
    for (std::uint32_t t = 0; t < num_tiles; ++t) {
        const std::uint32_t col = t % tiles_x;
        const std::uint32_t row = t / tiles_x;
        const std::uint32_t tx0 = std::max(sz.xtosiz + col * sz.xtsiz, sz.xosiz);
        const std::uint32_t ty0 = std::max(sz.ytosiz + row * sz.ytsiz, sz.yosiz);
        const std::uint32_t tx1 = std::min(sz.xtosiz + (col + 1) * sz.xtsiz, sz.xsiz);
        const std::uint32_t ty1 = std::min(sz.ytosiz + (row + 1) * sz.ytsiz, sz.ysiz);
        image.tiles[t].x0 = tx0 - sz.xosiz;
        image.tiles[t].y0 = ty0 - sz.yosiz;
        image.tiles[t].x1 = tx1 - sz.xosiz;
        image.tiles[t].y1 = ty1 - sz.yosiz;
    }

    // Tile-part loop — ISO/IEC 15444-1 Annex A.4.2/A.4.3. The main-header loop
    // above stopped having just consumed the first SOT marker code. Each
    // iteration parses one tile-part (SOT segment, optional tile-part-header
    // markers, SOD, body) and appends its post-SOD body to the addressed tile,
    // then expects the next SOT or the final EOC. Tile-parts may interleave
    // tiles; concatenation per Isot reassembles each tile's bitstream in
    // tile-part order.
    bool at_sot = true;
    while (at_sot) {
        const std::size_t sot_start = r.pos() - 2;  // the 0xFF90 just consumed
        const std::uint16_t lsot = r.U16();
        if (lsot != 10) throw std::runtime_error("jpx: unexpected SOT segment length");
        const std::uint16_t isot = r.U16();
        const std::uint32_t psot = r.U32();
        r.U8();  // TPsot — tile-part index (order is implicit in concatenation)
        r.U8();  // TNsot — number of tile-parts
        if (isot >= num_tiles) throw std::runtime_error("jpx: SOT tile index out of range");

        // Tile-part header markers up to SOD. Per-tile coding/quant overrides
        // are out of scope; informational markers are stepped over.
        for (;;) {
            if (r.Remaining() < 2) throw std::runtime_error("jpx: truncated tile-part header");
            const std::uint16_t m = ReadMarker(r);
            if (m == kSOD) break;
            const std::uint16_t lm = r.U16();
            if (lm < 2) throw std::runtime_error("jpx: invalid tile-part marker length");
            const std::size_t mend = r.pos() + (static_cast<std::size_t>(lm) - 2);
            switch (m) {
                case kCOD: case kCOC: case kQCD: case kQCC: case kRGN: case kPOC:
                    throw std::runtime_error("jpx: per-tile-header coding/quant overrides unsupported");
                case kCOM: case kPLT: case kPPT:
                    break;  // informational — step over
                default:
                    throw std::runtime_error("jpx: unexpected marker in tile-part header");
            }
            r.SeekTo(mend);
        }

        const std::size_t data_start = r.pos();
        std::size_t tp_end;
        if (psot != 0) {
            tp_end = sot_start + psot;
            if (tp_end > r.size()) throw std::runtime_error("jpx: tile-part Psot runs past end");
        } else {
            // Psot == 0 is permitted only for the last tile-part: data runs to EOC.
            std::size_t e = data_start;
            while (e + 1 < r.size() && !(static_cast<std::uint8_t>(cs[e]) == 0xFF &&
                                         static_cast<std::uint8_t>(cs[e + 1]) == 0xD9)) {
                ++e;
            }
            tp_end = e;
        }

        std::vector<std::byte>& dst = image.tiles[isot].data;
        dst.insert(dst.end(), cs.begin() + static_cast<std::ptrdiff_t>(data_start),
                   cs.begin() + static_cast<std::ptrdiff_t>(tp_end));

        r.SeekTo(tp_end);
        if (r.Remaining() < 2) { at_sot = false; break; }
        const std::uint16_t nm = ReadMarker(r);
        if (nm == kSOT) continue;
        if (nm == kEOC) break;
        throw std::runtime_error("jpx: expected SOT or EOC after tile-part");
    }

    for (const Tile& tile : image.tiles) {
        if (tile.data.empty()) throw std::runtime_error("jpx: tile with no tile-part data");
    }
    return image;
}

namespace {

// Box header — ISO/IEC 15444-1 Annex I.4 (Table I.1): LBox(4) TBox(4), with
// an 8-byte XLBox extension when LBox == 1, or "extends to end of file" when
// LBox == 0.
struct BoxHeader {
    std::uint32_t type;
    std::size_t header_len;  // bytes consumed by LBox/TBox[/XLBox] (informational)
    std::size_t content_len; // bytes of box content following the header
};

BoxHeader ReadBoxHeader(Reader& r) {
    const std::uint32_t lbox = r.U32();
    const std::uint32_t tbox = r.U32();
    if (lbox == 1) {
        const std::uint64_t xlbox = (static_cast<std::uint64_t>(r.U32()) << 32) | r.U32();
        if (xlbox < 16) throw std::runtime_error("jpx: invalid JP2 box XLBox length");
        return BoxHeader{tbox, 16, static_cast<std::size_t>(xlbox - 16)};
    }
    if (lbox == 0) {
        return BoxHeader{tbox, 8, r.Remaining()};
    }
    if (lbox < 8) throw std::runtime_error("jpx: invalid JP2 box LBox length");
    return BoxHeader{tbox, 8, static_cast<std::size_t>(lbox - 8)};
}

constexpr std::uint32_t FourCc(char a, char b, char c, char d) {
    return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) << 24) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 16) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 8) |
           static_cast<std::uint32_t>(static_cast<std::uint8_t>(d));
}

constexpr std::uint32_t kBoxSignature = FourCc('j', 'P', ' ', ' ');  // 0x6A502020
constexpr std::uint32_t kBoxJp2Header = FourCc('j', 'p', '2', 'h');
constexpr std::uint32_t kBoxColr = FourCc('c', 'o', 'l', 'r');
constexpr std::uint32_t kBoxPclr = FourCc('p', 'c', 'l', 'r');
constexpr std::uint32_t kBoxCmap = FourCc('c', 'm', 'a', 'p');
constexpr std::uint32_t kBoxCodestream = FourCc('j', 'p', '2', 'c');

// Inspect a `jp2h` (JP2 header) superbox's sub-boxes for v1-unsupported
// colour handling — ISO/IEC 15444-1 Annex I.5.3 (`pclr`/`cmap`, palettized
// colour) and the `colr` sub-box (METH == 2 is a restricted ICC profile).
// When `out_enum_cs` is non-null and an enumerated colour space (METH == 1)
// is present, its EnumCS value (Annex I.5.3.3, Table I.10 / 15444-2:
// 12 = CMYK, 16 = sRGB, 17 = greyscale, 18 = sYCC) is written there.
void CheckJp2HeaderBox(std::span<const std::byte> content, int* out_enum_cs) {
    Reader r(content.data(), content.size());
    while (r.Remaining() >= 8) {
        const BoxHeader sub = ReadBoxHeader(r);
        const std::size_t sub_start = r.pos();
        const std::size_t sub_end = sub_start + sub.content_len;
        if (sub_end > r.size()) throw std::runtime_error("jpx: JP2 sub-box runs past jp2h end");

        if (sub.type == kBoxPclr || sub.type == kBoxCmap) {
            throw std::runtime_error("jpx: palettized JP2 (pclr/cmap) is unsupported");
        }
        if (sub.type == kBoxColr) {
            Reader colr(content.data() + sub_start, sub.content_len);
            const std::uint8_t meth = colr.U8();
            if (meth == 2) {
                throw std::runtime_error("jpx: restricted ICC colour (colr METH=2) is unsupported");
            }
            // METH==1: PREC(1) + APPROX(1) + EnumCS(4, big-endian).
            if (meth == 1 && colr.Remaining() >= 6) {
                colr.U8();  // PREC (precedence) — unused
                colr.U8();  // APPROX (approximation) — unused
                const std::uint32_t enum_cs = colr.U32();
                if (out_enum_cs) *out_enum_cs = static_cast<int>(enum_cs);
            }
        }
        r.SeekTo(sub_end);
    }
}

}  // namespace

// Locate the contiguous-codestream box (`jp2c`) inside a JP2 file —
// ISO/IEC 15444-1 Annex I.
std::span<const std::byte> Jp2FindCodestream(std::span<const std::byte> jp2, int* out_enum_cs) {
    Reader r(jp2.data(), jp2.size());

    // Signature box (Annex I.5.1, Table I.2): LBox=0x0000000C, TBox='jP  ',
    // content = 0x0D0A870A.
    if (r.Remaining() < 12) throw std::runtime_error("jpx: truncated JP2 signature box");
    const BoxHeader sig = ReadBoxHeader(r);
    if (sig.type != kBoxSignature || sig.content_len != 4) {
        throw std::runtime_error("jpx: missing or malformed JP2 signature box");
    }
    const std::uint32_t sig_content = r.U32();
    if (sig_content != 0x0D0A870A) {
        throw std::runtime_error("jpx: invalid JP2 signature box content");
    }

    // Walk the remaining top-level boxes (Annex I.4) looking for `jp2h`
    // (validated for v1-unsupported colour handling) and `jp2c` (the
    // codestream payload).
    std::optional<std::span<const std::byte>> codestream;
    while (r.Remaining() >= 8) {
        const BoxHeader box = ReadBoxHeader(r);
        const std::size_t start = r.pos();
        const std::size_t end = start + box.content_len;
        if (end > r.size()) throw std::runtime_error("jpx: JP2 box runs past end of file");

        if (box.type == kBoxJp2Header) {
            CheckJp2HeaderBox(r.SpanFrom(start, end), out_enum_cs);
        } else if (box.type == kBoxCodestream) {
            codestream = r.SpanFrom(start, end);
        }

        r.SeekTo(end);
    }

    if (!codestream.has_value()) throw std::runtime_error("jpx: no jp2c (codestream) box found");
    return *codestream;
}

namespace {

// Packet-header bit reader with bit-stuffing — ISO/IEC 15444-1 Annex B.10.1.
// Bits are read MSB-first. After any 0xFF byte is consumed, the following
// byte contributes only its low 7 bits (its MSB is a stuff bit inserted by
// the encoder so that no 0xFFxx sequence in a packet header is mistaken for
// a marker).
class PacketBitReader {
public:
    PacketBitReader(const std::uint8_t* p, std::size_t len) : p_(p), len_(len) {}

    int ReadBit() {
        if (bits_left_ == 0) Load();
        --bits_left_;
        return (cur_ >> bits_left_) & 1;
    }

    std::uint32_t ReadBits(int n) {
        std::uint32_t v = 0;
        for (int i = 0; i < n; ++i) v = (v << 1) | static_cast<std::uint32_t>(ReadBit());
        return v;
    }

    // Advance to the next byte boundary. Per Annex B.10.1, if the byte most
    // recently consumed was 0xFF, the reader is already positioned with at
    // most 7 valid bits in `cur_`; discarding any remainder is sufficient —
    // the stuff-bit accounting is handled transparently by Load().
    void Align() { bits_left_ = 0; }

    // Number of bytes consumed from the underlying buffer so far (i.e. the
    // byte offset immediately following the last loaded byte). After
    // Align(), this is the offset of the packet body.
    std::size_t BytePos() const { return pos_; }

private:
    void Load() {
        const std::uint8_t b = (pos_ < len_) ? p_[pos_] : 0xFF;
        ++pos_;
        if (last_ == 0xFF) {
            cur_ = b & 0x7F;
            bits_left_ = 7;
        } else {
            cur_ = b;
            bits_left_ = 8;
        }
        last_ = b;
    }

    const std::uint8_t* p_;
    std::size_t len_;
    std::size_t pos_ = 0;
    std::uint8_t cur_ = 0;
    int bits_left_ = 0;
    std::uint8_t last_ = 0x00;
};

// Hierarchical tag tree over a w*h grid of leaves — ISO/IEC 15444-1 Annex
// B.10.2. A pyramid of nodes is built (each level ceil(w/2)*ceil(h/2) of the
// one below, down to a 1*1 root); a leaf's value is decoded by walking
// root->leaf, each node carrying a running lower bound. Node state is retained
// between calls so successive queries (rising thresholds for the same leaf, or
// later quality layers) resume rather than restart. Algorithm and node
// semantics mirror OpenJPEG's opj_tgt_decode (value initialised "infinite",
// fixed to the bound on a 1-bit). A 1*1 tree behaves exactly like a single
// inclusion/zero-bit-plane node (the prior single-code-block-per-subband case).
class TagTree {
public:
    TagTree(int w, int h) {
        int lw = w, lh = h;
        std::size_t total = 0;
        for (;;) {
            offsets_.push_back(total);
            level_w_.push_back(lw);
            total += static_cast<std::size_t>(lw) * static_cast<std::size_t>(lh);
            if (lw <= 1 && lh <= 1) break;
            lw = (lw + 1) / 2;
            lh = (lh + 1) / 2;
        }
        nodes_.assign(total, Node{});
        levels_ = static_cast<int>(offsets_.size());
    }

    // Decode whether leaf (lx,ly)'s value is below `threshold` (Annex B.10.2).
    bool Decode(PacketBitReader& r, int lx, int ly, int threshold) {
        int low = 0;
        for (int level = levels_ - 1; level >= 0; --level) {
            Node& node = nodes_[offsets_[static_cast<std::size_t>(level)] +
                                 static_cast<std::size_t>((ly >> level)) * level_w_[static_cast<std::size_t>(level)] +
                                 (lx >> level)];
            if (low > node.low) node.low = low; else low = node.low;
            while (low < threshold && low < node.value) {
                if (r.ReadBit()) node.value = low; else ++low;
            }
            node.low = low;
        }
        return nodes_[static_cast<std::size_t>(ly) * level_w_[0] + lx].value < threshold;
    }

    // Fully resolve a leaf's value (used for the zero-bit-plane count): raise
    // the threshold until the value is pinned down.
    int DecodeValue(PacketBitReader& r, int lx, int ly) {
        int t = 0;
        while (!Decode(r, lx, ly, t + 1)) ++t;
        return t;
    }

private:
    struct Node { int value = (1 << 30); int low = 0; };
    std::vector<Node> nodes_;
    std::vector<std::size_t> offsets_;
    std::vector<int> level_w_;
    int levels_ = 0;
};

// Decode the number of coding passes for a newly-included code-block —
// ISO/IEC 15444-1 Annex B.10.6, Table B.4.
int DecodeNumPasses(PacketBitReader& r) {
    if (r.ReadBit() == 0) return 1;
    if (r.ReadBit() == 0) return 2;
    const std::uint32_t v = r.ReadBits(2);
    if (v < 3) return 3 + static_cast<int>(v);
    const std::uint32_t w = r.ReadBits(5);
    if (w < 31) return 6 + static_cast<int>(w);
    return 37 + static_cast<int>(r.ReadBits(7));
}

// floor(log2(n)) for n >= 1 — used for the code-block segment-length field
// width (Annex B.10.7).
int FloorLog2(int n) {
    int b = 0;
    while (n > 1) {
        n >>= 1;
        ++b;
    }
    return b;
}

}  // namespace

// Test-only helper: fully decode a `w`x`h`-leaf hierarchical tag tree (Annex
// B.10.2) from a packed, MSB-first, bit-stuffed bitstream, returning each
// leaf's resolved value in row-major (by-major) order. Lets the unit tests
// exercise the tag tree without a full packet.
std::vector<int> TagTreeDecodeAll(std::span<const std::uint8_t> bits, int w, int h) {
    PacketBitReader r(bits.data(), bits.size());
    TagTree tree(w, h);
    std::vector<int> out;
    out.reserve(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            out.push_back(tree.DecodeValue(r, x, y));
        }
    }
    return out;
}

// One code-block's tier-2 packet metadata, sufficient to locate and decode
// its tier-1 (EBCOT) bitstream — ISO/IEC 15444-1 Annex B.10.
struct CodeBlockInfo {
    bool included = false;
    const std::uint8_t* data = nullptr;
    std::size_t length = 0;
    int num_passes = 0;
    int num_zero_bitplanes = 0;
    int width = 0;
    int height = 0;
    int orient = 0;  // 0=LL,1=HL,2=LH,3=HH
};

// One subband's geometry within a resolution level, used to drive the
// per-subband/per-code-block loop in ParsePacket — ISO/IEC 15444-1 Annex B.10.
struct SubbandLayout {
    int width;
    int height;
    int cb_w;
    int cb_h;
    int orient;
};

// Parse one LRCP, single-layer, single-precinct-per-subband packet for a
// resolution level — ISO/IEC 15444-1 Annex B.10. `subbands` describes each
// subband's code-block grid (one entry per subband at this resolution: just
// the LL band at resolution 0, or HL/LH/HH at resolution >= 1). Returns the
// per-code-block metadata (in subband, then code-block, order) and advances
// nothing in `tile`; the caller slices the packet body using the returned
// `PacketBitReader::BytePos()`-derived offset.
std::vector<CodeBlockInfo> ParsePacket(const std::uint8_t* tile, std::size_t tile_len,
                                        const std::vector<SubbandLayout>& subbands,
                                        std::size_t& body_offset) {
    PacketBitReader r(tile, tile_len);

    std::vector<CodeBlockInfo> blocks;

    const int present = r.ReadBit();
    if (present == 0) {
        r.Align();
        body_offset = r.BytePos();
        return blocks;
    }

    // Per-subband inclusion / zero-bit-plane tag trees (one leaf each, since
    // this task covers a single code-block per subband).
    for (const SubbandLayout& sb : subbands) {
        const int nbx = (sb.width + sb.cb_w - 1) / sb.cb_w;
        const int nby = (sb.height + sb.cb_h - 1) / sb.cb_h;

        // One inclusion and one zero-bit-plane tag tree per subband (single
        // precinct), shared across the subband's code-blocks — Annex B.10.2.
        TagTree incl_tree(nbx, nby);
        TagTree zbp_tree(nbx, nby);

        for (int by = 0; by < nby; ++by) {
            for (int bx = 0; bx < nbx; ++bx) {
                const int cb_w = std::min(sb.cb_w, sb.width - bx * sb.cb_w);
                const int cb_h = std::min(sb.cb_h, sb.height - by * sb.cb_h);

                CodeBlockInfo info;
                info.width = cb_w;
                info.height = cb_h;
                info.orient = sb.orient;

                // Inclusion at quality layer 0 -> threshold 1 (Annex B.10.4).
                if (incl_tree.Decode(r, bx, by, 1)) {
                    info.included = true;
                    info.num_zero_bitplanes = zbp_tree.DecodeValue(r, bx, by);
                    info.num_passes = DecodeNumPasses(r);

                    int lblock = 3;
                    while (r.ReadBit() == 1) ++lblock;

                    const int nbits = lblock + FloorLog2(info.num_passes);
                    info.length = r.ReadBits(nbits);
                }

                blocks.push_back(info);
            }
        }
    }

    r.Align();
    body_offset = r.BytePos();
    return blocks;
}

// Ceiling division for non-negative results — used by the subband
// dimension formula (Eq B-15) below, where all the arguments that occur in
// practice for these fixtures keep the numerator non-negative; negative
// numerators (the `xob`/`yob` subtrahend term) are handled by FloorDiv via
// ceil(a/b) = -floor(-a/b).
std::int32_t CeilDiv(std::int32_t num, std::int32_t den) {
    return -FloorDiv(-num, den);
}

// Subband dimensions — ISO/IEC 15444-1 Eq (B-15). `nb` is the decomposition
// level the subband belongs to (origin-1: nb=1 is the finest detail level);
// (xob, yob) is the subband's orientation offset (LL=(0,0), HL=(1,0),
// LH=(0,1), HH=(1,1)). `w`/`h` are the full tile-component dimensions
// (origin 0, no sub-sampling — v1 scope).
void SubbandDims(std::int32_t w, std::int32_t h, int nb, int xob, int yob, int& sbw, int& sbh) {
    const std::int32_t two_nb = 1 << nb;
    // nb == 0 only occurs for the LL band at resolution 0 when NL == 0,
    // where xob == yob == 0 and the 2^(nb-1)*xob/yob subtrahend term is
    // therefore always zero; avoid the otherwise-undefined 2^(-1) shift.
    const std::int32_t two_nb1 = (nb >= 1) ? (1 << (nb - 1)) : 0;
    sbw = static_cast<int>(CeilDiv(w - two_nb1 * xob, two_nb) - CeilDiv(0 - two_nb1 * xob, two_nb));
    sbh = static_cast<int>(CeilDiv(h - two_nb1 * yob, two_nb) - CeilDiv(0 - two_nb1 * yob, two_nb));
}

// Quantization exponent epsilon_b for a subband at resolution r (0 = the
// coarsest LL) and orientation — ISO/IEC 15444-1 Annex A.6.4 / E.1. Styles 0
// (none) and 2 (expounded) carry one entry per subband in codestream
// (resolution-major) order; style 1 (derived) carries only the LL entry and
// derives the rest by Eq E-5 (eps_b = eps_0 - (NL - n_b), n_b = NL - r + 1).
int SubbandExpn(const Image& image, int r, int orient) {
    if (image.quant.empty()) return image.components[0].bit_depth;
    if (image.quant_style == 1) {
        return (r == 0) ? image.quant[0].exponent : image.quant[0].exponent - r + 1;
    }
    const std::size_t qi = (r == 0) ? 0 : static_cast<std::size_t>(3 * (r - 1) + orient);
    return image.quant[std::min(qi, image.quant.size() - 1)].exponent;
}

// Maximum magnitude bit-plane count Mb = epsilon_b + G - 1 (Eq E-2).
int SubbandMb(const Image& image, int r, int orient) {
    return SubbandExpn(image, r, orient) + image.guard_bits - 1;
}

// EBCOT bit-plane coder state for one code-block sample — ISO/IEC 15444-1
// Annex D.
struct SampleState {
    bool sig = false;
    bool sign = false;
    std::int32_t mag = 0;
    bool visited = false;
    bool refined_once = false;
};

// ZC (zero-coding) context label — ISO/IEC 15444-1 Annex D, Tables D.1-D.3.
// `h`/`v`/`d` are the counts of significant horizontal, vertical, and
// diagonal neighbours (0-2, 0-2, 0-4 respectively).
int ZcContext(int orient, int h, int v, int d) {
    switch (orient) {
        case 0:  // LL — Table D.1
        case 2:  // LH — Table D.1
            if (h == 2) return 8;
            if (h == 1) {
                if (v >= 1) return 7;
                if (d >= 1) return 6;
                return 5;
            }
            if (v == 2) return 4;
            if (v == 1) return 3;
            if (d >= 2) return 2;
            if (d == 1) return 1;
            return 0;
        case 1:  // HL — Table D.2 (H and V swapped relative to D.1)
            if (v == 2) return 8;
            if (v == 1) {
                if (h >= 1) return 7;
                if (d >= 1) return 6;
                return 5;
            }
            if (h == 2) return 4;
            if (h == 1) return 3;
            if (d >= 2) return 2;
            if (d == 1) return 1;
            return 0;
        default: {  // HH — Table D.3
            const int hv = h + v;
            if (d >= 3) return 8;
            if (d == 2) return (hv >= 1) ? 7 : 6;
            if (d == 1) {
                if (hv >= 2) return 5;
                if (hv == 1) return 4;
                return 3;
            }
            if (hv >= 2) return 2;
            if (hv == 1) return 1;
            return 0;
        }
    }
}

// Decode one EBCOT code-block — ISO/IEC 15444-1 Annex D. Returns w*h signed
// coefficients in row-major order. `orient`: 0=LL, 1=HL, 2=LH, 3=HH.
//
// `mb` is the subband's maximum number of magnitude bit-planes Mb = epsilon_b +
// G - 1 (Eq E-2). `num_zero_bitplanes` (from the tier-2 packet header, Annex
// B.10.5, via the zero-bit-plane tag-tree) is the count of all-zero
// most-significant bit-planes; the first coded pass is a cleanup pass for the
// most-significant non-zero bit-plane, whose index is therefore
// `Mb - 1 - num_zero_bitplanes`. For a fully decoded (e.g. lossless) block this
// equals `(num_passes - 1) / 3`, but for a truncated lossy block — which stops
// above plane 0 — only the `Mb`-derived index gives the correct bit weights.
std::vector<std::int32_t> DecodeCodeBlock(const std::uint8_t* data, std::size_t len, int w, int h,
                                           int num_passes, int num_zero_bitplanes, int orient, int mb,
                                           bool reversible) {
    MqDecoder mq(data, len);
    std::array<MqContext, 19> ctx{};
    // Initial context states — ISO/IEC 15444-1 Table D.7. Only three
    // contexts take a non-zero start state: the all-zero-neighbour ZC
    // context (index 4), the run-length context (index 3), and the UNIFORM
    // context (index 46). Every other context — including all magnitude-
    // refinement contexts (14/15/16) — starts at index 0.
    ctx[0].index = 4;
    ctx[17].index = 3;
    ctx[18].index = 46;

    std::vector<SampleState> state(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    auto idx = [w](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x); };
    auto in_bounds = [w, h](int x, int y) { return x >= 0 && x < w && y >= 0 && y < h; };
    auto is_sig = [&](int x, int y) { return in_bounds(x, y) && state[idx(x, y)].sig; };
    auto get_sign = [&](int x, int y) { return state[idx(x, y)].sign; };

    auto decode_bit = [&](int c) { return mq.Decode(ctx[static_cast<std::size_t>(c)]); };

    auto sign_decode = [&](int x, int y) {
        auto contrib = [&](int nx, int ny) -> int {
            if (!is_sig(nx, ny)) return 0;
            return get_sign(nx, ny) ? -1 : 1;
        };
        int hc = contrib(x - 1, y) + contrib(x + 1, y);
        hc = std::clamp(hc, -1, 1);
        int vc = contrib(x, y - 1) + contrib(x, y + 1);
        vc = std::clamp(vc, -1, 1);

        int sc_ctx;
        int xorbit;
        if (hc == 1) {
            if (vc == 1) { sc_ctx = 13; xorbit = 0; }
            else if (vc == 0) { sc_ctx = 12; xorbit = 0; }
            else { sc_ctx = 11; xorbit = 0; }
        } else if (hc == 0) {
            if (vc == 1) { sc_ctx = 10; xorbit = 0; }
            else if (vc == 0) { sc_ctx = 9; xorbit = 0; }
            else { sc_ctx = 10; xorbit = 1; }
        } else {
            if (vc == 1) { sc_ctx = 11; xorbit = 1; }
            else if (vc == 0) { sc_ctx = 12; xorbit = 1; }
            else { sc_ctx = 13; xorbit = 1; }
        }

        const int d = decode_bit(sc_ctx);
        state[idx(x, y)].sign = (d ^ xorbit) != 0;
    };

    auto neighbour_counts = [&](int x, int y, int& hh, int& vv, int& dd) {
        hh = (is_sig(x - 1, y) ? 1 : 0) + (is_sig(x + 1, y) ? 1 : 0);
        vv = (is_sig(x, y - 1) ? 1 : 0) + (is_sig(x, y + 1) ? 1 : 0);
        dd = (is_sig(x - 1, y - 1) ? 1 : 0) + (is_sig(x - 1, y + 1) ? 1 : 0) +
             (is_sig(x + 1, y - 1) ? 1 : 0) + (is_sig(x + 1, y + 1) ? 1 : 0);
    };

    auto mr_context = [&](int x, int y) {
        SampleState& s = state[idx(x, y)];
        if (!s.refined_once) {
            int nsum = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    if (is_sig(x + dx, y + dy)) ++nsum;
                }
            }
            return (nsum == 0) ? 14 : 15;
        }
        return 16;
    };

    // Horizontal stripes of 4 rows (Annex D.10, Figure D.x scan order).
    struct Stripe { int y0, h; };
    std::vector<Stripe> stripes;
    for (int y = 0; y < h; y += 4) stripes.push_back({y, std::min(4, h - y)});

    // The first (most-significant) coded bit-plane is decoded by a CLEANUP
    // pass only; every lower plane has SPP, MRP, CUP in that order (Annex D.3).
    // Its index is Mb - 1 - num_zero_bitplanes (Eq E-2 / Annex B.10.5).
    int plane = mb - 1 - num_zero_bitplanes;

    int pass_idx = 0;
    int lowest_plane = plane;  // bit-plane of the last coding pass executed
    while (pass_idx < num_passes) {
        lowest_plane = plane;
        int pass_type;  // 0 = CUP, 1 = SPP, 2 = MRP
        if (pass_idx == 0) {
            pass_type = 0;
        } else {
            const int m = (pass_idx - 1) % 3;
            pass_type = (m == 0) ? 1 : (m == 1) ? 2 : 0;
        }

        // The "visited this bit-plane" membership flag is reset once per
        // bit-plane — before the significance-propagation pass (and before
        // the lone cleanup pass of the first coded plane) — NOT before every
        // pass. This is what keeps magnitude refinement (Annex D.3.3) from
        // refining a coefficient that became significant earlier in the SAME
        // bit-plane: such a coefficient is marked visited by SPP/CUP and the
        // MRP pass skips it, matching the encoder which codes no refinement
        // bit for it until the next plane.
        if (pass_idx == 0 || pass_type == 1) {
            for (auto& s : state) s.visited = false;
        }

        if (pass_type == 1) {  // SPP
            for (const Stripe& st : stripes) {
                for (int x = 0; x < w; ++x) {
                    for (int dy = 0; dy < st.h; ++dy) {
                        const int y = st.y0 + dy;
                        SampleState& s = state[idx(x, y)];
                        if (s.sig) continue;
                        int hh, vv, dd;
                        neighbour_counts(x, y, hh, vv, dd);
                        if (hh + vv + dd == 0) continue;
                        const int c = ZcContext(orient, hh, vv, dd);
                        const int d = decode_bit(c);
                        s.visited = true;
                        if (d == 1) {
                            s.sig = true;
                            s.mag |= (1 << plane);
                            sign_decode(x, y);
                        }
                    }
                }
            }
        } else if (pass_type == 2) {  // MRP
            for (const Stripe& st : stripes) {
                for (int x = 0; x < w; ++x) {
                    for (int dy = 0; dy < st.h; ++dy) {
                        const int y = st.y0 + dy;
                        SampleState& s = state[idx(x, y)];
                        if (!s.sig || s.visited) continue;
                        const int c = mr_context(x, y);
                        const int d = decode_bit(c);
                        s.mag |= (d << std::max(plane, 0));
                        s.refined_once = true;
                        s.visited = true;
                    }
                }
            }
        } else {  // CUP (cleanup)
            for (const Stripe& st : stripes) {
                for (int x = 0; x < w; ++x) {
                    bool col_ok = (st.h == 4);
                    if (col_ok) {
                        for (int dy = 0; dy < 4; ++dy) {
                            const int y = st.y0 + dy;
                            const SampleState& s = state[idx(x, y)];
                            if (s.sig || s.visited) { col_ok = false; break; }
                            int hh, vv, dd;
                            neighbour_counts(x, y, hh, vv, dd);
                            if (hh + vv + dd != 0) { col_ok = false; break; }
                        }
                    }
                    if (col_ok) {
                        const int r = decode_bit(17);
                        if (r == 0) {
                            for (int dy = 0; dy < 4; ++dy) state[idx(x, st.y0 + dy)].visited = true;
                        } else {
                            const int k_hi = decode_bit(18);
                            const int k_lo = decode_bit(18);
                            const int k = (k_hi << 1) | k_lo;
                            for (int dy = 0; dy < 4; ++dy) {
                                const int y = st.y0 + dy;
                                SampleState& s = state[idx(x, y)];
                                s.visited = true;
                                if (dy < k) continue;
                                if (dy == k) {
                                    s.sig = true;
                                    s.mag |= (1 << plane);
                                    sign_decode(x, y);
                                } else {
                                    const int c = ZcContext(orient, [&] { int hh, vv, dd; neighbour_counts(x, y, hh, vv, dd); return hh; }(),
                                                             [&] { int hh, vv, dd; neighbour_counts(x, y, hh, vv, dd); return vv; }(),
                                                             [&] { int hh, vv, dd; neighbour_counts(x, y, hh, vv, dd); return dd; }());
                                    const int d = decode_bit(c);
                                    if (d == 1) {
                                        s.sig = true;
                                        s.mag |= (1 << plane);
                                        sign_decode(x, y);
                                    }
                                }
                            }
                        }
                    } else {
                        for (int dy = 0; dy < st.h; ++dy) {
                            const int y = st.y0 + dy;
                            SampleState& s = state[idx(x, y)];
                            if (s.visited || s.sig) continue;
                            int hh, vv, dd;
                            neighbour_counts(x, y, hh, vv, dd);
                            const int c = ZcContext(orient, hh, vv, dd);
                            const int d = decode_bit(c);
                            if (d == 1) {
                                s.sig = true;
                                s.mag |= (1 << plane);
                                sign_decode(x, y);
                            }
                        }
                    }
                }
            }
        }
        if (pass_type == 0) --plane;
        ++pass_idx;
    }

    // Reconstruction. For the reversible (5/3) path the coefficient is the
    // exact decoded magnitude. For the irreversible (9/7) path return the
    // mid-point reconstruction value in OpenJPEG's "doubled" units:
    // datap = 2*mag + 2^p (p = lowest decoded bit-plane), so the caller's
    // dequantization datap * 0.5 * delta places the value at the centre of the
    // remaining uncertainty interval (Annex E.1). p == 0 (fully decoded) gives
    // the familiar 2*mag + 1.
    const int p_min = std::max(lowest_plane, 0);
    std::vector<std::int32_t> out(state.size());
    for (std::size_t i = 0; i < state.size(); ++i) {
        const std::int32_t m = state[i].mag;
        std::int32_t v;
        if (reversible) {
            v = m;
        } else if (state[i].sig) {
            v = 2 * m + (1 << p_min);
        } else {
            v = 0;
        }
        out[i] = state[i].sign ? -v : v;
    }
    return out;
}

// Tier-2 packet walk for one tile-component set, filling the per-component
// Mallat-arranged coefficient planes — ISO/IEC 15444-1 Annex B.10/B.12. Walks
// packets in the codestream's progression order (LRCP or RLCP) and accumulates
// each code-block's coding passes and codeword bytes across all quality layers
// before a single tier-1 (EBCOT) decode (default code-block style ⇒ one
// codeword segment per block, so the per-layer byte runs concatenate). One
// precinct per subband (default full-subband precincts). `tile` is the tile's
// concatenated bitstream; `w`/`h` are the tile-component dimensions.
void DecodeTilePackets(const Image& image, const std::uint8_t* tile, std::size_t tile_len,
                       int w, int h, std::size_t ncomp, int cb_w_max, int cb_h_max,
                       std::vector<std::vector<std::int32_t>>& planes) {
    const int nl = image.levels;
    const int layers = image.layers;

    struct Cblk {
        bool included = false;
        int num_zero_bitplanes = 0;
        int total_passes = 0;
        int lblock = 3;
        std::vector<std::uint8_t> data;
        int px = 0, py = 0, bw = 0, bh = 0, orient = 0;
    };
    struct Sub {
        int nbx = 0, nby = 0;
        std::optional<TagTree> incl;  // shared inclusion tag tree (across layers)
        std::optional<TagTree> zbp;   // shared zero-bit-plane tag tree
        std::vector<Cblk> blocks;     // nbx*nby, row-major
    };
    // state[r][c] = the subbands of resolution r for component c.
    std::vector<std::vector<std::vector<Sub>>> state(
        static_cast<std::size_t>(nl + 1), std::vector<std::vector<Sub>>(ncomp));

    for (int r = 0; r <= nl; ++r) {
        std::vector<SubbandLayout> sbs;
        std::vector<std::pair<int, int>> offs;  // quadrant placement (ox, oy)
        if (r == 0) {
            int sbw, sbh;
            SubbandDims(w, h, nl, 0, 0, sbw, sbh);
            sbs.push_back(SubbandLayout{sbw, sbh, cb_w_max, cb_h_max, 0});
            offs.emplace_back(0, 0);
        } else {
            const int nb = nl - r + 1;
            const int low_w = static_cast<int>(CeilDiv(w, 1 << (nl - r + 1)));
            const int low_h = static_cast<int>(CeilDiv(h, 1 << (nl - r + 1)));
            int a, b;
            SubbandDims(w, h, nb, 1, 0, a, b);
            sbs.push_back(SubbandLayout{a, b, cb_w_max, cb_h_max, 1});
            offs.emplace_back(low_w, 0);
            SubbandDims(w, h, nb, 0, 1, a, b);
            sbs.push_back(SubbandLayout{a, b, cb_w_max, cb_h_max, 2});
            offs.emplace_back(0, low_h);
            SubbandDims(w, h, nb, 1, 1, a, b);
            sbs.push_back(SubbandLayout{a, b, cb_w_max, cb_h_max, 3});
            offs.emplace_back(low_w, low_h);
        }
        for (std::size_t c = 0; c < ncomp; ++c) {
            for (std::size_t s = 0; s < sbs.size(); ++s) {
                const SubbandLayout& L = sbs[s];
                const auto [ox, oy] = offs[s];
                Sub sub;
                sub.nbx = (L.width + L.cb_w - 1) / L.cb_w;
                sub.nby = (L.height + L.cb_h - 1) / L.cb_h;
                sub.incl.emplace(sub.nbx, sub.nby);
                sub.zbp.emplace(sub.nbx, sub.nby);
                sub.blocks.resize(static_cast<std::size_t>(sub.nbx) * sub.nby);
                for (int by = 0; by < sub.nby; ++by) {
                    for (int bx = 0; bx < sub.nbx; ++bx) {
                        Cblk& blk = sub.blocks[static_cast<std::size_t>(by) * sub.nbx + bx];
                        blk.px = ox + bx * L.cb_w;
                        blk.py = oy + by * L.cb_h;
                        blk.bw = std::min(L.cb_w, L.width - bx * L.cb_w);
                        blk.bh = std::min(L.cb_h, L.height - by * L.cb_h);
                        blk.orient = L.orient;
                    }
                }
                state[static_cast<std::size_t>(r)][c].push_back(std::move(sub));
            }
        }
    }

    std::size_t cursor = 0;
    auto parse_packet = [&](int r, int l, std::size_t c) {
        PacketBitReader pr(tile + cursor, tile_len - cursor);
        if (pr.ReadBit() == 0) {  // empty packet — Annex B.10.3
            pr.Align();
            cursor += pr.BytePos();
            return;
        }
        std::vector<std::pair<Cblk*, std::size_t>> segs;
        for (Sub& sub : state[static_cast<std::size_t>(r)][c]) {
            for (int by = 0; by < sub.nby; ++by) {
                for (int bx = 0; bx < sub.nbx; ++bx) {
                    Cblk& blk = sub.blocks[static_cast<std::size_t>(by) * sub.nbx + bx];
                    bool contributes = false;
                    if (!blk.included) {
                        // First inclusion via the tag tree at threshold layer+1
                        // (Annex B.10.4).
                        if (sub.incl->Decode(pr, bx, by, l + 1)) {
                            blk.included = true;
                            blk.num_zero_bitplanes = sub.zbp->DecodeValue(pr, bx, by);
                            blk.lblock = 3;
                            contributes = true;
                        }
                    } else {
                        contributes = (pr.ReadBit() == 1);
                    }
                    if (contributes) {
                        const int newpasses = DecodeNumPasses(pr);
                        while (pr.ReadBit() == 1) ++blk.lblock;
                        const int nbits = blk.lblock + FloorLog2(newpasses);
                        const std::size_t seglen = pr.ReadBits(nbits);
                        blk.total_passes += newpasses;
                        segs.emplace_back(&blk, seglen);
                    }
                }
            }
        }
        pr.Align();
        std::size_t body = cursor + pr.BytePos();
        for (auto& [blk, seglen] : segs) {
            blk->data.insert(blk->data.end(), tile + body, tile + body + seglen);
            body += seglen;
        }
        cursor = body;
    };

    if (image.progression == 0) {  // LRCP: layer, resolution, component
        for (int l = 0; l < layers; ++l)
            for (int r = 0; r <= nl; ++r)
                for (std::size_t c = 0; c < ncomp; ++c) parse_packet(r, l, c);
    } else {  // RLCP: resolution, layer, component
        for (int r = 0; r <= nl; ++r)
            for (int l = 0; l < layers; ++l)
                for (std::size_t c = 0; c < ncomp; ++c) parse_packet(r, l, c);
    }

    // Tier-1 decode each included block and place it into the component plane.
    for (int r = 0; r <= nl; ++r) {
        for (std::size_t c = 0; c < ncomp; ++c) {
            for (Sub& sub : state[static_cast<std::size_t>(r)][c]) {
                for (Cblk& blk : sub.blocks) {
                    if (!blk.included) continue;
                    const std::vector<std::int32_t> coeffs = DecodeCodeBlock(
                        blk.data.data(), blk.data.size(), blk.bw, blk.bh,
                        blk.total_passes, blk.num_zero_bitplanes, blk.orient,
                        SubbandMb(image, r, blk.orient),
                        image.wavelet == Wavelet::Reversible53);
                    for (int yy = 0; yy < blk.bh; ++yy) {
                        for (int xx = 0; xx < blk.bw; ++xx) {
                            planes[c][static_cast<std::size_t>(blk.py + yy) * static_cast<std::size_t>(w) +
                                      static_cast<std::size_t>(blk.px + xx)] =
                                coeffs[static_cast<std::size_t>(yy) * static_cast<std::size_t>(blk.bw) +
                                       static_cast<std::size_t>(xx)];
                        }
                    }
                }
            }
        }
    }
}

}  // namespace detail

DecodedImage Decode(std::span<const std::byte> input) {
    if (input.empty()) {
        throw std::runtime_error("jpx::Decode: empty input");
    }

    // Framing detect — ISO/IEC 15444-1 Annex A.4.1 (raw codestream, starts
    // with SOC 0xFF4F) or Annex I.5.1 (JP2 file, starts with the 12-byte
    // signature box).
    std::span<const std::byte> codestream;
    int enum_cs = -1;  // -1 = unknown (raw codestream or no colr box)
    if (input.size() >= 2 && static_cast<std::uint8_t>(input[0]) == 0xFF &&
        static_cast<std::uint8_t>(input[1]) == 0x4F) {
        codestream = input;
    } else if (input.size() >= 12) {
        codestream = detail::Jp2FindCodestream(input, &enum_cs);
    } else {
        throw std::runtime_error("jpx::Decode: not a JPEG 2000 codestream or JP2 file");
    }

    const detail::Image image = detail::ParseCodestream(codestream);

    // v1 scope: 5/3 reversible or 9/7 irreversible; 1, 3, or 4 components.
    // The 3-component colour transform is RCT/ICT (Annex G); the 4-component
    // case is CMYK (colr EnumCS 12), decoded via the 9/7 path the real-world
    // images use (5/3 CMYK does not occur in scope and is rejected below).
    const std::size_t ncomp = image.components.size();
    const bool cmyk = (ncomp == 4);
    if (cmyk && enum_cs != 12) {
        throw std::runtime_error("jpx::Decode: 4-component non-CMYK colour space unsupported");
    }
    if (cmyk && image.wavelet != detail::Wavelet::Irreversible97) {
        throw std::runtime_error("jpx::Decode: 4-component CMYK requires the 9/7 path");
    }
    if (ncomp != 1 && ncomp != 3 && ncomp != 4) {
        throw std::runtime_error("jpx::Decode: unsupported component count (expected 1, 3, or 4)");
    }

    // Output canvas (RGBA). Each tile is decoded independently and composited
    // at its canvas offset — ISO/IEC 15444-1 Annex B.3.
    DecodedImage out;
    out.width = image.width;
    out.height = image.height;
    out.components = 4;
    out.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4,
                      std::byte{0});
    const std::size_t canvas_w = image.width;

    const int nl = image.levels;
    const int cb_w_max = 1 << (image.cb_w_exp + 2);
    const int cb_h_max = 1 << (image.cb_h_exp + 2);

    for (const detail::Tile& current_tile : image.tiles) {
        const int w = static_cast<int>(current_tile.x1 - current_tile.x0);
        const int h = static_cast<int>(current_tile.y1 - current_tile.y0);
        const auto* tile = reinterpret_cast<const std::uint8_t*>(current_tile.data.data());
        const std::size_t tile_len = current_tile.data.size();

    // Per-component planes in the standard Mallat quadrant arrangement —
    // ISO/IEC 15444-1 Annex F.4. The inverse DWT operates in place on each.
    std::vector<std::vector<std::int32_t>> planes(
        ncomp, std::vector<std::int32_t>(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0));

    // Tier-2 packet walk (LRCP/RLCP, all quality layers, one precinct per
    // subband) — accumulates each code-block's coding passes across layers and
    // fills `planes` with the quantized coefficients (Annex B.10/B.12).
    detail::DecodeTilePackets(image, tile, tile_len, w, h, ncomp, cb_w_max, cb_h_max, planes);

    const detail::Component& comp0 = image.components[0];
    const std::int32_t shift = 1 << (comp0.bit_depth - 1);
    const std::int32_t max_val = (1 << comp0.bit_depth) - 1;
    const std::size_t npix = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
    const bool irreversible = (image.wavelet == detail::Wavelet::Irreversible97);

    // Per-pixel R/G/B in the level-shifted (pre-DC-shift) domain. The 5/3 and
    // 9/7 tracks fill these differently (integer synthesis + RCT vs.
    // dequantised float synthesis + ICT) but share the final DC level-shift,
    // clamp, and RGBA packing below.
    std::vector<std::int32_t> r_plane(npix), g_plane(npix), b_plane(npix);

    if (!irreversible) {
        // Inverse 5/3 DWT, per component, NL decomposition levels — ISO/IEC
        // 15444-1 Annex F.3.7/F.4. The coefficients are exact integers.
        for (std::size_t c = 0; c < ncomp; ++c) {
            detail::Idwt2dMallat53(planes[c], static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), nl);
        }
        for (std::size_t i = 0; i < npix; ++i) {
            if (ncomp == 3) {
                // Inverse reversible colour transform (RCT) — ISO/IEC 15444-1
                // Annex G.2, Eq (G-7..G-9). Planes are (Y, Ub, Vr).
                const std::int32_t y_val = planes[0][i];
                const std::int32_t ub = planes[1][i];
                const std::int32_t vr = planes[2][i];
                const std::int32_t g_centered = y_val - detail::FloorDiv(ub + vr, 4);
                r_plane[i] = vr + g_centered;
                g_plane[i] = g_centered;
                b_plane[i] = ub + g_centered;
            } else {
                r_plane[i] = g_plane[i] = b_plane[i] = planes[0][i];
            }
        }
    } else {
        // Inverse quantization (ISO/IEC 15444-1 Annex E.1, Eq E-3..E-5) feeding
        // the irreversible 9/7 synthesis. Each subband's quantized integer
        // coefficient q is reconstructed mid-point as sign*(|q| + 0.5)*delta_b
        // (the reconstruction parameter r = 0.5; insignificant samples stay 0),
        // with the step size delta_b = (1 + mu_b/2^11) * 2^(R_b - eps_b),
        // R_b = bit_depth + log2(gain_b) (Eq E-4 sub-band gains: LL 0, HL/LH 1,
        // HH 2).
        const int bit_depth = comp0.bit_depth;
        auto subband_delta = [&](int r, int orient) -> double {
            const int gain = (orient == 0) ? 0 : (orient == 3) ? 2 : 1;
            const int rb = bit_depth + gain;
            const int expn = detail::SubbandExpn(image, r, orient);
            int mant;
            if (image.quant.empty()) {
                mant = 0;
            } else if (image.quant_style == 1) {  // derived: mantissa shared with the LL entry
                mant = image.quant[0].mantissa;
            } else {  // none / expounded: one entry per subband
                const std::size_t qi = (r == 0) ? 0 : static_cast<std::size_t>(3 * (r - 1) + orient);
                mant = image.quant[std::min(qi, image.quant.size() - 1)].mantissa;
            }
            return (1.0 + static_cast<double>(mant) / 2048.0) * std::ldexp(1.0, rb - expn);
        };

        std::vector<std::vector<double>> fplanes(ncomp, std::vector<double>(npix, 0.0));
        for (std::size_t c = 0; c < ncomp; ++c) {
            auto put = [&](int x0, int y0, int bw, int bh, double delta) {
                for (int yy = 0; yy < bh; ++yy) {
                    for (int xx = 0; xx < bw; ++xx) {
                        const std::size_t k =
                            static_cast<std::size_t>(y0 + yy) * static_cast<std::size_t>(w) +
                            static_cast<std::size_t>(x0 + xx);
                        // planes hold datap = 2*mag + 2^p (DecodeCodeBlock,
                        // irreversible path); the reconstructed coefficient is
                        // datap * 0.5 * delta (Annex E.1 mid-point).
                        const std::int32_t q = planes[c][k];
                        if (q == 0) continue;
                        const double half = static_cast<double>(q < 0 ? -q : q) * 0.5;
                        fplanes[c][k] = (q < 0 ? -half : half) * delta;
                    }
                }
            };
            int sbw, sbh;
            detail::SubbandDims(w, h, nl, /*xob=*/0, /*yob=*/0, sbw, sbh);  // resolution-0 LL
            put(0, 0, sbw, sbh, subband_delta(0, 0));
            for (int r = 1; r <= nl; ++r) {
                const int nb = nl - r + 1;
                const int low_w = static_cast<int>(detail::CeilDiv(w, 1 << (nl - r + 1)));
                const int low_h = static_cast<int>(detail::CeilDiv(h, 1 << (nl - r + 1)));
                int hl_w, hl_h, lh_w, lh_h, hh_w, hh_h;
                detail::SubbandDims(w, h, nb, /*xob=*/1, /*yob=*/0, hl_w, hl_h);
                detail::SubbandDims(w, h, nb, /*xob=*/0, /*yob=*/1, lh_w, lh_h);
                detail::SubbandDims(w, h, nb, /*xob=*/1, /*yob=*/1, hh_w, hh_h);
                put(low_w, 0, hl_w, hl_h, subband_delta(r, 1));
                put(0, low_h, lh_w, lh_h, subband_delta(r, 2));
                put(low_w, low_h, hh_w, hh_h, subband_delta(r, 3));
            }
            detail::Idwt2dMallat97(fplanes[c], static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), nl);
        }

        for (std::size_t i = 0; i < npix; ++i) {
            double c0, c1, c2;  // components 0..2 after the optional inverse MCT
            if (image.mct && ncomp >= 3) {
                // Inverse irreversible colour transform (ICT) on components
                // 0..2 — ISO/IEC 15444-1 Annex G.1, Eq (G-6). For a 3-component
                // image these are (Y, Cb, Cr); for 4-component CMYK the MCT was
                // applied to the first three channels only (component 3 = K is
                // untouched, matching the encoder).
                const double y_val = fplanes[0][i];
                const double cb = fplanes[1][i];
                const double cr = fplanes[2][i];
                c0 = y_val + 1.402 * cr;
                c1 = y_val - 0.344136 * cb - 0.714136 * cr;
                c2 = y_val + 1.772 * cb;
            } else {
                c0 = fplanes[0][i];
                c1 = (ncomp >= 3) ? fplanes[1][i] : fplanes[0][i];
                c2 = (ncomp >= 3) ? fplanes[2][i] : fplanes[0][i];
            }
            if (cmyk) {
                // DC level-shift the four components to 0..255 C/M/Y/K, then
                // CMYK -> RGB exactly as OpenJPEG color_cmyk_to_rgb: sX = 1/255;
                // invert; R = 255*C*K, G = 255*M*K, B = 255*Y*K (truncated).
                // The result is already final RGB in 0..255, so the shared
                // level-shift loop below skips it (guarded by `cmyk`).
                // lvl() yields a level-shifted sample in [0, max_val]; the
                // CMYK->RGB ratios use it normalised to [0,1] (max_val, not a
                // hardcoded 255, so >8-bit components normalise correctly).
                const double scale = static_cast<double>(max_val);
                const auto lvl = [&](double v) {
                    return std::clamp(static_cast<std::int32_t>(std::lround(v)) + shift, 0, max_val);
                };
                const double cc = 1.0 - lvl(c0) / scale;
                const double mm = 1.0 - lvl(c1) / scale;
                const double yy = 1.0 - lvl(c2) / scale;
                const double kk = 1.0 - lvl(fplanes[3][i]) / scale;
                r_plane[i] = static_cast<std::int32_t>(255.0 * cc * kk);
                g_plane[i] = static_cast<std::int32_t>(255.0 * mm * kk);
                b_plane[i] = static_cast<std::int32_t>(255.0 * yy * kk);
            } else {
                r_plane[i] = static_cast<std::int32_t>(std::lround(c0));
                g_plane[i] = static_cast<std::int32_t>(std::lround(c1));
                b_plane[i] = static_cast<std::int32_t>(std::lround(c2));
            }
        }
    }

    // Composite this tile's pixels into the canvas at its offset (Annex B.3).
    // DC level-shift + clamp — ISO/IEC 15444-1 Annex G.1.2 / G.2 (applied after
    // the inverse colour transform). The CMYK path already produced final RGB
    // (level-shift folded into the CMYK->RGB conversion), so it only clamps.
    // Non-CMYK samples live in [0, max_val] (max_val = 2^bit_depth-1); scale
    // them down to 8-bit RGBA for output. CMYK already produced 0..255 RGB, so
    // it is packed as-is (field 33342_*_16bit_RGB / _CMYK).
    const auto to8 = [&](std::int32_t v) -> std::uint8_t {
        if (cmyk || max_val == 255) return static_cast<std::uint8_t>(v);
        return static_cast<std::uint8_t>(
            (static_cast<std::int64_t>(v) * 255 + max_val / 2) / max_val);
    };
    for (std::size_t i = 0; i < npix; ++i) {
        const std::int32_t dc = cmyk ? 0 : shift;
        const std::int32_t r_val = std::clamp(r_plane[i] + dc, 0, max_val);
        const std::int32_t g_val = std::clamp(g_plane[i] + dc, 0, max_val);
        const std::int32_t b_val = std::clamp(b_plane[i] + dc, 0, max_val);

        const std::size_t tx = i % static_cast<std::size_t>(w);
        const std::size_t ty = i / static_cast<std::size_t>(w);
        const std::size_t cidx =
            ((static_cast<std::size_t>(current_tile.y0) + ty) * canvas_w +
             (static_cast<std::size_t>(current_tile.x0) + tx)) * 4;
        out.pixels[cidx + 0] = static_cast<std::byte>(to8(r_val));
        out.pixels[cidx + 1] = static_cast<std::byte>(to8(g_val));
        out.pixels[cidx + 2] = static_cast<std::byte>(to8(b_val));
        out.pixels[cidx + 3] = static_cast<std::byte>(0xFF);
    }
    }  // for each tile
    return out;
}

}
