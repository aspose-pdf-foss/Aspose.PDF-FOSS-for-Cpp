// foundation/ccitt — CCITTFaxDecode decoder.
//
// Implements decode-only support for ITU-T T.4 (Group 3) and T.6
// (Group 4) bilevel image compression as used by PDF's
// /CCITTFaxDecode filter.
//
// Reference:
//   ITU-T Recommendation T.4 (07/2003) — Group 3 facsimile.
//   ITU-T Recommendation T.6 (11/1988) — Group 4 facsimile.
//   PDF 32000-1:2008 §7.4.7 (CCITTFaxDecode filter, Table 11).
//
// Design notes:
//
// Run-length tables. T.4 §3.5.1 Table 2 (white runs) + Table 3
// (black runs) are encoded verbatim as kWhiteTerm / kWhiteMakeup /
// kBlackTerm / kBlackMakeup. The "common" extended make-up codes
// for runs 1792..2560 (T.4 §3.5.1 Table 4) are kCommonMakeup.
//
// Bit reader. MSB-first within each byte: bit 7 of the first input
// byte is the first bit of the stream. ``Peek(n)`` accumulates
// bytes as needed and returns the top n bits right-aligned.
//
// 2D row decoding. T.6 (and the 2D rows of mixed Group 3) maintain
// a "reference line" — the previously decoded row — and emit a
// sequence of mode codes that refer to colour-change positions on
// it. The reference for the very first row is an imaginary
// all-white line (no transitions). After each row is decoded, the
// list of colour-change positions becomes the next row's
// reference.
//
// Output polarity. The decoder produces a T.6-natural raster where
// 0 represents white and 1 represents black. With Params.BlackIs1
// set, the raster is inverted before output so 1 represents white
// and 0 represents black — the canonical reads this directly.

#include "ccitt.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::ccitt {
namespace {

// ---------------------------------------------------------------------------
// Bit reader (MSB-first per byte).
// ---------------------------------------------------------------------------

class BitReader {
public:
    explicit BitReader(std::span<const std::byte> d) : data_(d) {}

    // Peek up to 24 bits (enough for any CCITT code).
    std::uint32_t Peek(int n) {
        Refill(n);
        if (n <= 0) return 0;
        if (n > buffer_bits_) {
            // Not enough bits left — pad with zeros at the LSB end so
            // the bit pattern still aligns at peek width.
            std::uint32_t v = buffer_;
            int shortfall = n - buffer_bits_;
            return (v << shortfall) & ((1u << n) - 1u);
        }
        return (buffer_ >> (buffer_bits_ - n)) & ((1u << n) - 1u);
    }

    void Consume(int n) {
        if (n <= 0) return;
        if (n >= buffer_bits_) {
            // Consumed everything we have plus possibly more.
            int over = n - buffer_bits_;
            buffer_ = 0;
            buffer_bits_ = 0;
            (void)over;  // can't pull from past end; caller will throw if needed
            return;
        }
        buffer_bits_ -= n;
        buffer_ &= (1u << buffer_bits_) - 1u;
    }

    // Drop the partial bits at the start of the buffer to align to the
    // next byte boundary in the input stream.
    void AlignToByte() {
        int extra = buffer_bits_ % 8;
        if (extra) Consume(extra);
    }

    bool Empty() const {
        return pos_ >= data_.size() && buffer_bits_ == 0;
    }

    int BitsAvailable() const {
        return buffer_bits_ +
               static_cast<int>((data_.size() - pos_) * 8);
    }

private:
    void Refill(int need) {
        while (buffer_bits_ < need && pos_ < data_.size()) {
            buffer_ = (buffer_ << 8) |
                      static_cast<std::uint8_t>(data_[pos_]);
            ++pos_;
            buffer_bits_ += 8;
        }
    }

    std::span<const std::byte> data_;
    std::size_t pos_ = 0;
    std::uint32_t buffer_ = 0;
    int buffer_bits_ = 0;
};

// ---------------------------------------------------------------------------
// Huffman code tables. Bit patterns are right-aligned in the
// ``bits`` field; ``len`` carries the actual code-length.
// ---------------------------------------------------------------------------

struct CodePattern {
    std::uint16_t bits;
    std::uint8_t  len;
    std::int16_t  value;
};

// White run-length terminating codes (T.4 Table 2). Run lengths 0..63.
constexpr std::array<CodePattern, 64> kWhiteTerm = {{
    {0b00110101, 8, 0},     {0b000111, 6, 1},       {0b0111, 4, 2},
    {0b1000, 4, 3},         {0b1011, 4, 4},         {0b1100, 4, 5},
    {0b1110, 4, 6},         {0b1111, 4, 7},         {0b10011, 5, 8},
    {0b10100, 5, 9},        {0b00111, 5, 10},       {0b01000, 5, 11},
    {0b001000, 6, 12},      {0b000011, 6, 13},      {0b110100, 6, 14},
    {0b110101, 6, 15},      {0b101010, 6, 16},      {0b101011, 6, 17},
    {0b0100111, 7, 18},     {0b0001100, 7, 19},     {0b0001000, 7, 20},
    {0b0010111, 7, 21},     {0b0000011, 7, 22},     {0b0000100, 7, 23},
    {0b0101000, 7, 24},     {0b0101011, 7, 25},     {0b0010011, 7, 26},
    {0b0100100, 7, 27},     {0b0011000, 7, 28},     {0b00000010, 8, 29},
    {0b00000011, 8, 30},    {0b00011010, 8, 31},    {0b00011011, 8, 32},
    {0b00010010, 8, 33},    {0b00010011, 8, 34},    {0b00010100, 8, 35},
    {0b00010101, 8, 36},    {0b00010110, 8, 37},    {0b00010111, 8, 38},
    {0b00101000, 8, 39},    {0b00101001, 8, 40},    {0b00101010, 8, 41},
    {0b00101011, 8, 42},    {0b00101100, 8, 43},    {0b00101101, 8, 44},
    {0b00000100, 8, 45},    {0b00000101, 8, 46},    {0b00001010, 8, 47},
    {0b00001011, 8, 48},    {0b01010010, 8, 49},    {0b01010011, 8, 50},
    {0b01010100, 8, 51},    {0b01010101, 8, 52},    {0b00100100, 8, 53},
    {0b00100101, 8, 54},    {0b01011000, 8, 55},    {0b01011001, 8, 56},
    {0b01011010, 8, 57},    {0b01011011, 8, 58},    {0b01001010, 8, 59},
    {0b01001011, 8, 60},    {0b00110010, 8, 61},    {0b00110011, 8, 62},
    {0b00110100, 8, 63},
}};

// White run-length make-up codes (T.4 Table 2). Runs 64..1728 in 64s.
constexpr std::array<CodePattern, 27> kWhiteMakeup = {{
    {0b11011, 5, 64},        {0b10010, 5, 128},       {0b010111, 6, 192},
    {0b0110111, 7, 256},     {0b00110110, 8, 320},    {0b00110111, 8, 384},
    {0b01100100, 8, 448},    {0b01100101, 8, 512},    {0b01101000, 8, 576},
    {0b01100111, 8, 640},    {0b011001100, 9, 704},   {0b011001101, 9, 768},
    {0b011010010, 9, 832},   {0b011010011, 9, 896},   {0b011010100, 9, 960},
    {0b011010101, 9, 1024},  {0b011010110, 9, 1088},  {0b011010111, 9, 1152},
    {0b011011000, 9, 1216},  {0b011011001, 9, 1280},  {0b011011010, 9, 1344},
    {0b011011011, 9, 1408},  {0b010011000, 9, 1472},  {0b010011001, 9, 1536},
    {0b010011010, 9, 1600},  {0b011000, 6, 1664},     {0b010011011, 9, 1728},
}};

// Black run-length terminating codes (T.4 Table 3). Run lengths 0..63.
constexpr std::array<CodePattern, 64> kBlackTerm = {{
    {0b0000110111, 10, 0},   {0b010, 3, 1},           {0b11, 2, 2},
    {0b10, 2, 3},            {0b011, 3, 4},           {0b0011, 4, 5},
    {0b0010, 4, 6},          {0b00011, 5, 7},         {0b000101, 6, 8},
    {0b000100, 6, 9},        {0b0000100, 7, 10},      {0b0000101, 7, 11},
    {0b0000111, 7, 12},      {0b00000100, 8, 13},     {0b00000111, 8, 14},
    {0b000011000, 9, 15},    {0b0000010111, 10, 16},  {0b0000011000, 10, 17},
    {0b0000001000, 10, 18},  {0b00001100111, 11, 19}, {0b00001101000, 11, 20},
    {0b00001101100, 11, 21}, {0b00000110111, 11, 22}, {0b00000101000, 11, 23},
    {0b00000010111, 11, 24}, {0b00000011000, 11, 25}, {0b000011001010, 12, 26},
    {0b000011001011, 12, 27}, {0b000011001100, 12, 28}, {0b000011001101, 12, 29},
    {0b000001101000, 12, 30}, {0b000001101001, 12, 31}, {0b000001101010, 12, 32},
    {0b000001101011, 12, 33}, {0b000011010010, 12, 34}, {0b000011010011, 12, 35},
    {0b000011010100, 12, 36}, {0b000011010101, 12, 37}, {0b000011010110, 12, 38},
    {0b000011010111, 12, 39}, {0b000001101100, 12, 40}, {0b000001101101, 12, 41},
    {0b000011011010, 12, 42}, {0b000011011011, 12, 43}, {0b000001010100, 12, 44},
    {0b000001010101, 12, 45}, {0b000001010110, 12, 46}, {0b000001010111, 12, 47},
    {0b000001100100, 12, 48}, {0b000001100101, 12, 49}, {0b000001010010, 12, 50},
    {0b000001010011, 12, 51}, {0b000000100100, 12, 52}, {0b000000110111, 12, 53},
    {0b000000111000, 12, 54}, {0b000000100111, 12, 55}, {0b000000101000, 12, 56},
    {0b000001011000, 12, 57}, {0b000001011001, 12, 58}, {0b000000101011, 12, 59},
    {0b000000101100, 12, 60}, {0b000001011010, 12, 61}, {0b000001100110, 12, 62},
    {0b000001100111, 12, 63},
}};

// Black run-length make-up codes (T.4 Table 3). Runs 64..1728 in 64s.
constexpr std::array<CodePattern, 27> kBlackMakeup = {{
    {0b0000001111, 10, 64},      {0b000011001000, 12, 128},
    {0b000011001001, 12, 192},   {0b000001011011, 12, 256},
    {0b000000110011, 12, 320},   {0b000000110100, 12, 384},
    {0b000000110101, 12, 448},   {0b0000001101100, 13, 512},
    {0b0000001101101, 13, 576},  {0b0000001001010, 13, 640},
    {0b0000001001011, 13, 704},  {0b0000001001100, 13, 768},
    {0b0000001001101, 13, 832},  {0b0000001110010, 13, 896},
    {0b0000001110011, 13, 960},  {0b0000001110100, 13, 1024},
    {0b0000001110101, 13, 1088}, {0b0000001110110, 13, 1152},
    {0b0000001110111, 13, 1216}, {0b0000001010010, 13, 1280},
    {0b0000001010011, 13, 1344}, {0b0000001010100, 13, 1408},
    {0b0000001010101, 13, 1472}, {0b0000001011010, 13, 1536},
    {0b0000001011011, 13, 1600}, {0b0000001100100, 13, 1664},
    {0b0000001100101, 13, 1728},
}};

// Common make-up codes for runs 1792..2560 (T.4 Table 4). Same
// codes for white and black; chained after a colour-specific
// make-up to span runs longer than 1728 + 63.
constexpr std::array<CodePattern, 13> kCommonMakeup = {{
    {0b00000001000,  11, 1792},  {0b00000001100,  11, 1856},
    {0b00000001101,  11, 1920},  {0b000000010010, 12, 1984},
    {0b000000010011, 12, 2048},  {0b000000010100, 12, 2112},
    {0b000000010101, 12, 2176},  {0b000000010110, 12, 2240},
    {0b000000010111, 12, 2304},  {0b000000011100, 12, 2368},
    {0b000000011101, 12, 2432},  {0b000000011110, 12, 2496},
    {0b000000011111, 12, 2560},
}};

// 2D mode codes (T.4 §4.2 + T.6, identical sets).
enum class Mode : std::uint8_t {
    Pass, Horizontal, V0, VR1, VR2, VR3, VL1, VL2, VL3, Extension2D,
};

struct ModePattern {
    std::uint16_t bits;
    std::uint8_t  len;
    Mode mode;
};

// Sorted by length ascending so a shorter prefix code (e.g. V0 = 1
// bit) wins over any longer code that happens to share its top
// bits — this is required for prefix-free Huffman matching.
constexpr std::array<ModePattern, 10> kModeCodes = {{
    {0b1, 1, Mode::V0},
    {0b011, 3, Mode::VR1},
    {0b010, 3, Mode::VL1},
    {0b001, 3, Mode::Horizontal},
    {0b0001, 4, Mode::Pass},
    {0b000011, 6, Mode::VR2},
    {0b000010, 6, Mode::VL2},
    {0b0000011, 7, Mode::VR3},
    {0b0000010, 7, Mode::VL3},
    {0b0000001, 7, Mode::Extension2D},
}};

// Maximum bit-length we ever peek for run-length codes — black
// make-up tops out at 13 bits and common make-up at 12 bits, so 13
// covers everything.
constexpr int kMaxRunCodeBits = 13;
constexpr int kMaxModeCodeBits = 7;

// EOL / EOFB markers (T.4/T.6). The 12-bit pattern 0000 0000 0001
// terminates fill bits; EOFB is two consecutive EOLs.
constexpr std::uint16_t kEOL = 0x001;

// ---------------------------------------------------------------------------
// Code-table lookup helpers.
// ---------------------------------------------------------------------------

// Match the next code from the bit reader against ``table``. Tries
// every entry; first match wins (entries sorted by length ascending
// so shorter codes preempt longer ones that share their prefix).
// Returns the entry's ``value`` on match or -1 on no match.
template <std::size_t N>
int MatchCode(BitReader& r,
              const std::array<CodePattern, N>& table,
              int peek_max) {
    std::uint32_t bits = r.Peek(peek_max);
    for (const auto& c : table) {
        std::uint32_t prefix = (bits >> (peek_max - c.len)) &
                               ((1u << c.len) - 1u);
        if (prefix == c.bits) {
            r.Consume(c.len);
            return c.value;
        }
    }
    return -1;
}

// Decode one full run of the given colour. Chains make-up codes
// (per-colour and common) until a terminating code is matched; the
// terminator's run length plus all make-ups gives the total. Throws
// on malformed input.
int DecodeRunLength(BitReader& r, bool is_white) {
    int total = 0;
    while (true) {
        int term = is_white
            ? MatchCode(r, kWhiteTerm, kMaxRunCodeBits)
            : MatchCode(r, kBlackTerm, kMaxRunCodeBits);
        if (term >= 0) {
            return total + term;
        }
        int mk = is_white
            ? MatchCode(r, kWhiteMakeup, kMaxRunCodeBits)
            : MatchCode(r, kBlackMakeup, kMaxRunCodeBits);
        if (mk > 0) {
            total += mk;
            continue;
        }
        int cm = MatchCode(r, kCommonMakeup, kMaxRunCodeBits);
        if (cm > 0) {
            total += cm;
            continue;
        }
        throw std::runtime_error(
            std::string("CCITT: invalid ") +
            (is_white ? "white" : "black") +
            " run-length code");
    }
}

// Match the next 2D mode code; throws on no match.
Mode DecodeMode(BitReader& r) {
    std::uint32_t bits = r.Peek(kMaxModeCodeBits);
    for (const auto& c : kModeCodes) {
        std::uint32_t prefix =
            (bits >> (kMaxModeCodeBits - c.len)) &
            ((1u << c.len) - 1u);
        if (prefix == c.bits) {
            r.Consume(c.len);
            return c.mode;
        }
    }
    throw std::runtime_error("CCITT: invalid 2D mode code");
}

// Skip leading zero "fill" bits until the EOL terminating 1 bit is
// consumed (T.4 §3.3). Used between rows when EndOfLine=true.
// Returns false if the EOL was not found before the stream ended.
bool ConsumeEOL(BitReader& r) {
    // EOL pattern is at least 11 zero bits followed by 1.
    int zeros = 0;
    while (r.BitsAvailable() > 0) {
        std::uint32_t bit = r.Peek(1);
        r.Consume(1);
        if (bit == 0) {
            ++zeros;
        } else {
            return zeros >= 11;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// 2D row decoding helpers.
// ---------------------------------------------------------------------------

// Find b1: first reference colour-change x_i strictly greater than
// a0 such that the colour AT x_i (i.e. the colour of pixels in
// [x_i, x_{i+1}) on the reference line) is opposite to ``a0_color``.
//
// Reference rows always start "implicitly white" (per T.6); the
// colour at the i-th change therefore alternates, starting BLACK at
// ``ref_changes[0]``: i even → black, i odd → white.
//
// "Color of b1 must be opposite to a0_color" therefore translates to
//   * a0_color == 0 (white): pick first i with x_i > a0 AND i even
//     (so colour at x_i = black, opposite to white);
//   * a0_color == 1 (black): pick first i with x_i > a0 AND i odd.
//
// If no ref-change qualifies, b1 is the implicit right-edge sentinel
// (= columns).
int FindB1(const std::vector<int>& ref_changes,
           int a0, int a0_color, int columns) {
    int target_parity = a0_color;  // 0 or 1
    int n = static_cast<int>(ref_changes.size());
    for (int i = 0; i < n; ++i) {
        if (ref_changes[i] <= a0) continue;
        if ((i & 1) == target_parity) return ref_changes[i];
    }
    return columns;
}

// b2 is the next ref-change strictly to the right of b1 (regardless
// of colour). Sentinel = columns.
int FindB2(const std::vector<int>& ref_changes,
           int b1, int columns) {
    int n = static_cast<int>(ref_changes.size());
    for (int i = 0; i < n; ++i) {
        if (ref_changes[i] > b1) return ref_changes[i];
    }
    return columns;
}

// Emit pixels of colour ``color`` (0 = white, 1 = black) into the
// row buffer at columns [from, to). Clamps to the row bounds; an
// empty interval is a no-op.
void EmitRange(std::vector<std::uint8_t>& row, int from, int to,
               int columns, std::uint8_t color) {
    int lo = from < 0 ? 0 : from;
    int hi = to > columns ? columns : to;
    for (int x = lo; x < hi; ++x) row[x] = color;
}

// Decode one 2D row. ``ref_changes`` is the previous row's
// colour-change positions (sorted, no sentinel). On return,
// ``cur_changes`` holds this row's colour-change positions and
// ``row`` is the filled bilevel raster (1 = black, 0 = white in
// T.6-natural convention — applied on output by the caller's
// BlackIs1 step).
void DecodeRow2D(BitReader& r, int columns,
                 const std::vector<int>& ref_changes,
                 std::vector<int>& cur_changes,
                 std::vector<std::uint8_t>& row) {
    row.assign(columns, 0);  // implicit white background
    cur_changes.clear();

    // a0 is conceptually the imaginary changing pixel just before
    // column 0; -1 lets the b1 lookup ("ref_changes[i] > a0")
    // include column 0. a0_color = white = colour of pixels left of
    // a0 (the imaginary white background).
    int a0 = -1;
    int a0_color = 0;  // 0 = white, 1 = black

    while (a0 < columns) {
        int b1 = FindB1(ref_changes, a0, a0_color, columns);
        int b2 = FindB2(ref_changes, b1, columns);

        Mode m = DecodeMode(r);
        switch (m) {
        case Mode::Pass: {
            // Pass: pixels in [a0, b2) keep the SAME colour as the
            // segment currently being scanned. By the T.6 convention
            // a0_color names the colour of pixels currently being
            // emitted (the segment STARTING at a0), so emit a0_color.
            EmitRange(row, a0, b2, columns,
                      static_cast<std::uint8_t>(a0_color));
            a0 = b2;
            // a0_color unchanged.
            break;
        }
        case Mode::V0:
        case Mode::VR1: case Mode::VR2: case Mode::VR3:
        case Mode::VL1: case Mode::VL2: case Mode::VL3: {
            int offset = 0;
            switch (m) {
            case Mode::V0:  offset =  0; break;
            case Mode::VR1: offset = +1; break;
            case Mode::VR2: offset = +2; break;
            case Mode::VR3: offset = +3; break;
            case Mode::VL1: offset = -1; break;
            case Mode::VL2: offset = -2; break;
            case Mode::VL3: offset = -3; break;
            default: break;
            }
            int new_a0 = b1 + offset;
            if (new_a0 < 0 || new_a0 > columns) {
                throw std::runtime_error(
                    "CCITT: V-mode position out of bounds");
            }
            // Pixels [a0, new_a0) of a0_color, then a0_color toggles
            // (a transition at new_a0).
            EmitRange(row, a0, new_a0, columns,
                      static_cast<std::uint8_t>(a0_color));
            if (new_a0 < columns) cur_changes.push_back(new_a0);
            a0 = new_a0;
            a0_color = 1 - a0_color;
            break;
        }
        case Mode::Horizontal: {
            // First run is the SAME colour as a0 (= a0_color in our
            // convention "a0_color is the colour of the segment
            // starting at a0"). Second run is the opposite colour.
            // a0_color toggles twice through H, so it's unchanged
            // afterwards.
            std::uint8_t first_color =
                static_cast<std::uint8_t>(a0_color);
            int run1 = DecodeRunLength(r, first_color == 0);
            int run2 = DecodeRunLength(r, first_color != 0);
            int from_x = (a0 < 0 ? 0 : a0);
            int after_first = from_x + run1;
            int after_second = after_first + run2;
            if (after_second > columns) {
                throw std::runtime_error(
                    "CCITT: H-mode run lengths exceed Columns");
            }
            EmitRange(row, a0, after_first, columns, first_color);
            EmitRange(row, after_first, after_second, columns,
                      static_cast<std::uint8_t>(1 - first_color));
            a0 = after_second;
            // a0_color unchanged: toggled twice.
            break;
        }
        case Mode::Extension2D:
            throw std::runtime_error(
                "CCITT: 2D extension code not supported");
        }
    }

    if (a0 != columns) {
        throw std::runtime_error(
            "CCITT: 2D row cursor != Columns at end");
    }

    // Re-derive cur_changes from the row buffer to handle the H-mode
    // edge cases cleanly (the per-mode cur_changes pushes above are
    // best-effort; reading the actual filled row gives the canonical
    // change list without per-case fiddling).
    cur_changes.clear();
    int prev_color = 0;
    for (int x = 0; x < columns; ++x) {
        if (row[x] != prev_color) {
            cur_changes.push_back(x);
            prev_color = row[x];
        }
    }
}

// Decode one 1D row (Group 3, K = 0). Rows always start white;
// runs alternate white→black→white until ``columns`` pixels have
// been emitted.
void DecodeRow1D(BitReader& r, int columns,
                 std::vector<std::uint8_t>& row) {
    row.assign(columns, 0);
    int x = 0;
    bool is_white = true;
    while (x < columns) {
        int run = DecodeRunLength(r, is_white);
        if (run < 0 || x + run > columns) {
            throw std::runtime_error(
                "CCITT: 1D run-length exceeds Columns");
        }
        std::uint8_t color = is_white ? 0 : 1;
        EmitRange(row, x, x + run, columns, color);
        x += run;
        is_white = !is_white;
    }
}

// ---------------------------------------------------------------------------
// Output bit packing.
// ---------------------------------------------------------------------------

// Pack the decoded bilevel scanlines into the output byte buffer.
// MSB-first per byte; scanlines padded to whole bytes
// ((columns + 7) / 8 bytes/row).
//
// Internal raster convention. The 2D / 1D row decoders fill ``rows``
// using the libtiff/PhotometricInterpretation=BlackIsZero convention
// they share with Pillow-encoded fixtures: raster bit 1 = white
// pixel (i.e. the source's pixel value passes through unchanged
// through libtiff's encode + decode), bit 0 = black pixel.
//
// Output convention. PDF's BlackIs1 chooses what the OUTPUT bits
// represent:
//   * BlackIs1=false (PDF default — output uses PDF-normal "1 =
//     white"): emit raster bits as-is.
//   * BlackIs1=true (output uses "1 = black", reverse from PDF
//     normal): invert the raster.
void PackBits(const std::vector<std::vector<std::uint8_t>>& rows,
              int columns, bool black_is_1,
              std::vector<std::byte>& out) {
    int bytes_per_row = (columns + 7) / 8;
    out.assign(static_cast<std::size_t>(bytes_per_row) * rows.size(),
               std::byte{0});
    for (std::size_t r = 0; r < rows.size(); ++r) {
        const auto& row = rows[r];
        for (int c = 0; c < columns; ++c) {
            std::uint8_t v = row[static_cast<std::size_t>(c)];
            std::uint8_t out_bit =
                black_is_1 ? static_cast<std::uint8_t>(1 - v) : v;
            if (out_bit) {
                out[r * bytes_per_row + (c / 8)] |=
                    std::byte{static_cast<std::uint8_t>(
                        1u << (7 - (c % 8)))};
            }
        }
    }
}

// ---------------------------------------------------------------------------
// CFAX framing reader (used by Parse).
// ---------------------------------------------------------------------------

constexpr std::array<std::byte, 4> kCfaxMagic = {
    std::byte{'C'}, std::byte{'F'}, std::byte{'A'}, std::byte{'X'},
};

struct ParsedFixture {
    Params params;
    std::vector<std::byte> body;
};

ParsedFixture ParseCfax(std::span<const std::byte> file) {
    if (file.size() < 16) {
        throw std::runtime_error("CCITT fixture: too short for CFAX header");
    }
    if (!std::equal(file.begin(), file.begin() + 4, kCfaxMagic.begin())) {
        throw std::runtime_error("CCITT fixture: missing CFAX magic");
    }
    if (static_cast<std::uint8_t>(file[4]) != 1u) {
        throw std::runtime_error("CCITT fixture: unsupported version");
    }
    ParsedFixture pf;
    pf.params.K = static_cast<std::int8_t>(
        static_cast<std::uint8_t>(file[5]));
    std::uint8_t flags = static_cast<std::uint8_t>(file[6]);
    pf.params.EndOfLine        = (flags & 0x01) != 0;
    pf.params.EncodedByteAlign = (flags & 0x02) != 0;
    pf.params.EndOfBlock       = (flags & 0x04) != 0;
    pf.params.BlackIs1         = (flags & 0x08) != 0;
    pf.params.DamagedRowsBeforeError =
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[7]));
    pf.params.Columns =
        (static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[8])) << 8) |
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[9]));
    pf.params.Rows =
        (static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[10])) << 24) |
        (static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[11])) << 16) |
        (static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[12])) << 8) |
         static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[13]));
    std::uint32_t body_len =
        (static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[14])) << 8) |
        static_cast<std::uint32_t>(static_cast<std::uint8_t>(file[15]));
    if (file.size() != 16 + body_len) {
        throw std::runtime_error(
            "CCITT fixture: declared body length does not match file size");
    }
    pf.body.assign(file.begin() + 16, file.end());
    return pf;
}

// ---------------------------------------------------------------------------
// Bit writer (MSB-first per byte) + encoder helpers.
// ---------------------------------------------------------------------------

class BitWriter {
public:
    // Append the low ``len`` bits of ``bits`` (most-significant first).
    void Put(std::uint32_t bits, int len) {
        for (int i = len - 1; i >= 0; --i) {
            cur_ = static_cast<std::uint8_t>((cur_ << 1) | ((bits >> i) & 1u));
            if (++nbits_ == 8) {
                out_.push_back(std::byte{cur_});
                cur_ = 0;
                nbits_ = 0;
            }
        }
    }
    void AlignToByte() {
        if (nbits_ != 0) {
            cur_ = static_cast<std::uint8_t>(cur_ << (8 - nbits_));
            out_.push_back(std::byte{cur_});
            cur_ = 0;
            nbits_ = 0;
        }
    }
    std::vector<std::byte> Take() {
        AlignToByte();
        return std::move(out_);
    }

private:
    std::vector<std::byte> out_;
    std::uint8_t cur_ = 0;
    int nbits_ = 0;
};

// Emit one run of ``run`` pixels of the given colour: zero or more make-up
// codes (largest value first) followed by exactly one terminating code.
void EmitRun(BitWriter& w, int run, bool is_white) {
    while (run >= 64) {
        const CodePattern* best = nullptr;
        const auto& mk = is_white ? kWhiteMakeup : kBlackMakeup;
        for (const auto& c : mk)
            if (c.value <= run && (best == nullptr || c.value > best->value))
                best = &c;
        for (const auto& c : kCommonMakeup)
            if (c.value <= run && (best == nullptr || c.value > best->value))
                best = &c;
        // The smallest make-up value is 64, so a match always exists here.
        w.Put(best->bits, best->len);
        run -= best->value;
    }
    const auto& term = is_white ? kWhiteTerm : kBlackTerm;
    w.Put(term[static_cast<std::size_t>(run)].bits,
          term[static_cast<std::size_t>(run)].len);
}

// Colour-change positions of ``row`` (x where the colour differs from x-1;
// the pixel left of column 0 is implicitly white).
std::vector<int> RowChanges(const std::vector<std::uint8_t>& row,
                            int columns) {
    std::vector<int> ch;
    std::uint8_t prev = 0;
    for (int x = 0; x < columns; ++x) {
        if (row[static_cast<std::size_t>(x)] != prev) {
            ch.push_back(x);
            prev = row[static_cast<std::size_t>(x)];
        }
    }
    return ch;
}

void EmitMode(BitWriter& w, Mode m) {
    for (const auto& c : kModeCodes) {
        if (c.mode == m) {
            w.Put(c.bits, c.len);
            return;
        }
    }
}

// Encode one 2D (T.6) row against the reference line's change positions.
// Inverse of DecodeRow2D.
void EncodeRow2D(BitWriter& w, const std::vector<std::uint8_t>& row,
                 const std::vector<int>& ref_changes, int columns) {
    const std::vector<int> cur = RowChanges(row, columns);
    auto first_gt = [](const std::vector<int>& v, int x, int cols) {
        for (int e : v)
            if (e > x) return e;
        return cols;
    };

    int a0 = -1;
    int color = 0;  // colour of the segment starting at a0 (0 white, 1 black)
    while (a0 < columns) {
        const int a1 = first_gt(cur, a0, columns);
        const int b1 = FindB1(ref_changes, a0, color, columns);
        const int b2 = FindB2(ref_changes, b1, columns);

        if (b2 < a1) {
            EmitMode(w, Mode::Pass);
            a0 = b2;  // colour unchanged
        } else {
            const int d = a1 - b1;
            const int ad = d < 0 ? -d : d;
            if (ad <= 3) {
                Mode m = Mode::V0;
                switch (d) {
                    case 0: m = Mode::V0; break;
                    case 1: m = Mode::VR1; break;
                    case 2: m = Mode::VR2; break;
                    case 3: m = Mode::VR3; break;
                    case -1: m = Mode::VL1; break;
                    case -2: m = Mode::VL2; break;
                    case -3: m = Mode::VL3; break;
                    default: break;
                }
                EmitMode(w, m);
                a0 = a1;
                color = 1 - color;
            } else {
                const int a2 = first_gt(cur, a1, columns);
                EmitMode(w, Mode::Horizontal);
                const int from_x = a0 < 0 ? 0 : a0;
                EmitRun(w, a1 - from_x, color == 0);
                EmitRun(w, a2 - a1, color != 0);
                a0 = a2;  // colour unchanged (two transitions)
            }
        }
    }
}

// Encode one 1D (T.4 Group 3) row: alternating white/black runs from the
// left edge. Inverse of DecodeRow1D.
void EncodeRow1D(BitWriter& w, const std::vector<std::uint8_t>& row,
                 int columns) {
    int x = 0;
    bool is_white = true;
    while (x < columns) {
        const std::uint8_t target =
            is_white ? std::uint8_t{0} : std::uint8_t{1};
        int run = 0;
        while (x + run < columns &&
               row[static_cast<std::size_t>(x + run)] == target) {
            ++run;
        }
        EmitRun(w, run, is_white);
        x += run;
        is_white = !is_white;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

DecodedImage Decode(std::span<const std::byte> stream,
                    const Params& params) {
    if (params.Columns == 0) {
        throw std::runtime_error("CCITT: Columns must be > 0");
    }
    if (stream.empty()) {
        throw std::runtime_error("CCITT: empty stream");
    }
    BitReader r(stream);
    std::vector<std::vector<std::uint8_t>> rows;
    std::vector<int> ref_changes;
    std::vector<int> cur_changes;
    std::vector<std::uint8_t> row;

    int columns = static_cast<int>(params.Columns);
    int target_rows = static_cast<int>(params.Rows);

    auto handle_eol_prefix = [&]() {
        if (params.EndOfLine) {
            if (!ConsumeEOL(r)) {
                throw std::runtime_error(
                    "CCITT: missing EOL before row");
            }
        }
        if (params.EncodedByteAlign) {
            r.AlignToByte();
        }
    };

    if (params.K < 0) {
        // Group 4 (T.6) — pure 2D, no EOL between rows.
        int row_index = 0;
        while (true) {
            if (params.EndOfBlock) {
                // Stop on EOFB (two consecutive EOL patterns, 24
                // bits). Peek 24 bits and check.
                std::uint32_t peek = r.Peek(24);
                std::uint32_t eofb =
                    (static_cast<std::uint32_t>(kEOL) << 12) |
                    static_cast<std::uint32_t>(kEOL);
                if (peek == eofb) {
                    r.Consume(24);
                    break;
                }
            }
            if (target_rows > 0 && row_index >= target_rows) break;
            if (r.BitsAvailable() <= 0) break;
            DecodeRow2D(r, columns, ref_changes, cur_changes, row);
            rows.push_back(row);
            ref_changes = cur_changes;
            ++row_index;
        }
    } else if (params.K == 0) {
        // Group 3 1D — rows are pure 1D run-length sequences. EOL
        // codes precede each row only when EndOfLine=true.
        int row_index = 0;
        while (true) {
            if (target_rows > 0 && row_index >= target_rows) break;
            if (r.BitsAvailable() <= 0) break;
            handle_eol_prefix();
            DecodeRow1D(r, columns, row);
            rows.push_back(row);
            ++row_index;
        }
    } else {
        // Group 3 mixed (K > 0) — every row is preceded by a tag bit:
        // 1 = 1D row, 0 = 2D row. EOL framing optional. Not needed
        // for the v1 fixture corpus (Pillow's libtiff doesn't emit
        // K > 0); leave as a clearly-marked TODO so adding it later
        // is a small focused patch rather than a rewrite.
        throw std::runtime_error(
            "CCITT: K > 0 (Group 3 mixed) not yet implemented");
    }

    DecodedImage out;
    out.width = params.Columns;
    out.height = static_cast<std::uint32_t>(rows.size());
    PackBits(rows, columns, params.BlackIs1, out.bits);
    return out;
}

std::vector<std::byte> Encode(const DecodedImage& image,
                              const Params& params) {
    if (params.Columns == 0) {
        throw std::runtime_error("CCITT: Columns must be > 0");
    }
    if (params.K > 0) {
        throw std::runtime_error(
            "CCITT: K > 0 (Group 3 mixed) encode not implemented");
    }
    const int columns = static_cast<int>(params.Columns);
    const int rows = static_cast<int>(image.height);
    const int bytes_per_row = (columns + 7) / 8;
    if (image.bits.size() <
        static_cast<std::size_t>(bytes_per_row) * static_cast<std::size_t>(rows)) {
        throw std::runtime_error("CCITT: raster smaller than width*height");
    }

    // Unpack one row into 0=white / 1=black internal pixels (inverse of the
    // decoder's PackBits, honouring BlackIs1).
    auto extract_row = [&](int r) {
        std::vector<std::uint8_t> row(static_cast<std::size_t>(columns), 0);
        for (int c = 0; c < columns; ++c) {
            const std::size_t idx =
                static_cast<std::size_t>(r) * bytes_per_row + c / 8;
            const std::uint8_t byte =
                static_cast<std::uint8_t>(image.bits[idx]);
            const int bit = (byte >> (7 - (c % 8))) & 1;
            const int v = params.BlackIs1 ? (1 - bit) : bit;
            row[static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(v);
        }
        return row;
    };

    BitWriter w;
    std::vector<int> ref_changes;  // empty reference line before row 0
    for (int r = 0; r < rows; ++r) {
        const auto row = extract_row(r);
        if (params.K < 0) {
            EncodeRow2D(w, row, ref_changes, columns);
            ref_changes = RowChanges(row, columns);
        } else {  // K == 0 — Group 3 1D
            if (params.EndOfLine) w.Put(kEOL, 12);
            EncodeRow1D(w, row, columns);
            if (params.EncodedByteAlign) w.AlignToByte();
        }
    }

    // End-of-block framing: EOFB (2 EOL) for G4, RTC (6 EOL) for G3.
    if (params.EndOfBlock) {
        const int n = params.K < 0 ? 2 : 6;
        for (int i = 0; i < n; ++i) w.Put(kEOL, 12);
    }

    return w.Take();
}

DecodedImage Parse(std::span<const std::byte> file) {
    auto pf = ParseCfax(file);
    return Decode(std::span<const std::byte>{
                      pf.body.data(), pf.body.size()},
                  pf.params);
}

std::string ToCanonical(const DecodedImage& image) {
    std::string out;
    out.reserve(image.bits.size() * 3 + 32);
    out += "width=" + std::to_string(image.width) + "\n";
    out += "height=" + std::to_string(image.height) + "\n";
    int bytes_per_row = (static_cast<int>(image.width) + 7) / 8;
    static const char* kHex = "0123456789abcdef";
    for (std::uint32_t r = 0; r < image.height; ++r) {
        out += "row " + std::to_string(r) + " ";
        for (int c = 0; c < bytes_per_row; ++c) {
            std::uint8_t b = static_cast<std::uint8_t>(
                image.bits[static_cast<std::size_t>(r) *
                               bytes_per_row + c]);
            out += kHex[b >> 4];
            out += kHex[b & 0xf];
        }
        out += "\n";
    }
    return out;
}

}  // namespace foundation::ccitt
