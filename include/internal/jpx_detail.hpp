#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
namespace foundation::jpx::detail {

// A single adaptive arithmetic-coding context: probability-state index
// (row in the Qe table, ISO/IEC 15444-1 Table C.2) plus the current
// most-probable-symbol sense.
struct MqContext { std::uint8_t index = 0; std::uint8_t mps = 0; };

// MQ arithmetic decoder — ISO/IEC 15444-1 Annex C. Operates over a fixed
// byte buffer; reads past the end synthesise 0xFF bytes (Annex C.3.4
// BYTEIN, Figure C.18, when no more compressed data is available).
class MqDecoder {
public:
    MqDecoder(const std::uint8_t* data, std::size_t len);

    // Decode one binary decision using adaptive context cx (updated in place).
    int Decode(MqContext& cx);

private:
    std::uint8_t ByteAt(std::size_t i) const { return i < len_ ? data_[i] : 0xFF; }
    void ByteIn();
    void Renorm();

    const std::uint8_t* data_;
    std::size_t len_, bp_;
    std::uint32_t c_, a_;
    int ct_;
};

// Reversible 5/3 wavelet, one decomposition level, in place — ISO/IEC
// 15444-1 Annex F.3.8 (forward, Table F.4) and F.3.7 (inverse, Table F.4).
// Symmetric boundary extension per F.3.3 (s[-1] = s[1], s[N] = s[N-2]).
void Fdwt53_1d(std::vector<std::int32_t>& s);
void Idwt53_1d(std::vector<std::int32_t>& s);

// Irreversible 9/7 wavelet, one decomposition level, in place — ISO/IEC
// 15444-1 Annex F.3.8 (forward) and F.3.7 (inverse), lifting coefficients
// and scaling factor from Table F.4. Symmetric boundary extension per F.3.3.
void Fdwt97_1d(std::vector<double>& s);
void Idwt97_1d(std::vector<double>& s);

// Two-dimensional inverse (synthesis) transform over a row-major w*h plane,
// `levels` resolution levels — ISO/IEC 15444-1 Annex F.3.1 (separable
// row/column application) combined with the Mallat multiresolution
// decomposition order of Annex F.4 (smallest LL band synthesised first).
//
// Idwt2d_53/Idwt2d_97 operate on the checkerboard (even/odd index)
// interleaving that the 1-D lifting steps consume directly. Idwt2dMallat53
// instead operates on a plane laid out in the standard Mallat quadrant
// arrangement (each level's LL band a contiguous block at the top-left of
// the next-larger active region, HL/LH/HH filling the remaining quadrants —
// Annex F.4), converting each level's active region to the checkerboard form
// before synthesising it. This is the layout tier-2 packet decoding produces
// directly from the per-subband code-block placement (Annex B.10).
void Idwt2d_53(std::vector<std::int32_t>& plane, std::uint32_t w, std::uint32_t h, int levels);
void Idwt2d_97(std::vector<double>& plane, std::uint32_t w, std::uint32_t h, int levels);
void Idwt2dMallat53(std::vector<std::int32_t>& plane, std::uint32_t w, std::uint32_t h, int levels);
void Idwt2dMallat97(std::vector<double>& plane, std::uint32_t w, std::uint32_t h, int levels);

// Per-component bit depth and signedness — ISO/IEC 15444-1 Annex A.5.1
// (SIZ marker, Ssiz field: low 7 bits = bit depth - 1, high bit = signed).
struct Component {
    int bit_depth;
    bool is_signed;
};

// Quantization step size for one subband — ISO/IEC 15444-1 Annex A.6.4 (QCD/
// QCC marker, SPqcd/SPqcc entries: exponent in the high bits, mantissa in the
// low 11 bits for the derived/expounded styles).
struct SubbandQuant {
    int exponent;
    int mantissa;
};

// Wavelet transform selection — ISO/IEC 15444-1 Annex A.6.1 (COD marker,
// SPcod transformation field: 1 = reversible 5/3, 0 = irreversible 9/7).
enum class Wavelet { Reversible53, Irreversible97 };

// One tile of the image — ISO/IEC 15444-1 Annex B.3. `data` is the tile's
// complete entropy-coded bitstream: the post-SOD body of every tile-part with
// this tile index, concatenated in tile-part order. The bounds are in image
// coordinates relative to the image origin (canvas coordinates), clipped to
// the image area.
struct Tile {
    std::uint32_t x0, y0, x1, y1;     // canvas-relative bounds [x0,x1) x [y0,y1)
    std::vector<std::byte> data;      // concatenated post-SOD bitstream of all this tile's tile-parts
};

// Parsed main-header + tiles, sufficient to drive the tier-1/tier-2 decode
// pipeline — ISO/IEC 15444-1 Annex A (codestream syntax).
struct Image {
    std::uint32_t width;   // SIZ: Xsiz - XOsiz (Annex A.5.1)
    std::uint32_t height;  // SIZ: Ysiz - YOsiz (Annex A.5.1)
    std::uint32_t x0;      // SIZ: XOsiz, image origin (Annex A.5.1)
    std::uint32_t y0;      // SIZ: YOsiz, image origin (Annex A.5.1)

    std::vector<Component> components;  // SIZ: one entry per Csiz (Annex A.5.1)

    Wavelet wavelet;  // COD: SPcod transformation (Annex A.6.1)
    int levels;       // COD: SPcod number of decomposition levels (Annex A.6.1)
    int cb_w_exp;      // COD: SPcod code-block width exponent xcb (actual = xcb+2)
    int cb_h_exp;      // COD: SPcod code-block height exponent ycb (actual = ycb+2)
    bool mct;          // COD: SGcod multiple-component transform flag (Annex A.6.1)
    int layers;        // COD: SGcod number of quality layers (Annex A.6.1)
    int progression;   // COD: SGcod progression order — 0 = LRCP, 1 = RLCP (Annex A.6.1)

    int quant_style;   // QCD: Sqcd low 5 bits — 0 none, 1 derived, 2 expounded (Annex A.6.4)
    int guard_bits;    // QCD: Sqcd high 3 bits (Annex A.6.4)
    std::vector<SubbandQuant> quant;  // QCD: SPqcd entries, per subband or single (Annex A.6.4)

    std::uint32_t tiles_x;  // number of tiles across (Annex B.3)
    std::uint32_t tiles_y;  // number of tiles down

    // One entry per tile, in tile-index (Isot) order: row-major over the tile
    // grid. Each carries its canvas bounds and concatenated bitstream.
    std::vector<Tile> tiles;
};

// Parse a JPEG 2000 codestream main header plus its single tile-part —
// ISO/IEC 15444-1 Annex A. `cs` must begin with the SOC marker (0xFF4F).
// Throws std::runtime_error on truncation, malformed/unknown required
// markers, or any v1-unsupported feature (multi-tile, >3 components,
// bit depth != 8, sub-sampling, non-LRCP progression, multiple quality
// layers, non-default code-block style, ROI).
Image ParseCodestream(std::span<const std::byte> cs);

// Locate the contiguous-codestream box (`jp2c`) inside a JP2 file — ISO/IEC
// 15444-1 Annex I (box structure). `jp2` must begin with the 12-byte JP2
// signature box. Returns the `jp2c` box content (which itself begins with
// the SOC marker, suitable for ParseCodestream). If `out_enum_cs` is non-null
// and an enumerated colour space (`colr` METH=1) is present, its EnumCS value
// is written there (Table I.10 / 15444-2: 12 = CMYK, 16 = sRGB, 17 = greyscale,
// 18 = sYCC); left unchanged (caller initialises to -1) when absent. Throws
// std::runtime_error if the signature box is missing/invalid, no `jp2c` box is
// found, or a palettized (`pclr`/`cmap`) or restricted-ICC (`colr` METH=2) box
// is present (v1-unsupported, Annex I.5.3/I.5.4).
std::span<const std::byte> Jp2FindCodestream(std::span<const std::byte> jp2,
                                             int* out_enum_cs = nullptr);

// Test-only: fully decode a `w`x`h`-leaf hierarchical tag tree (ISO/IEC
// 15444-1 Annex B.10.2) from a packed, MSB-first, bit-stuffed bitstream;
// returns each leaf's resolved value in row-major order. Exposed so the unit
// tests can exercise the tag tree directly.
std::vector<int> TagTreeDecodeAll(std::span<const std::uint8_t> bits, int w, int h);

}
