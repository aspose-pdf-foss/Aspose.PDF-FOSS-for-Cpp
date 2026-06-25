#include "cff.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "cff_strings.inc"

// CFF v1 — full Type 2 CharString scope.
//
// Spec: Adobe Tech Note #5176 (CFF) + #5177 (Type 2 CharStrings).
// Anchor: the project spec
//
// Helper layout (internal namespace, top-down):
//   * Cursor — bounds-checked byte reader
//   * INDEX walker — count + offsets + payload spans
//   * DICT decoder — operand stream + (key, [values]) emit
//   * TopDict / PrivateDict / FontDict resolvers — extract
//     spec-defined operators by their integer key
//   * Charset reader — formats 0/1/2 → vector<SID>
//   * FDSelect reader — formats 0/3 → vector<uint8>
//   * SID resolver — predefined-strings table or String INDEX
//   * Type 2 interpreter — single function with a switch on
//     each opcode; subroutine call/return via a cursor stack;
//     hint-mask byte width derived from cumulative stem count
//   * Public Parse — composes all of the above
//   * ToCanonical — emits the gate text per the anchor

namespace foundation::cff {

namespace {

// --------------------------------------------------------------------
// Cursor — bounds-checked byte reader
// --------------------------------------------------------------------

// Wraps a span<byte> + cursor. Every read advances pos_ and
// throws on out-of-range. Big-endian readers because every
// multi-byte CFF integer is BE.
struct Cursor {
    std::span<const std::byte> data;
    std::size_t pos = 0;

    void Require(std::size_t n) const {
        if (pos + n > data.size()) {
            throw std::runtime_error(
                "cff: cursor out of range");
        }
    }

    std::uint8_t U8() {
        Require(1);
        const auto v = static_cast<std::uint8_t>(data[pos]);
        ++pos;
        return v;
    }

    std::uint16_t U16() {
        Require(2);
        const auto hi = static_cast<std::uint8_t>(data[pos]);
        const auto lo = static_cast<std::uint8_t>(data[pos + 1]);
        pos += 2;
        return static_cast<std::uint16_t>((hi << 8) | lo);
    }

    std::uint32_t U24() {
        Require(3);
        std::uint32_t v = 0;
        for (int i = 0; i < 3; ++i) {
            v = (v << 8)
                | static_cast<std::uint8_t>(data[pos + i]);
        }
        pos += 3;
        return v;
    }

    std::uint32_t U32() {
        Require(4);
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8)
                | static_cast<std::uint8_t>(data[pos + i]);
        }
        pos += 4;
        return v;
    }

    std::int16_t I16() {
        return static_cast<std::int16_t>(U16());
    }

    std::int32_t I32() {
        return static_cast<std::int32_t>(U32());
    }

    // Read N bytes at an offSize-wide field. CFF INDEX
    // offsets are 1..4 bytes wide (offSize); the same shape
    // shows up in the header table for absolute offsets.
    std::uint32_t OffSize(int n) {
        if (n < 1 || n > 4) {
            throw std::runtime_error(
                "cff: offSize must be 1..4");
        }
        std::uint32_t v = 0;
        Require(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            v = (v << 8)
                | static_cast<std::uint8_t>(data[pos + i]);
        }
        pos += static_cast<std::size_t>(n);
        return v;
    }
};

// --------------------------------------------------------------------
// INDEX walker
// --------------------------------------------------------------------

// Per-INDEX state: count, offSize, and the start of the data
// area. We expose entries as spans into the original buffer
// rather than copying — every consumer reads them once.
struct Index {
    std::vector<std::span<const std::byte>> entries;
};

// Parse a CFF INDEX starting at cur.pos. Advances cur past
// the entire INDEX. count==0 is a 2-byte INDEX with no
// offSize byte (per Tech Note #5176 §5).
Index ParseIndex(Cursor& cur) {
    const std::uint16_t count = cur.U16();
    Index idx;
    if (count == 0) {
        return idx;
    }

    const int offSize = static_cast<int>(cur.U8());
    if (offSize < 1 || offSize > 4) {
        throw std::runtime_error("cff: INDEX offSize 1..4");
    }

    // Read count+1 offsets. Offsets are relative to the byte
    // AFTER the offsets table (i.e. data area starts at the
    // byte after the (count+1) × offSize-byte block).
    std::vector<std::uint32_t> offsets;
    offsets.reserve(static_cast<std::size_t>(count) + 1);
    for (std::size_t i = 0; i <= count; ++i) {
        offsets.push_back(cur.OffSize(offSize));
    }
    if (offsets.front() != 1) {
        throw std::runtime_error(
            "cff: INDEX first offset must be 1");
    }

    const std::size_t data_start = cur.pos;
    const std::size_t data_size =
        static_cast<std::size_t>(offsets.back()) - 1;
    if (data_start + data_size > cur.data.size()) {
        throw std::runtime_error(
            "cff: INDEX data area exceeds buffer");
    }

    idx.entries.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t lo =
            static_cast<std::size_t>(offsets[i]) - 1;
        const std::size_t hi =
            static_cast<std::size_t>(offsets[i + 1]) - 1;
        if (hi < lo) {
            throw std::runtime_error(
                "cff: INDEX offsets non-monotonic");
        }
        idx.entries.push_back(
            cur.data.subspan(data_start + lo, hi - lo));
    }
    cur.pos = data_start + data_size;
    return idx;
}

// --------------------------------------------------------------------
// DICT decoder
// --------------------------------------------------------------------

// DICT operand variant.
using DictOperand = std::variant<std::int64_t, double>;

// One DICT operator entry: integer key (1-byte op or
// (12<<8)|op2 escape) plus the accumulated operand list.
struct DictEntry {
    std::uint16_t key;
    std::vector<DictOperand> values;
};

// DICT integer decoders (Tech Note #5176 §5).
//
// b0 in 32..246 → b0 - 139
// b0 in 247..250: b0/b1 → ((b0 - 247) * 256) + b1 + 108
// b0 in 251..254: b0/b1 → -((b0 - 251) * 256) - b1 - 108
// b0 == 0x1C: b1/b2 → int16 BE
// b0 == 0x1D: b1..b4 → int32 BE
//
// Type 2 CharStrings have a SIMILAR but DIFFERENT range
// (different byte for int32; no 0x1D; reals via 0xFF + 4
// bytes 16.16 fixed). The DICT decoder is in this section,
// the Type 2 operand decoder is later. They are deliberately
// SEPARATE functions — semantic_traps[1] in the anchor.
std::int64_t ParseDictInteger(Cursor& cur, std::uint8_t b0) {
    if (b0 >= 32 && b0 <= 246) {
        return static_cast<std::int64_t>(b0) - 139;
    }
    if (b0 >= 247 && b0 <= 250) {
        const std::uint8_t b1 = cur.U8();
        return (static_cast<std::int64_t>(b0) - 247) * 256
             + static_cast<std::int64_t>(b1) + 108;
    }
    if (b0 >= 251 && b0 <= 254) {
        const std::uint8_t b1 = cur.U8();
        return -((static_cast<std::int64_t>(b0) - 251) * 256
                 + static_cast<std::int64_t>(b1) + 108);
    }
    if (b0 == 0x1C) {
        return static_cast<std::int64_t>(cur.I16());
    }
    if (b0 == 0x1D) {
        return static_cast<std::int64_t>(cur.I32());
    }
    throw std::runtime_error("cff: bad DICT integer prefix");
}

// DICT real decoder (Tech Note #5176 §5, b0 == 0x1E).
//
// A nibble stream:
//   0..9   decimal digit
//   a      decimal point
//   b      'E'  (positive exponent intro)
//   c      'E-' (negative exponent intro)
//   d      RESERVED — treat as malformed
//   e      minus sign
//   f      end-of-number
//
// IMPORTANT (semantic_traps[0]): the 0xF terminator can land
// on EITHER nibble of a byte. After consuming 0xF, advance
// the cursor to the NEXT byte boundary regardless of which
// nibble carried the terminator — the trailing nibble in the
// terminator's byte is padding.
double ParseDictReal(Cursor& cur) {
    std::string s;
    bool low_nibble_pending = false;
    std::uint8_t pending_byte = 0;

    auto next_nibble = [&]() -> std::uint8_t {
        if (low_nibble_pending) {
            low_nibble_pending = false;
            return pending_byte & 0x0F;
        }
        pending_byte = cur.U8();
        low_nibble_pending = true;
        return (pending_byte >> 4) & 0x0F;
    };

    for (;;) {
        const std::uint8_t n = next_nibble();
        if (n <= 9) {
            s.push_back(static_cast<char>('0' + n));
        } else if (n == 0xA) {
            s.push_back('.');
        } else if (n == 0xB) {
            s.push_back('E');
        } else if (n == 0xC) {
            s.push_back('E');
            s.push_back('-');
        } else if (n == 0xE) {
            s.push_back('-');
        } else if (n == 0xF) {
            // Terminator. If we were mid-byte (low nibble
            // unread), discard the unread low nibble — the
            // cursor already advanced past pending_byte when
            // low_nibble_pending became true. So nothing to
            // do; subsequent reads start at the next byte.
            break;
        } else {
            throw std::runtime_error(
                "cff: reserved nibble 0xD in DICT real");
        }
    }

    if (s.empty()) {
        return 0.0;
    }
    try {
        return std::stod(s);
    } catch (const std::exception&) {
        throw std::runtime_error(
            "cff: malformed DICT real");
    }
}

// Parse a DICT byte stream (Top DICT / FontDict / Private
// DICT). Returns the operator entries in source order;
// caller's resolver picks up keys it cares about. Trailing
// operands without an operator at end-of-DICT are malformed.
std::vector<DictEntry> ParseDict(std::span<const std::byte> bytes) {
    Cursor cur{bytes, 0};
    std::vector<DictEntry> out;
    std::vector<DictOperand> stack;

    while (cur.pos < bytes.size()) {
        const std::uint8_t b0 = cur.U8();
        if (b0 <= 21) {
            // Operator. Escape via byte 12 + op2 byte; both
            // single-byte operators and escaped operators
            // share the (key, [operands]) representation.
            std::uint16_t key;
            if (b0 == 12) {
                const std::uint8_t op2 = cur.U8();
                key = static_cast<std::uint16_t>(
                    (12u << 8) | op2);
            } else {
                key = b0;
            }
            DictEntry e;
            e.key = key;
            e.values.swap(stack);
            out.push_back(std::move(e));
            // stack.swap(...) above leaves stack empty.
        } else if (b0 == 0x1E) {
            stack.emplace_back(ParseDictReal(cur));
        } else {
            stack.emplace_back(ParseDictInteger(cur, b0));
        }
    }

    if (!stack.empty()) {
        throw std::runtime_error(
            "cff: DICT trailing operands without operator");
    }
    return out;
}

// Convenience: extract operand[0] as double (real or int).
// Used by FontMatrix and other real-typed operators.
double DictAsDouble(const DictOperand& v) {
    if (std::holds_alternative<std::int64_t>(v)) {
        return static_cast<double>(
            std::get<std::int64_t>(v));
    }
    return std::get<double>(v);
}

// Convenience: extract operand[0] as int64. Throws if real.
std::int64_t DictAsInt(const DictOperand& v) {
    if (!std::holds_alternative<std::int64_t>(v)) {
        throw std::runtime_error(
            "cff: expected integer operand");
    }
    return std::get<std::int64_t>(v);
}

// --------------------------------------------------------------------
// TopDict, PrivateDict, FontDict
// --------------------------------------------------------------------

struct TopDict {
    // FontMatrix default [0.001 0 0 0.001 0 0] gives
    // unitsPerEm = 1000.
    std::array<double, 6> font_matrix{
        {0.001, 0.0, 0.0, 0.001, 0.0, 0.0}};
    std::int64_t charstring_type = 2;
    std::int64_t charset_offset = 0;          // 0=ISOAdobe predef
    std::int64_t encoding_offset = 0;         // unused for PDF
    std::int64_t charstrings_offset = 0;      // REQUIRED
    std::int64_t private_size = 0;            // non-CID
    std::int64_t private_offset = 0;          // non-CID
    bool has_private = false;
    bool has_ros = false;                     // CID flag
    std::int64_t fd_array_offset = 0;         // CID
    std::int64_t fd_select_offset = 0;        // CID
    std::int64_t cid_count = 8720;            // CID default
};

struct PrivateDict {
    double default_width_x = 0.0;
    double nominal_width_x = 0.0;
    std::int64_t local_subrs_offset_rel = 0;  // RELATIVE to PD start
    bool has_subrs = false;
};

// FontDict (CID only) wraps a per-FD Private DICT pointer.
struct FontDict {
    PrivateDict priv;
    std::vector<std::span<const std::byte>> local_subrs;
};

// Resolve a TopDict from a parsed DICT entry list.
TopDict ResolveTopDict(const std::vector<DictEntry>& entries) {
    TopDict td;
    for (const auto& e : entries) {
        switch (e.key) {
            case 17: {  // CharStrings
                if (e.values.empty()) {
                    throw std::runtime_error(
                        "cff: CharStrings missing operand");
                }
                td.charstrings_offset =
                    DictAsInt(e.values[0]);
                break;
            }
            case 15: {  // charset
                if (e.values.empty()) break;
                td.charset_offset = DictAsInt(e.values[0]);
                break;
            }
            case 16: {  // Encoding
                if (e.values.empty()) break;
                td.encoding_offset = DictAsInt(e.values[0]);
                break;
            }
            case 18: {  // Private [size, offset]
                if (e.values.size() < 2) break;
                td.private_size = DictAsInt(e.values[0]);
                td.private_offset = DictAsInt(e.values[1]);
                td.has_private = true;
                break;
            }
            case (12u << 8) | 7: {  // FontMatrix
                if (e.values.size() != 6) {
                    throw std::runtime_error(
                        "cff: FontMatrix needs 6 operands");
                }
                for (int i = 0; i < 6; ++i) {
                    td.font_matrix[i] =
                        DictAsDouble(e.values[i]);
                }
                break;
            }
            case (12u << 8) | 6: {  // CharstringType
                if (e.values.empty()) break;
                td.charstring_type = DictAsInt(e.values[0]);
                break;
            }
            case (12u << 8) | 30: {  // ROS [r, o, sup]
                td.has_ros = true;
                break;
            }
            case (12u << 8) | 34: {  // CIDFontVersion
                break;  // ignored
            }
            case (12u << 8) | 35: {  // CIDFontRevision
                break;  // ignored
            }
            case (12u << 8) | 36: {  // CIDFontType
                break;  // ignored
            }
            case (12u << 8) | 37: {  // CIDCount
                if (e.values.empty()) break;
                td.cid_count = DictAsInt(e.values[0]);
                break;
            }
            case (12u << 8) | 38: {  // UIDBase
                break;  // ignored
            }
            case (12u << 8) | 36 + 100: {
                // Filler — CFF spec doesn't actually use this
                // key; left so the switch's brace structure
                // is consistent during edits.
                break;
            }
            // FDArray, FDSelect, FontName under CID.
            // Tech Note #5176 §18 assigns FDArray = 0x0C24 (12 36)
            // and FDSelect = 0x0C25 (12 37). These COLLIDE with
            // CIDFontType (12 36) and CIDCount (12 37) in the
            // same numeric encoding — but per spec, ROS-bearing
            // Top DICTs must use the CID semantics for keys 36/37
            // and there's no overlap with the non-CID semantics
            // because CIDFontType/CIDCount only appear when ROS
            // is present anyway. To avoid the duplicate-case
            // ambiguity we handle FDArray/FDSelect by their
            // dedicated keys 0xFFFE/0xFFFF below — we never
            // emit those keys directly, but a second pass after
            // ROS is detected re-reads the DICT to extract them.
            default:
                break;
        }
    }
    return td;
}

// CID Top DICTs carry FDArray and FDSelect under operator
// codes that collide with CIDFontType / CIDCount in the
// numeric (12, op2) encoding. To keep the switch unambiguous
// we walk the entry list a second time when ROS is present
// and pick out keys (12 36) = FDArray and (12 37) = FDSelect.
// Per Tech Note #5176 §17 the meaning depends on whether ROS
// is present in the same DICT — when it IS, the keys mean
// FDArray/FDSelect; when it's NOT, they mean
// CIDFontType/CIDCount.
void ResolveCidExtras(const std::vector<DictEntry>& entries,
                      TopDict& td) {
    for (const auto& e : entries) {
        if (e.key == ((12u << 8) | 36)) {
            // FDArray (CID branch).
            if (!e.values.empty()) {
                td.fd_array_offset = DictAsInt(e.values[0]);
            }
        } else if (e.key == ((12u << 8) | 37)) {
            // FDSelect (CID branch).
            if (!e.values.empty()) {
                td.fd_select_offset = DictAsInt(e.values[0]);
            }
        }
    }
}

// Resolve a PrivateDict from its DICT entry list.
PrivateDict ResolvePrivateDict(
        const std::vector<DictEntry>& entries) {
    PrivateDict pd;
    for (const auto& e : entries) {
        switch (e.key) {
            case 19: {  // Subrs (relative offset)
                if (e.values.empty()) break;
                pd.local_subrs_offset_rel =
                    DictAsInt(e.values[0]);
                pd.has_subrs = true;
                break;
            }
            case 20: {  // defaultWidthX
                if (e.values.empty()) break;
                pd.default_width_x =
                    DictAsDouble(e.values[0]);
                break;
            }
            case 21: {  // nominalWidthX
                if (e.values.empty()) break;
                pd.nominal_width_x =
                    DictAsDouble(e.values[0]);
                break;
            }
            default:
                break;
        }
    }
    return pd;
}

// --------------------------------------------------------------------
// Charset
// --------------------------------------------------------------------

// A non-CID font's charset is a list of SIDs (one per glyph,
// glyph 0 = .notdef is implicit). A CID-keyed font uses the
// same byte format but the SIDs ARE CIDs.
//
// Format 0:  byte 0 = 0; (numGlyphs-1) × uint16 entries.
// Format 1:  byte 0 = 1; ranges of (firstSID:uint16, nLeft:uint8)
//            covering nLeft+1 glyphs from firstSID.
// Format 2:  byte 0 = 2; ranges of (firstSID:uint16, nLeft:uint16).
std::vector<std::uint32_t> ParseCharset(
        std::span<const std::byte> data,
        std::size_t offset,
        std::uint32_t num_glyphs) {
    std::vector<std::uint32_t> out;
    out.reserve(num_glyphs);
    out.push_back(0);  // gid 0 is always SID 0 (.notdef / CID 0)

    Cursor cur{data, offset};
    const std::uint8_t format = cur.U8();
    if (format == 0) {
        for (std::uint32_t i = 1; i < num_glyphs; ++i) {
            out.push_back(cur.U16());
        }
    } else if (format == 1 || format == 2) {
        std::uint32_t covered = 1;  // .notdef already in
        while (covered < num_glyphs) {
            const std::uint32_t first = cur.U16();
            const std::uint32_t n_left =
                (format == 1) ? cur.U8() : cur.U16();
            for (std::uint32_t i = 0; i <= n_left; ++i) {
                if (covered >= num_glyphs) break;
                out.push_back(first + i);
                ++covered;
            }
        }
    } else {
        throw std::runtime_error(
            "cff: unsupported charset format");
    }
    return out;
}

// --------------------------------------------------------------------
// FDSelect
// --------------------------------------------------------------------

// CID-keyed: maps glyph index to FontDict index.
// Format 0:  byte 0 = 0; numGlyphs × uint8 fd indices.
// Format 3:  byte 0 = 3; nRanges:uint16; nRanges ×
//            (first:uint16, fd:uint8); sentinel:uint16.
//            Sentinel is one past the LAST glyph index
//            (semantic_traps[7]).
std::vector<std::uint8_t> ParseFDSelect(
        std::span<const std::byte> data,
        std::size_t offset,
        std::uint32_t num_glyphs) {
    Cursor cur{data, offset};
    const std::uint8_t format = cur.U8();
    std::vector<std::uint8_t> out(num_glyphs, 0);

    if (format == 0) {
        for (std::uint32_t i = 0; i < num_glyphs; ++i) {
            out[i] = cur.U8();
        }
    } else if (format == 3) {
        const std::uint16_t n_ranges = cur.U16();
        if (n_ranges == 0) {
            throw std::runtime_error(
                "cff: FDSelect format 3 needs ≥1 range");
        }
        std::vector<std::uint16_t> firsts;
        std::vector<std::uint8_t> fds;
        firsts.reserve(n_ranges);
        fds.reserve(n_ranges);
        for (std::uint16_t i = 0; i < n_ranges; ++i) {
            firsts.push_back(cur.U16());
            fds.push_back(cur.U8());
        }
        const std::uint16_t sentinel = cur.U16();
        if (sentinel < num_glyphs) {
            throw std::runtime_error(
                "cff: FDSelect sentinel < num_glyphs");
        }
        // Range i covers gids [firsts[i] .. firsts[i+1])
        // (or the sentinel for the last range).
        for (std::uint16_t i = 0; i < n_ranges; ++i) {
            const std::uint32_t lo = firsts[i];
            const std::uint32_t hi = (i + 1 < n_ranges)
                ? firsts[i + 1] : sentinel;
            for (std::uint32_t g = lo;
                 g < hi && g < num_glyphs; ++g) {
                out[g] = fds[i];
            }
        }
    } else {
        throw std::runtime_error(
            "cff: unsupported FDSelect format");
    }
    return out;
}

// --------------------------------------------------------------------
// SID resolver
// --------------------------------------------------------------------

// SIDs 0..390 are the predefined Standard Strings table.
// SIDs ≥ 391 reference the CFF String INDEX.
std::string ResolveSid(std::int64_t sid,
                       const Index& string_index) {
    if (sid < 0) {
        throw std::runtime_error("cff: negative SID");
    }
    if (sid < 391) {
        return std::string(detail::kPredefinedStrings[sid]);
    }
    const std::size_t idx =
        static_cast<std::size_t>(sid) - 391;
    if (idx >= string_index.entries.size()) {
        throw std::runtime_error(
            "cff: SID out of String INDEX range");
    }
    const auto& s = string_index.entries[idx];
    return std::string(
        reinterpret_cast<const char*>(s.data()), s.size());
}

// --------------------------------------------------------------------
// Type 2 CharString interpreter
// --------------------------------------------------------------------

// Subroutine bias formula (Tech Note #5177 §4.7).
//   count <  1240 → 107
//   count < 33900 → 1131
//   else          → 32768
std::int32_t SubrBias(std::size_t count) {
    if (count < 1240) return 107;
    if (count < 33900) return 1131;
    return 32768;
}

// One frame on the subroutine return stack.
struct SubrFrame {
    std::span<const std::byte> program;
    std::size_t pos;
};

// Per-glyph interpreter state. Operand stack max depth 96.
struct InterpState {
    std::vector<double> stack;     // operand stack
    std::vector<SubrFrame> rets;   // subroutine return stack
    std::int32_t cur_x = 0;
    std::int32_t cur_y = 0;
    std::int32_t advance = 0;
    std::int32_t subpath_start_x = 0;
    std::int32_t subpath_start_y = 0;
    std::vector<PathSegment> path;
    std::uint32_t stem_count = 0;  // cumulative stems
    bool width_resolved = false;
    bool subpath_open = false;
};

// Round a float coord to int32 — Type 2 emits integer
// coordinates; floats only arise from the rare 16.16 fixed-
// point operand and after-arithmetic stack values.
std::int32_t RoundCoord(double v) {
    return static_cast<std::int32_t>(std::lround(v));
}

// Append a M/L/C/Z segment.
void EmitMove(InterpState& s, std::int32_t x, std::int32_t y) {
    if (s.subpath_open) {
        s.path.push_back({PathOp::Z, {}});
    }
    s.path.push_back({PathOp::M, {x, y}});
    s.subpath_start_x = x;
    s.subpath_start_y = y;
    s.subpath_open = true;
}

void EmitLine(InterpState& s, std::int32_t x, std::int32_t y) {
    s.path.push_back({PathOp::L, {x, y}});
}

void EmitCurve(InterpState& s,
               std::int32_t x1, std::int32_t y1,
               std::int32_t x2, std::int32_t y2,
               std::int32_t x3, std::int32_t y3) {
    s.path.push_back(
        {PathOp::C, {x1, y1, x2, y2, x3, y3}});
}

// Resolve the (deferred) glyph advance width when we hit the
// FIRST stack-clearing operator. The Type 2 width convention
// (Tech Note #5177 §4.5):
//   if stack-depth > op.declared_count:
//     width = stack[0] + nominalWidthX  (consume stack[0])
//   else:
//     width = defaultWidthX
// declared_count is the number of operands the operator
// itself REQUIRES (per its grammar). Stack-clearing ops
// include all path ops, all stem ops, and endchar.
//
// Returns the operand offset to skip (0 or 1) — caller uses
// stack[offset...] as the operator's actual operands.
std::size_t ResolveWidth(InterpState& s,
                         const PrivateDict& pd,
                         std::size_t declared_count,
                         bool variadic) {
    if (s.width_resolved) return 0;
    s.width_resolved = true;
    const std::size_t depth = s.stack.size();
    bool has_extra = false;
    if (variadic) {
        // Variadic operators (e.g. rlineto takes any pairs;
        // rrcurveto takes any sextets). The "extra" slot is
        // present iff the depth is NOT an exact multiple of
        // the operand-group size (declared_count is the
        // group size for variadic ops).
        if (declared_count == 0) {
            has_extra = (depth >= 1);
        } else {
            has_extra = (depth % declared_count) != 0;
        }
    } else {
        has_extra = depth > declared_count;
    }
    if (has_extra) {
        s.advance = RoundCoord(
            s.stack[0] + pd.nominal_width_x);
        return 1;
    }
    s.advance = RoundCoord(pd.default_width_x);
    return 0;
}

// Type 2 operand decoder (Tech Note #5177 §3.2).
// b0 in 32..246 → b0 - 139
// b0 in 247..250 + 1 byte → ((b0 - 247) * 256) + b1 + 108
// b0 in 251..254 + 1 byte → -((b0 - 251) * 256) - b1 - 108
// b0 == 28 + 2 bytes → int16 BE (note: byte 28, NOT 0x1C)
// b0 == 255 + 4 bytes → 16.16 fixed-point real (signed)
// 0x1C / 0x1D / 0x1E DO NOT exist in Type 2 — semantic_traps[1].
double ParseT2Operand(Cursor& cur, std::uint8_t b0) {
    if (b0 >= 32 && b0 <= 246) {
        return static_cast<double>(
            static_cast<int>(b0) - 139);
    }
    if (b0 >= 247 && b0 <= 250) {
        const std::uint8_t b1 = cur.U8();
        return static_cast<double>(
            (static_cast<int>(b0) - 247) * 256
            + static_cast<int>(b1) + 108);
    }
    if (b0 >= 251 && b0 <= 254) {
        const std::uint8_t b1 = cur.U8();
        return static_cast<double>(
            -((static_cast<int>(b0) - 251) * 256
              + static_cast<int>(b1) + 108));
    }
    if (b0 == 28) {
        const std::int16_t v = cur.I16();
        return static_cast<double>(v);
    }
    if (b0 == 255) {
        const std::int32_t v = cur.I32();
        // 16.16 signed fixed-point.
        return static_cast<double>(v) / 65536.0;
    }
    throw std::runtime_error("cff: bad T2 operand prefix");
}

// Run the Type 2 interpreter on a charstring program. Used
// for glyph CharStrings AND for subroutine bodies (recursive
// expansion via the return stack).
void RunCharString(
        const std::span<const std::byte> program,
        InterpState& s,
        const std::vector<std::span<const std::byte>>& locals,
        const std::vector<std::span<const std::byte>>& globals,
        const PrivateDict& pd,
        int depth);

// One stack-clearing operator's pre-processing: resolve width
// + return the stack offset where REAL operands start.
std::size_t StartClearing(InterpState& s, const PrivateDict& pd,
                          std::size_t declared,
                          bool variadic) {
    return ResolveWidth(s, pd, declared, variadic);
}

// Helper: count stem pairs added by a stem operator. Each
// stem is two operands (y or x and dy or dx); the count we
// track is the NUMBER OF STEMS, not the operand count.
void AddStems(InterpState& s, std::size_t operand_offset) {
    const std::size_t pairs =
        (s.stack.size() - operand_offset) / 2;
    s.stem_count += static_cast<std::uint32_t>(pairs);
}

// Hint-mask byte width is ceil(stem_count / 8). Mask bytes
// follow the operator immediately and consume cur.pos.
std::size_t HintMaskBytes(const InterpState& s) {
    return (s.stem_count + 7) / 8;
}

void RunCharString(
        const std::span<const std::byte> program,
        InterpState& s,
        const std::vector<std::span<const std::byte>>& locals,
        const std::vector<std::span<const std::byte>>& globals,
        const PrivateDict& pd,
        int depth) {
    if (depth > 10) {
        throw std::runtime_error(
            "cff: subroutine recursion depth > 10");
    }

    Cursor cur{program, 0};
    bool ended = false;

    while (cur.pos < program.size() && !ended) {
        const std::uint8_t b0 = cur.U8();

        // Operator branch.
        if (b0 <= 31 && b0 != 28) {
            std::uint16_t key = b0;
            if (b0 == 12) {
                const std::uint8_t op2 = cur.U8();
                key = static_cast<std::uint16_t>(
                    (12u << 8) | op2);
            }

            switch (key) {
                case 1: {   // hstem
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    break;
                }
                case 3: {   // vstem
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    break;
                }
                case 18: {  // hstemhm
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    break;
                }
                case 23: {  // vstemhm
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    break;
                }
                case 19: {  // hintmask
                    // Stems may also be DECLARED before the
                    // mask without an explicit stem op — any
                    // operands left on the stack at hintmask
                    // are vstem-shaped pairs (Tech Note
                    // #5177 §4.3).
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    cur.pos += HintMaskBytes(s);
                    if (cur.pos > program.size()) {
                        throw std::runtime_error(
                            "cff: hintmask overruns program");
                    }
                    break;
                }
                case 20: {  // cntrmask
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    AddStems(s, off);
                    s.stack.clear();
                    cur.pos += HintMaskBytes(s);
                    if (cur.pos > program.size()) {
                        throw std::runtime_error(
                            "cff: cntrmask overruns program");
                    }
                    break;
                }
                case 21: {  // rmoveto: dx dy
                    const auto off =
                        StartClearing(s, pd, 2, false);
                    if (s.stack.size() < off + 2) {
                        throw std::runtime_error(
                            "cff: rmoveto needs 2 operands");
                    }
                    s.cur_x += RoundCoord(s.stack[off]);
                    s.cur_y += RoundCoord(s.stack[off + 1]);
                    EmitMove(s, s.cur_x, s.cur_y);
                    s.stack.clear();
                    break;
                }
                case 22: {  // hmoveto: dx
                    const auto off =
                        StartClearing(s, pd, 1, false);
                    if (s.stack.size() < off + 1) {
                        throw std::runtime_error(
                            "cff: hmoveto needs 1 operand");
                    }
                    s.cur_x += RoundCoord(s.stack[off]);
                    EmitMove(s, s.cur_x, s.cur_y);
                    s.stack.clear();
                    break;
                }
                case 4: {   // vmoveto: dy
                    const auto off =
                        StartClearing(s, pd, 1, false);
                    if (s.stack.size() < off + 1) {
                        throw std::runtime_error(
                            "cff: vmoveto needs 1 operand");
                    }
                    s.cur_y += RoundCoord(s.stack[off]);
                    EmitMove(s, s.cur_x, s.cur_y);
                    s.stack.clear();
                    break;
                }
                case 5: {   // rlineto: {dx dy}+
                    const auto off =
                        StartClearing(s, pd, 2, true);
                    for (std::size_t i = off;
                         i + 1 < s.stack.size(); i += 2) {
                        s.cur_x +=
                            RoundCoord(s.stack[i]);
                        s.cur_y +=
                            RoundCoord(s.stack[i + 1]);
                        EmitLine(s, s.cur_x, s.cur_y);
                    }
                    s.stack.clear();
                    break;
                }
                case 6: {   // hlineto: alternates dx,dy starting dx
                    const auto off =
                        StartClearing(s, pd, 1, true);
                    bool horiz = true;
                    for (std::size_t i = off;
                         i < s.stack.size(); ++i) {
                        if (horiz) {
                            s.cur_x +=
                                RoundCoord(s.stack[i]);
                        } else {
                            s.cur_y +=
                                RoundCoord(s.stack[i]);
                        }
                        EmitLine(s, s.cur_x, s.cur_y);
                        horiz = !horiz;
                    }
                    s.stack.clear();
                    break;
                }
                case 7: {   // vlineto: alternates dy,dx starting dy
                    const auto off =
                        StartClearing(s, pd, 1, true);
                    bool vert = true;
                    for (std::size_t i = off;
                         i < s.stack.size(); ++i) {
                        if (vert) {
                            s.cur_y +=
                                RoundCoord(s.stack[i]);
                        } else {
                            s.cur_x +=
                                RoundCoord(s.stack[i]);
                        }
                        EmitLine(s, s.cur_x, s.cur_y);
                        vert = !vert;
                    }
                    s.stack.clear();
                    break;
                }
                case 8: {   // rrcurveto: {dx1 dy1 dx2 dy2 dx3 dy3}+
                    const auto off =
                        StartClearing(s, pd, 6, true);
                    for (std::size_t i = off;
                         i + 5 < s.stack.size(); i += 6) {
                        const std::int32_t x1 = s.cur_x
                            + RoundCoord(s.stack[i]);
                        const std::int32_t y1 = s.cur_y
                            + RoundCoord(s.stack[i + 1]);
                        const std::int32_t x2 = x1
                            + RoundCoord(s.stack[i + 2]);
                        const std::int32_t y2 = y1
                            + RoundCoord(s.stack[i + 3]);
                        const std::int32_t x3 = x2
                            + RoundCoord(s.stack[i + 4]);
                        const std::int32_t y3 = y2
                            + RoundCoord(s.stack[i + 5]);
                        EmitCurve(s, x1, y1, x2, y2, x3, y3);
                        s.cur_x = x3;
                        s.cur_y = y3;
                    }
                    s.stack.clear();
                    break;
                }
                case 24: {  // rcurveline: {curve}+ {dx dy}
                    const auto off =
                        StartClearing(s, pd, 6, false);
                    // Variable curves followed by one line.
                    std::size_t i = off;
                    while (i + 7 < s.stack.size()) {
                        const std::int32_t x1 = s.cur_x
                            + RoundCoord(s.stack[i]);
                        const std::int32_t y1 = s.cur_y
                            + RoundCoord(s.stack[i + 1]);
                        const std::int32_t x2 = x1
                            + RoundCoord(s.stack[i + 2]);
                        const std::int32_t y2 = y1
                            + RoundCoord(s.stack[i + 3]);
                        const std::int32_t x3 = x2
                            + RoundCoord(s.stack[i + 4]);
                        const std::int32_t y3 = y2
                            + RoundCoord(s.stack[i + 5]);
                        EmitCurve(s, x1, y1, x2, y2, x3, y3);
                        s.cur_x = x3;
                        s.cur_y = y3;
                        i += 6;
                    }
                    if (i + 1 >= s.stack.size()) {
                        throw std::runtime_error(
                            "cff: rcurveline missing line tail");
                    }
                    s.cur_x += RoundCoord(s.stack[i]);
                    s.cur_y += RoundCoord(s.stack[i + 1]);
                    EmitLine(s, s.cur_x, s.cur_y);
                    s.stack.clear();
                    break;
                }
                case 25: {  // rlinecurve: {dx dy}+ {curve}
                    const auto off =
                        StartClearing(s, pd, 2, false);
                    std::size_t i = off;
                    while (i + 7 < s.stack.size()) {
                        s.cur_x +=
                            RoundCoord(s.stack[i]);
                        s.cur_y +=
                            RoundCoord(s.stack[i + 1]);
                        EmitLine(s, s.cur_x, s.cur_y);
                        i += 2;
                    }
                    if (i + 5 >= s.stack.size()) {
                        throw std::runtime_error(
                            "cff: rlinecurve missing curve tail");
                    }
                    const std::int32_t x1 = s.cur_x
                        + RoundCoord(s.stack[i]);
                    const std::int32_t y1 = s.cur_y
                        + RoundCoord(s.stack[i + 1]);
                    const std::int32_t x2 = x1
                        + RoundCoord(s.stack[i + 2]);
                    const std::int32_t y2 = y1
                        + RoundCoord(s.stack[i + 3]);
                    const std::int32_t x3 = x2
                        + RoundCoord(s.stack[i + 4]);
                    const std::int32_t y3 = y2
                        + RoundCoord(s.stack[i + 5]);
                    EmitCurve(s, x1, y1, x2, y2, x3, y3);
                    s.cur_x = x3;
                    s.cur_y = y3;
                    s.stack.clear();
                    break;
                }
                case 26: {  // vvcurveto: opt-dx0 {dy1 dx2 dy2 dy3}+
                    const auto off =
                        StartClearing(s, pd, 4, true);
                    std::size_t i = off;
                    const bool has_dx0 =
                        ((s.stack.size() - off) % 4) == 1;
                    if (has_dx0) {
                        s.cur_x +=
                            RoundCoord(s.stack[i]);
                        ++i;
                    }
                    while (i + 3 < s.stack.size()) {
                        const std::int32_t x1 = s.cur_x;
                        const std::int32_t y1 = s.cur_y
                            + RoundCoord(s.stack[i]);
                        const std::int32_t x2 = x1
                            + RoundCoord(s.stack[i + 1]);
                        const std::int32_t y2 = y1
                            + RoundCoord(s.stack[i + 2]);
                        const std::int32_t x3 = x2;
                        const std::int32_t y3 = y2
                            + RoundCoord(s.stack[i + 3]);
                        EmitCurve(s, x1, y1, x2, y2, x3, y3);
                        s.cur_x = x3;
                        s.cur_y = y3;
                        i += 4;
                    }
                    s.stack.clear();
                    break;
                }
                case 27: {  // hhcurveto: opt-dy0 {dx1 dx2 dy2 dx3}+
                    const auto off =
                        StartClearing(s, pd, 4, true);
                    std::size_t i = off;
                    const bool has_dy0 =
                        ((s.stack.size() - off) % 4) == 1;
                    if (has_dy0) {
                        s.cur_y +=
                            RoundCoord(s.stack[i]);
                        ++i;
                    }
                    while (i + 3 < s.stack.size()) {
                        const std::int32_t x1 = s.cur_x
                            + RoundCoord(s.stack[i]);
                        const std::int32_t y1 = s.cur_y;
                        const std::int32_t x2 = x1
                            + RoundCoord(s.stack[i + 1]);
                        const std::int32_t y2 = y1
                            + RoundCoord(s.stack[i + 2]);
                        const std::int32_t x3 = x2
                            + RoundCoord(s.stack[i + 3]);
                        const std::int32_t y3 = y2;
                        EmitCurve(s, x1, y1, x2, y2, x3, y3);
                        s.cur_x = x3;
                        s.cur_y = y3;
                        i += 4;
                    }
                    s.stack.clear();
                    break;
                }
                case 30: {  // vhcurveto
                    // Repeating: {dy1 dx2 dy2 dx3} {dx dy3 dx2 dy2 dx3}*
                    // Tech Note #5177 §4.4 — alternating
                    // V-H, H-V curves with optional final
                    // single-coord adjustment.
                    const auto off =
                        StartClearing(s, pd, 4, true);
                    std::size_t i = off;
                    bool start_v = true;  // first curve starts vertical
                    while (i + 3 < s.stack.size()) {
                        const std::size_t rem =
                            s.stack.size() - i;
                        const bool has_tail =
                            (rem == 5 || rem == 9
                             || rem == 13);
                        if (start_v) {
                            const std::int32_t x1 = s.cur_x;
                            const std::int32_t y1 = s.cur_y
                                + RoundCoord(s.stack[i]);
                            const std::int32_t x2 = x1
                                + RoundCoord(s.stack[i + 1]);
                            const std::int32_t y2 = y1
                                + RoundCoord(s.stack[i + 2]);
                            const std::int32_t x3 = x2
                                + RoundCoord(s.stack[i + 3]);
                            std::int32_t y3 = y2;
                            if (has_tail
                                && i + 4 == s.stack.size() - 1) {
                                y3 += RoundCoord(s.stack[i + 4]);
                                ++i;
                            }
                            EmitCurve(
                                s, x1, y1, x2, y2, x3, y3);
                            s.cur_x = x3;
                            s.cur_y = y3;
                        } else {
                            const std::int32_t x1 = s.cur_x
                                + RoundCoord(s.stack[i]);
                            const std::int32_t y1 = s.cur_y;
                            const std::int32_t x2 = x1
                                + RoundCoord(s.stack[i + 1]);
                            const std::int32_t y2 = y1
                                + RoundCoord(s.stack[i + 2]);
                            std::int32_t x3 = x2;
                            const std::int32_t y3 = y2
                                + RoundCoord(s.stack[i + 3]);
                            if (has_tail
                                && i + 4 == s.stack.size() - 1) {
                                x3 += RoundCoord(s.stack[i + 4]);
                                ++i;
                            }
                            EmitCurve(
                                s, x1, y1, x2, y2, x3, y3);
                            s.cur_x = x3;
                            s.cur_y = y3;
                        }
                        i += 4;
                        start_v = !start_v;
                    }
                    s.stack.clear();
                    break;
                }
                case 31: {  // hvcurveto: same shape, swap H<->V
                    const auto off =
                        StartClearing(s, pd, 4, true);
                    std::size_t i = off;
                    bool start_h = true;
                    while (i + 3 < s.stack.size()) {
                        const std::size_t rem =
                            s.stack.size() - i;
                        const bool has_tail =
                            (rem == 5 || rem == 9
                             || rem == 13);
                        if (start_h) {
                            const std::int32_t x1 = s.cur_x
                                + RoundCoord(s.stack[i]);
                            const std::int32_t y1 = s.cur_y;
                            const std::int32_t x2 = x1
                                + RoundCoord(s.stack[i + 1]);
                            const std::int32_t y2 = y1
                                + RoundCoord(s.stack[i + 2]);
                            std::int32_t x3 = x2;
                            const std::int32_t y3 = y2
                                + RoundCoord(s.stack[i + 3]);
                            if (has_tail
                                && i + 4 == s.stack.size() - 1) {
                                x3 += RoundCoord(s.stack[i + 4]);
                                ++i;
                            }
                            EmitCurve(
                                s, x1, y1, x2, y2, x3, y3);
                            s.cur_x = x3;
                            s.cur_y = y3;
                        } else {
                            const std::int32_t x1 = s.cur_x;
                            const std::int32_t y1 = s.cur_y
                                + RoundCoord(s.stack[i]);
                            const std::int32_t x2 = x1
                                + RoundCoord(s.stack[i + 1]);
                            const std::int32_t y2 = y1
                                + RoundCoord(s.stack[i + 2]);
                            const std::int32_t x3 = x2
                                + RoundCoord(s.stack[i + 3]);
                            std::int32_t y3 = y2;
                            if (has_tail
                                && i + 4 == s.stack.size() - 1) {
                                y3 += RoundCoord(s.stack[i + 4]);
                                ++i;
                            }
                            EmitCurve(
                                s, x1, y1, x2, y2, x3, y3);
                            s.cur_x = x3;
                            s.cur_y = y3;
                        }
                        i += 4;
                        start_h = !start_h;
                    }
                    s.stack.clear();
                    break;
                }
                case 14: {  // endchar
                    StartClearing(s, pd, 0, false);
                    if (s.subpath_open) {
                        s.path.push_back({PathOp::Z, {}});
                        s.subpath_open = false;
                    }
                    s.stack.clear();
                    ended = true;
                    break;
                }
                case 10: {  // callsubr (local)
                    if (s.stack.empty()) {
                        throw std::runtime_error(
                            "cff: callsubr underflow");
                    }
                    const std::int32_t bias =
                        SubrBias(locals.size());
                    const std::int32_t biased =
                        RoundCoord(s.stack.back());
                    s.stack.pop_back();
                    const std::int64_t idx =
                        static_cast<std::int64_t>(biased)
                        + static_cast<std::int64_t>(bias);
                    if (idx < 0
                        || static_cast<std::size_t>(idx)
                           >= locals.size()) {
                        throw std::runtime_error(
                            "cff: local subr out of range");
                    }
                    RunCharString(
                        locals[static_cast<std::size_t>(idx)],
                        s, locals, globals, pd, depth + 1);
                    break;
                }
                case 29: {  // callgsubr (global)
                    if (s.stack.empty()) {
                        throw std::runtime_error(
                            "cff: callgsubr underflow");
                    }
                    const std::int32_t bias =
                        SubrBias(globals.size());
                    const std::int32_t biased =
                        RoundCoord(s.stack.back());
                    s.stack.pop_back();
                    const std::int64_t idx =
                        static_cast<std::int64_t>(biased)
                        + static_cast<std::int64_t>(bias);
                    if (idx < 0
                        || static_cast<std::size_t>(idx)
                           >= globals.size()) {
                        throw std::runtime_error(
                            "cff: global subr out of range");
                    }
                    RunCharString(
                        globals[static_cast<std::size_t>(idx)],
                        s, locals, globals, pd, depth + 1);
                    break;
                }
                case 11: {  // return
                    return;
                }
                case (12u << 8) | 35: {  // flex
                    StartClearing(s, pd, 13, false);
                    if (s.stack.size() < 13) {
                        throw std::runtime_error(
                            "cff: flex needs 13 operands");
                    }
                    const std::size_t base =
                        s.stack.size() - 13;
                    // 6 deltas per cubic + flex-depth.
                    const std::int32_t dx1 =
                        RoundCoord(s.stack[base]);
                    const std::int32_t dy1 =
                        RoundCoord(s.stack[base + 1]);
                    const std::int32_t dx2 =
                        RoundCoord(s.stack[base + 2]);
                    const std::int32_t dy2 =
                        RoundCoord(s.stack[base + 3]);
                    const std::int32_t dx3 =
                        RoundCoord(s.stack[base + 4]);
                    const std::int32_t dy3 =
                        RoundCoord(s.stack[base + 5]);
                    const std::int32_t dx4 =
                        RoundCoord(s.stack[base + 6]);
                    const std::int32_t dy4 =
                        RoundCoord(s.stack[base + 7]);
                    const std::int32_t dx5 =
                        RoundCoord(s.stack[base + 8]);
                    const std::int32_t dy5 =
                        RoundCoord(s.stack[base + 9]);
                    const std::int32_t dx6 =
                        RoundCoord(s.stack[base + 10]);
                    const std::int32_t dy6 =
                        RoundCoord(s.stack[base + 11]);
                    const std::int32_t x1 = s.cur_x + dx1;
                    const std::int32_t y1 = s.cur_y + dy1;
                    const std::int32_t x2 = x1 + dx2;
                    const std::int32_t y2 = y1 + dy2;
                    const std::int32_t x3 = x2 + dx3;
                    const std::int32_t y3 = y2 + dy3;
                    EmitCurve(s, x1, y1, x2, y2, x3, y3);
                    const std::int32_t x4 = x3 + dx4;
                    const std::int32_t y4 = y3 + dy4;
                    const std::int32_t x5 = x4 + dx5;
                    const std::int32_t y5 = y4 + dy5;
                    const std::int32_t x6 = x5 + dx6;
                    const std::int32_t y6 = y5 + dy6;
                    EmitCurve(s, x4, y4, x5, y5, x6, y6);
                    s.cur_x = x6;
                    s.cur_y = y6;
                    s.stack.clear();
                    break;
                }
                case (12u << 8) | 34: {  // hflex
                    StartClearing(s, pd, 7, false);
                    if (s.stack.size() < 7) {
                        throw std::runtime_error(
                            "cff: hflex needs 7 operands");
                    }
                    const std::size_t base =
                        s.stack.size() - 7;
                    const std::int32_t dx1 =
                        RoundCoord(s.stack[base]);
                    const std::int32_t dx2 =
                        RoundCoord(s.stack[base + 1]);
                    const std::int32_t dy2 =
                        RoundCoord(s.stack[base + 2]);
                    const std::int32_t dx3 =
                        RoundCoord(s.stack[base + 3]);
                    const std::int32_t dx4 =
                        RoundCoord(s.stack[base + 4]);
                    const std::int32_t dx5 =
                        RoundCoord(s.stack[base + 5]);
                    const std::int32_t dx6 =
                        RoundCoord(s.stack[base + 6]);
                    const std::int32_t x1 = s.cur_x + dx1;
                    const std::int32_t y1 = s.cur_y;
                    const std::int32_t x2 = x1 + dx2;
                    const std::int32_t y2 = y1 + dy2;
                    const std::int32_t x3 = x2 + dx3;
                    const std::int32_t y3 = y2;
                    EmitCurve(s, x1, y1, x2, y2, x3, y3);
                    const std::int32_t x4 = x3 + dx4;
                    const std::int32_t y4 = y3;
                    const std::int32_t x5 = x4 + dx5;
                    const std::int32_t y5 = y4 - dy2;  // back to y0
                    const std::int32_t x6 = x5 + dx6;
                    const std::int32_t y6 = y5;
                    EmitCurve(s, x4, y4, x5, y5, x6, y6);
                    s.cur_x = x6;
                    s.cur_y = y6;
                    s.stack.clear();
                    break;
                }
                case (12u << 8) | 36: {  // hflex1
                    StartClearing(s, pd, 9, false);
                    if (s.stack.size() < 9) {
                        throw std::runtime_error(
                            "cff: hflex1 needs 9 operands");
                    }
                    const std::size_t base =
                        s.stack.size() - 9;
                    const std::int32_t start_y = s.cur_y;
                    const std::int32_t dx1 =
                        RoundCoord(s.stack[base]);
                    const std::int32_t dy1 =
                        RoundCoord(s.stack[base + 1]);
                    const std::int32_t dx2 =
                        RoundCoord(s.stack[base + 2]);
                    const std::int32_t dy2 =
                        RoundCoord(s.stack[base + 3]);
                    const std::int32_t dx3 =
                        RoundCoord(s.stack[base + 4]);
                    const std::int32_t dx4 =
                        RoundCoord(s.stack[base + 5]);
                    const std::int32_t dx5 =
                        RoundCoord(s.stack[base + 6]);
                    const std::int32_t dy5 =
                        RoundCoord(s.stack[base + 7]);
                    const std::int32_t dx6 =
                        RoundCoord(s.stack[base + 8]);
                    const std::int32_t x1 = s.cur_x + dx1;
                    const std::int32_t y1 = s.cur_y + dy1;
                    const std::int32_t x2 = x1 + dx2;
                    const std::int32_t y2 = y1 + dy2;
                    const std::int32_t x3 = x2 + dx3;
                    const std::int32_t y3 = y2;
                    EmitCurve(s, x1, y1, x2, y2, x3, y3);
                    const std::int32_t x4 = x3 + dx4;
                    const std::int32_t y4 = y3;
                    const std::int32_t x5 = x4 + dx5;
                    const std::int32_t y5 = y4 + dy5;
                    const std::int32_t x6 = x5 + dx6;
                    const std::int32_t y6 = start_y;
                    EmitCurve(s, x4, y4, x5, y5, x6, y6);
                    s.cur_x = x6;
                    s.cur_y = y6;
                    s.stack.clear();
                    break;
                }
                case (12u << 8) | 37: {  // flex1
                    StartClearing(s, pd, 11, false);
                    if (s.stack.size() < 11) {
                        throw std::runtime_error(
                            "cff: flex1 needs 11 operands");
                    }
                    const std::size_t base =
                        s.stack.size() - 11;
                    const std::int32_t start_x = s.cur_x;
                    const std::int32_t start_y = s.cur_y;
                    const std::int32_t dx1 =
                        RoundCoord(s.stack[base]);
                    const std::int32_t dy1 =
                        RoundCoord(s.stack[base + 1]);
                    const std::int32_t dx2 =
                        RoundCoord(s.stack[base + 2]);
                    const std::int32_t dy2 =
                        RoundCoord(s.stack[base + 3]);
                    const std::int32_t dx3 =
                        RoundCoord(s.stack[base + 4]);
                    const std::int32_t dy3 =
                        RoundCoord(s.stack[base + 5]);
                    const std::int32_t dx4 =
                        RoundCoord(s.stack[base + 6]);
                    const std::int32_t dy4 =
                        RoundCoord(s.stack[base + 7]);
                    const std::int32_t dx5 =
                        RoundCoord(s.stack[base + 8]);
                    const std::int32_t dy5 =
                        RoundCoord(s.stack[base + 9]);
                    const std::int32_t d6 =
                        RoundCoord(s.stack[base + 10]);
                    const std::int32_t x1 = s.cur_x + dx1;
                    const std::int32_t y1 = s.cur_y + dy1;
                    const std::int32_t x2 = x1 + dx2;
                    const std::int32_t y2 = y1 + dy2;
                    const std::int32_t x3 = x2 + dx3;
                    const std::int32_t y3 = y2 + dy3;
                    EmitCurve(s, x1, y1, x2, y2, x3, y3);
                    const std::int32_t x4 = x3 + dx4;
                    const std::int32_t y4 = y3 + dy4;
                    const std::int32_t x5 = x4 + dx5;
                    const std::int32_t y5 = y4 + dy5;
                    // d6 applies to the dominant axis — H or V
                    // depending on which has greater span from
                    // start. (Tech Note #5177 §4.6.)
                    const std::int32_t total_dx = x5 - start_x;
                    const std::int32_t total_dy = y5 - start_y;
                    std::int32_t x6 = x5;
                    std::int32_t y6 = y5;
                    if (std::abs(total_dx)
                            > std::abs(total_dy)) {
                        x6 += d6;
                        y6 = start_y;
                    } else {
                        y6 += d6;
                        x6 = start_x;
                    }
                    EmitCurve(s, x4, y4, x5, y5, x6, y6);
                    s.cur_x = x6;
                    s.cur_y = y6;
                    s.stack.clear();
                    break;
                }
                case (12u << 8) | 0: {  // dotsection (deprecated)
                    s.stack.clear();
                    break;
                }
                case (12u << 8) | 6: {  // seac (deprecated)
                    throw std::runtime_error(
                        "cff: seac out of v1 scope");
                }
                default: {
                    throw std::runtime_error(
                        "cff: unknown Type 2 operator");
                }
            }
        } else {
            // Operand. Push the parsed value onto the stack.
            const double v = ParseT2Operand(cur, b0);
            if (s.stack.size() >= 96) {
                throw std::runtime_error(
                    "cff: T2 stack overflow (>96)");
            }
            s.stack.push_back(v);
        }
    }
}

// --------------------------------------------------------------------
// Parse — public entry point
// --------------------------------------------------------------------

}  // namespace (anonymous)

Cff Parse(std::span<const std::byte> input) {
    Cursor cur{input, 0};

    // Header: major (1), minor, hdrSize, offSize.
    const std::uint8_t major = cur.U8();
    if (major != 1) {
        throw std::runtime_error(
            "cff: majorVersion != 1 (CFF2 is a separate primitive)");
    }
    cur.U8();                      // minor (informational)
    const std::uint8_t hdrSize = cur.U8();
    cur.U8();                      // header offSize (unused in v1)
    if (hdrSize > input.size()) {
        throw std::runtime_error("cff: hdrSize > buffer");
    }
    cur.pos = hdrSize;

    // Name INDEX — exactly one entry in CFF v1.
    Index name_idx = ParseIndex(cur);
    if (name_idx.entries.empty()) {
        throw std::runtime_error(
            "cff: Name INDEX must have at least one entry");
    }

    // Top DICT INDEX — same count as Name INDEX.
    Index top_dict_idx = ParseIndex(cur);
    if (top_dict_idx.entries.empty()) {
        throw std::runtime_error(
            "cff: Top DICT INDEX must have at least one entry");
    }

    // String INDEX — supplies SIDs ≥ 391.
    Index string_idx = ParseIndex(cur);

    // Global Subrs INDEX.
    Index gsubrs_idx = ParseIndex(cur);

    // Resolve Top DICT.
    const auto top_dict_entries =
        ParseDict(top_dict_idx.entries[0]);
    TopDict top = ResolveTopDict(top_dict_entries);
    if (top.has_ros) {
        ResolveCidExtras(top_dict_entries, top);
    }
    if (top.charstring_type != 2) {
        throw std::runtime_error(
            "cff: CharstringType != 2 (Type 1 out of v1 scope)");
    }
    if (top.charstrings_offset == 0) {
        throw std::runtime_error(
            "cff: Top DICT missing CharStrings");
    }
    if (!top.has_ros && !top.has_private) {
        throw std::runtime_error(
            "cff: non-CID Top DICT missing Private");
    }
    if (top.has_ros
        && (top.fd_array_offset == 0
            || top.fd_select_offset == 0)) {
        throw std::runtime_error(
            "cff: CID Top DICT missing FDArray or FDSelect");
    }

    // CharStrings INDEX — at the absolute offset declared in
    // Top DICT.
    Cursor cs_cur{input,
                  static_cast<std::size_t>(top.charstrings_offset)};
    Index charstrings_idx = ParseIndex(cs_cur);
    const std::uint32_t num_glyphs =
        static_cast<std::uint32_t>(charstrings_idx.entries.size());
    if (num_glyphs == 0) {
        throw std::runtime_error(
            "cff: CharStrings INDEX is empty");
    }

    // Charset (gid → SID/CID).
    std::vector<std::uint32_t> charset_sids;
    if (top.charset_offset == 0
        || top.charset_offset == 1
        || top.charset_offset == 2) {
        // Predefined ISOAdobe / Expert / ExpertSubset
        // charsets — not used by any v1 fixture; emit an
        // explicit error so a future fixture that tries to
        // ride the predefined charset path doesn't silently
        // produce wrong output.
        throw std::runtime_error(
            "cff: predefined charset (0/1/2) not supported in v1");
    } else {
        charset_sids = ParseCharset(
            input,
            static_cast<std::size_t>(top.charset_offset),
            num_glyphs);
    }

    // Per-glyph PrivateDict resolution. Non-CID: one global
    // PrivateDict + local subrs, applied to every glyph.
    // CID: per-FD PrivateDict + local subrs via FDSelect.
    std::vector<FontDict> fd_array;
    std::vector<std::uint8_t> fd_select;

    if (top.has_ros) {
        // Read FDArray (an INDEX of FontDict-DICT bytes).
        Cursor fda_cur{input,
                       static_cast<std::size_t>(top.fd_array_offset)};
        Index fda_idx = ParseIndex(fda_cur);
        if (fda_idx.entries.empty()) {
            throw std::runtime_error(
                "cff: FDArray INDEX is empty");
        }
        for (const auto& fd_bytes : fda_idx.entries) {
            FontDict fd;
            const auto fd_entries = ParseDict(fd_bytes);
            // Each FontDict carries its own Private (size, offset)
            // pair under op 18.
            std::int64_t pd_size = 0;
            std::int64_t pd_offset = 0;
            bool ok = false;
            for (const auto& e : fd_entries) {
                if (e.key == 18 && e.values.size() >= 2) {
                    pd_size = DictAsInt(e.values[0]);
                    pd_offset = DictAsInt(e.values[1]);
                    ok = true;
                }
            }
            if (!ok) {
                throw std::runtime_error(
                    "cff: FontDict missing Private");
            }
            const auto pd_bytes = input.subspan(
                static_cast<std::size_t>(pd_offset),
                static_cast<std::size_t>(pd_size));
            const auto pd_entries = ParseDict(pd_bytes);
            fd.priv = ResolvePrivateDict(pd_entries);

            if (fd.priv.has_subrs) {
                // Subrs offset is RELATIVE to the Private
                // DICT start.
                Cursor sub_cur{input,
                    static_cast<std::size_t>(pd_offset)
                    + static_cast<std::size_t>(
                          fd.priv.local_subrs_offset_rel)};
                Index sub_idx = ParseIndex(sub_cur);
                fd.local_subrs = std::move(sub_idx.entries);
            }
            fd_array.push_back(std::move(fd));
        }
        fd_select = ParseFDSelect(
            input,
            static_cast<std::size_t>(top.fd_select_offset),
            num_glyphs);
    } else {
        FontDict fd;
        const auto pd_bytes = input.subspan(
            static_cast<std::size_t>(top.private_offset),
            static_cast<std::size_t>(top.private_size));
        const auto pd_entries = ParseDict(pd_bytes);
        fd.priv = ResolvePrivateDict(pd_entries);
        if (fd.priv.has_subrs) {
            Cursor sub_cur{input,
                static_cast<std::size_t>(top.private_offset)
                + static_cast<std::size_t>(
                      fd.priv.local_subrs_offset_rel)};
            Index sub_idx = ParseIndex(sub_cur);
            fd.local_subrs = std::move(sub_idx.entries);
        }
        fd_array.push_back(std::move(fd));
        fd_select.assign(num_glyphs, 0);
    }

    // Final Cff value.
    Cff out;
    out.units_per_em = static_cast<std::uint32_t>(
        std::lround(1.0 / top.font_matrix[0]));
    const auto& font_name_bytes = name_idx.entries[0];
    out.font_name.assign(
        reinterpret_cast<const char*>(font_name_bytes.data()),
        font_name_bytes.size());
    out.is_cid = top.has_ros;

    out.glyphs.reserve(num_glyphs);
    for (std::uint32_t gid = 0; gid < num_glyphs; ++gid) {
        Glyph g;
        g.gid = gid;
        g.cid = 0;

        const std::uint8_t fd_idx = fd_select[gid];
        if (fd_idx >= fd_array.size()) {
            throw std::runtime_error(
                "cff: FDSelect points past FDArray");
        }
        const FontDict& fd = fd_array[fd_idx];

        // Resolve label: non-CID uses SID-resolved name;
        // CID uses cid<N>.
        if (out.is_cid) {
            // For CID fonts charset_sids[gid] IS the CID.
            // Glyph 0 is implicit CID 0.
            g.cid = charset_sids[gid];
            g.name.clear();
        } else {
            const std::int64_t sid =
                static_cast<std::int64_t>(charset_sids[gid]);
            g.name = ResolveSid(sid, string_idx);
        }

        // Run the Type 2 program.
        InterpState st;
        RunCharString(charstrings_idx.entries[gid],
                      st,
                      fd.local_subrs,
                      gsubrs_idx.entries,
                      fd.priv,
                      0);
        // ResolveWidth never fires when the program emits no
        // stack-clearing op (e.g. an empty .notdef or an
        // empty cidN program). Use defaultWidthX in that case.
        if (!st.width_resolved) {
            st.advance = RoundCoord(fd.priv.default_width_x);
        }
        g.advance = st.advance;
        g.path = std::move(st.path);
        out.glyphs.push_back(std::move(g));
    }

    return out;
}

// --------------------------------------------------------------------
// ToCanonical
// --------------------------------------------------------------------

namespace {

// %XX-encode bytes outside 0x21..0x7E (and the percent sign).
// Mirrors oracle_fonttools / oracle_freetype byte-for-byte.
std::string EscapeFontName(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (const char c : raw) {
        const auto b = static_cast<std::uint8_t>(c);
        if (b >= 0x21 && b <= 0x7E && b != '%') {
            out.push_back(static_cast<char>(b));
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", b);
            out.append(buf);
        }
    }
    return out;
}

void AppendInt(std::string& s, std::int64_t v) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%lld",
                  static_cast<long long>(v));
    s.append(buf);
}

}  // namespace

std::string ToCanonical(const Cff& font) {
    std::string out;

    out.append("unitsPerEm ");
    AppendInt(out, font.units_per_em);
    out.push_back('\n');

    out.append("isCID ");
    out.push_back(font.is_cid ? '1' : '0');
    out.push_back('\n');

    out.append("fontName ");
    out.append(EscapeFontName(font.font_name));
    out.push_back('\n');

    out.append("numGlyphs ");
    AppendInt(out, font.glyphs.size());
    out.push_back('\n');

    for (const auto& g : font.glyphs) {
        out.append("glyph ");
        AppendInt(out, g.gid);
        out.push_back(' ');
        if (font.is_cid) {
            out.append("cid");
            AppendInt(out, g.cid);
        } else {
            out.append(g.name);
        }
        out.push_back(' ');
        AppendInt(out, g.advance);
        out.push_back('\n');

        for (const auto& seg : g.path) {
            out.append("  ");
            switch (seg.op) {
                case PathOp::M: {
                    out.push_back('M');
                    out.push_back(' ');
                    AppendInt(out, seg.coords[0]);
                    out.push_back(' ');
                    AppendInt(out, seg.coords[1]);
                    break;
                }
                case PathOp::L: {
                    out.push_back('L');
                    out.push_back(' ');
                    AppendInt(out, seg.coords[0]);
                    out.push_back(' ');
                    AppendInt(out, seg.coords[1]);
                    break;
                }
                case PathOp::C: {
                    out.push_back('C');
                    for (std::size_t i = 0;
                         i < 6; ++i) {
                        out.push_back(' ');
                        AppendInt(out, seg.coords[i]);
                    }
                    break;
                }
                case PathOp::Z: {
                    out.push_back('Z');
                    break;
                }
            }
            out.push_back('\n');
        }
    }

    return out;
}

}  // namespace foundation::cff
