// TrueType foundation primitive.
//
// Parses the sfnt/cmap tables for glyph mapping. Struct layout
// (TrueType / CmapEntry) and Parse/ToCanonical signatures are stable.
//
// Scope (per the project spec):
//   * sfnt_version 0x00010000 only; `OTTO`, `true`, `ttcf` are
//     rejected.
//   * Reads head, hhea, maxp, hmtx, cmap. Other tables ignored.
//   * cmap format 4 only, BMP subtable selection: prefer
//     (platform 3, encoding 1), fall back to (0, 3).
//   * hmtx: first numberOfHMetrics records carry per-glyph
//     advanceWidth; remaining glyphs share the last explicit
//     advance.

#include "truetype.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::truetype {

namespace {

// Big-endian integer reads. Every SFNT field is BE per the
// TrueType / OpenType specs.
std::uint16_t ReadU16(std::span<const std::byte> data,
                      std::size_t off) {
    if (off + 2 > data.size()) {
        throw std::runtime_error("truetype: truncated u16");
    }
    const auto a = static_cast<std::uint8_t>(data[off]);
    const auto b = static_cast<std::uint8_t>(data[off + 1]);
    return static_cast<std::uint16_t>((a << 8) | b);
}

std::int16_t ReadI16(std::span<const std::byte> data,
                     std::size_t off) {
    // Signed interpretation of the same two big-endian bytes.
    return static_cast<std::int16_t>(ReadU16(data, off));
}

std::uint8_t ReadU8(std::span<const std::byte> data,
                    std::size_t off) {
    if (off + 1 > data.size()) {
        throw std::runtime_error("truetype: truncated u8");
    }
    return static_cast<std::uint8_t>(data[off]);
}

std::int8_t ReadI8(std::span<const std::byte> data,
                   std::size_t off) {
    return static_cast<std::int8_t>(ReadU8(data, off));
}

// F2DOT14: int16 / 16384.0 — 2-bit signed integer, 14-bit
// fraction. Used by composite-component 2x2 transforms.
double ReadF2DOT14(std::span<const std::byte> data,
                   std::size_t off) {
    return static_cast<double>(ReadI16(data, off)) / 16384.0;
}

std::uint32_t ReadU32(std::span<const std::byte> data,
                      std::size_t off) {
    if (off + 4 > data.size()) {
        throw std::runtime_error("truetype: truncated u32");
    }
    const auto a = static_cast<std::uint8_t>(data[off]);
    const auto b = static_cast<std::uint8_t>(data[off + 1]);
    const auto c = static_cast<std::uint8_t>(data[off + 2]);
    const auto d = static_cast<std::uint8_t>(data[off + 3]);
    return (static_cast<std::uint32_t>(a) << 24)
         | (static_cast<std::uint32_t>(b) << 16)
         | (static_cast<std::uint32_t>(c) << 8)
         | (static_cast<std::uint32_t>(d));
}

// Table directory entry as it lives in the SFNT. We only keep
// tag + offset + length; the checksum field is ignored.
struct TableLoc {
    std::uint32_t tag;
    std::uint32_t offset;
    std::uint32_t length;
};

// Pack four ASCII characters into a 32-bit big-endian tag value.
// Matches how ReadU32 reads the table directory's tag field, so
// comparisons can be done with a single integer equality.
constexpr std::uint32_t Tag(char a, char b, char c, char d) {
    return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) << 24)
         | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 16)
         | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 8)
         | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(d)));
}

std::vector<TableLoc> ReadTableDirectory(
        std::span<const std::byte> input) {
    if (input.size() < 12) {
        throw std::runtime_error("truetype: offset table truncated");
    }
    const std::uint32_t sfnt_version = ReadU32(input, 0);
    if (sfnt_version != 0x00010000u) {
        throw std::runtime_error(
            "truetype: unsupported sfnt_version (v1 is TrueType only)");
    }
    const std::uint16_t num_tables = ReadU16(input, 4);
    const std::size_t dir_bytes =
        12 + static_cast<std::size_t>(num_tables) * 16;
    if (input.size() < dir_bytes) {
        throw std::runtime_error("truetype: table directory truncated");
    }
    std::vector<TableLoc> out;
    out.reserve(num_tables);
    for (std::size_t i = 0; i < num_tables; ++i) {
        const std::size_t base = 12 + i * 16;
        TableLoc loc;
        loc.tag    = ReadU32(input, base + 0);
        loc.offset = ReadU32(input, base + 8);
        loc.length = ReadU32(input, base + 12);
        out.push_back(loc);
    }
    return out;
}

std::span<const std::byte> GetTable(
        std::span<const std::byte> input,
        const std::vector<TableLoc>& dir,
        std::uint32_t tag) {
    for (const auto& loc : dir) {
        if (loc.tag != tag) continue;
        if (static_cast<std::size_t>(loc.offset) + loc.length
                > input.size()) {
            throw std::runtime_error(
                "truetype: table extends past input");
        }
        return input.subspan(loc.offset, loc.length);
    }
    throw std::runtime_error("truetype: required table missing");
}

void ParseHmtx(std::span<const std::byte> hmtx,
               std::uint16_t num_hmetrics,
               std::uint16_t num_glyphs,
               std::vector<std::uint32_t>& widths) {
    if (num_hmetrics == 0) {
        throw std::runtime_error(
            "truetype: numberOfHMetrics is zero (malformed hhea)");
    }
    if (num_hmetrics > num_glyphs) {
        throw std::runtime_error(
            "truetype: numberOfHMetrics > numGlyphs");
    }
    // num_hmetrics 4-byte records + (num_glyphs - num_hmetrics)
    // trailing 2-byte lsb slots. We don't expose lsb in v1 but a
    // truncated hmtx still indicates a broken font.
    const std::size_t needed =
        static_cast<std::size_t>(num_hmetrics) * 4
      + static_cast<std::size_t>(num_glyphs - num_hmetrics) * 2;
    if (hmtx.size() < needed) {
        throw std::runtime_error("truetype: hmtx truncated");
    }

    widths.resize(num_glyphs);
    for (std::uint16_t i = 0; i < num_hmetrics; ++i) {
        widths[i] = ReadU16(hmtx, static_cast<std::size_t>(i) * 4);
    }
    const std::uint32_t last = widths[num_hmetrics - 1];
    for (std::uint16_t i = num_hmetrics; i < num_glyphs; ++i) {
        widths[i] = last;
    }
}

void ParseCmapFormat4(std::span<const std::byte> sub,
                      std::vector<CmapEntry>& out) {
    // sub starts at the subtable's `format` field. Fixed header
    // is 14 bytes: format, length, language, segCountX2,
    // searchRange, entrySelector, rangeShift. Then four parallel
    // arrays of segCount each (with a reservedPad uint16 between
    // endCode and startCode), then glyphIdArray.
    if (sub.size() < 14) {
        throw std::runtime_error(
            "truetype: cmap format-4 subtable too small");
    }
    const std::uint16_t format = ReadU16(sub, 0);
    if (format != 4) {
        throw std::runtime_error(
            "truetype: expected cmap subtable format 4");
    }
    const std::uint16_t length = ReadU16(sub, 2);
    if (length < 14 || length > sub.size()) {
        throw std::runtime_error(
            "truetype: cmap format-4 length out of bounds");
    }
    const std::uint16_t seg_x2 = ReadU16(sub, 6);
    if (seg_x2 == 0 || (seg_x2 & 1) != 0) {
        throw std::runtime_error(
            "truetype: cmap format-4 segCountX2 invalid");
    }
    const std::uint16_t seg_count =
        static_cast<std::uint16_t>(seg_x2 / 2);

    // Layout (byte offsets from subtable start):
    //   endCode       at 14
    //   reservedPad   at 14 + 2*seg_count
    //   startCode     at 14 + 2 + 2*seg_count
    //   idDelta       at 14 + 2 + 4*seg_count
    //   idRangeOffset at 14 + 2 + 6*seg_count
    //   glyphIdArray  at 14 + 2 + 8*seg_count (rest of length)
    const std::size_t end_code_off   = 14;
    const std::size_t start_code_off = end_code_off   + 2 + 2 * seg_count;
    const std::size_t id_delta_off   = start_code_off + 2 * seg_count;
    const std::size_t id_ro_off      = id_delta_off   + 2 * seg_count;
    const std::size_t glyph_arr_off  = id_ro_off      + 2 * seg_count;

    if (glyph_arr_off > length) {
        throw std::runtime_error(
            "truetype: cmap format-4 parallel arrays overflow length");
    }

    for (std::uint16_t i = 0; i < seg_count; ++i) {
        const std::uint16_t end   = ReadU16(sub, end_code_off   + 2 * i);
        const std::uint16_t start = ReadU16(sub, start_code_off + 2 * i);
        const std::int16_t  delta = ReadI16(sub, id_delta_off   + 2 * i);
        const std::uint16_t ro    = ReadU16(sub, id_ro_off      + 2 * i);
        if (end < start) {
            throw std::runtime_error(
                "truetype: cmap format-4 reversed segment");
        }
        for (std::uint32_t c = start; c <= end; ++c) {
            std::uint16_t gid;
            if (ro == 0) {
                gid = static_cast<std::uint16_t>(
                    (c + static_cast<std::uint32_t>(delta)) & 0xFFFFu);
            } else {
                // The glyphIdArray slot is at
                // `&idRangeOffset[i] + ro + 2*(c - start)` bytes.
                // In absolute subtable offsets that's
                // `id_ro_off + 2*i + ro + 2*(c - start)`.
                const std::size_t slot_off =
                    id_ro_off + 2 * static_cast<std::size_t>(i)
                    + static_cast<std::size_t>(ro)
                    + 2 * static_cast<std::size_t>(c - start);
                if (slot_off + 2 > length) {
                    throw std::runtime_error(
                        "truetype: cmap format-4 idRangeOffset "
                        "walk out of range");
                }
                const std::uint16_t raw = ReadU16(sub, slot_off);
                if (raw == 0) {
                    gid = 0;
                } else {
                    gid = static_cast<std::uint16_t>(
                        (static_cast<std::uint32_t>(raw)
                           + static_cast<std::uint32_t>(delta))
                        & 0xFFFFu);
                }
            }
            if (gid != 0) {
                out.push_back(CmapEntry{c, gid});
            }
        }
    }
}

// Pick the BMP subtable from the cmap table: prefer
// (platform 3, encoding 1), fall back to (0, 3). Returns a
// span starting at the chosen subtable's `format` field.
std::span<const std::byte> SelectBmpSubtable(
        std::span<const std::byte> cmap) {
    if (cmap.size() < 4) {
        throw std::runtime_error("truetype: cmap table truncated");
    }
    const std::uint16_t num_sub = ReadU16(cmap, 2);
    const std::size_t dir_bytes =
        4 + static_cast<std::size_t>(num_sub) * 8;
    if (cmap.size() < dir_bytes) {
        throw std::runtime_error(
            "truetype: cmap subtable directory truncated");
    }

    std::uint32_t preferred = 0;
    std::uint32_t fallback = 0;
    bool have_preferred = false;
    bool have_fallback = false;
    for (std::uint16_t i = 0; i < num_sub; ++i) {
        const std::size_t rec = 4 + static_cast<std::size_t>(i) * 8;
        const std::uint16_t platform = ReadU16(cmap, rec);
        const std::uint16_t encoding = ReadU16(cmap, rec + 2);
        // Subtable offset is relative to start of the cmap TABLE.
        const std::uint32_t off = ReadU32(cmap, rec + 4);
        if (platform == 3 && encoding == 1) {
            preferred = off;
            have_preferred = true;
        } else if (platform == 0 && encoding == 3) {
            fallback = off;
            have_fallback = true;
        }
    }
    if (!have_preferred && !have_fallback) {
        throw std::runtime_error(
            "truetype: no format-4 Unicode BMP cmap subtable");
    }
    const std::uint32_t chosen = have_preferred ? preferred : fallback;
    if (static_cast<std::size_t>(chosen) + 4 > cmap.size()) {
        throw std::runtime_error(
            "truetype: cmap subtable offset out of range");
    }
    return cmap.subspan(chosen);
}

// glyf flag bits (simple glyphs).
constexpr std::uint8_t kFlagOnCurve     = 0x01;
constexpr std::uint8_t kFlagXShort      = 0x02;
constexpr std::uint8_t kFlagYShort      = 0x04;
constexpr std::uint8_t kFlagRepeat      = 0x08;
constexpr std::uint8_t kFlagXSameOrPos  = 0x10;
constexpr std::uint8_t kFlagYSameOrPos  = 0x20;

// Composite component flags.
constexpr std::uint16_t kCompArgsAreWords     = 0x0001;
constexpr std::uint16_t kCompArgsAreXY        = 0x0002;
constexpr std::uint16_t kCompHaveScale        = 0x0008;
constexpr std::uint16_t kCompMoreComponents   = 0x0020;
constexpr std::uint16_t kCompHaveXYScale      = 0x0040;
constexpr std::uint16_t kCompHaveTwoByTwo     = 0x0080;
constexpr std::uint16_t kCompHaveInstructions = 0x0100;

constexpr int kMaxCompositeDepth = 10;

std::vector<std::uint32_t> ParseLoca(
        std::span<const std::byte> loca,
        std::uint16_t num_glyphs,
        std::int16_t index_to_loc_format) {
    if (index_to_loc_format != 0 && index_to_loc_format != 1) {
        throw std::runtime_error(
            "truetype: head.indexToLocFormat must be 0 or 1");
    }
    const std::size_t entries =
        static_cast<std::size_t>(num_glyphs) + 1;
    const std::size_t needed =
        index_to_loc_format == 0 ? entries * 2 : entries * 4;
    if (loca.size() < needed) {
        throw std::runtime_error("truetype: loca truncated");
    }
    std::vector<std::uint32_t> out;
    out.reserve(entries);
    if (index_to_loc_format == 0) {
        for (std::size_t i = 0; i < entries; ++i) {
            out.push_back(
                static_cast<std::uint32_t>(ReadU16(loca, i * 2))
                * 2u);
        }
    } else {
        for (std::size_t i = 0; i < entries; ++i) {
            out.push_back(ReadU32(loca, i * 4));
        }
    }
    for (std::size_t i = 1; i < out.size(); ++i) {
        if (out[i] < out[i - 1]) {
            throw std::runtime_error(
                "truetype: loca offsets descend");
        }
    }
    return out;
}

// Decode the variable-length flag stream for a simple glyph.
// `off` is the offset into `slot` where the flags begin; on
// return it points one past the last flag byte consumed.
// The expanded flags vector has length num_points (after
// REPEAT-bit expansion).
std::vector<std::uint8_t> DecodeGlyphFlags(
        std::span<const std::byte> slot,
        std::size_t& off,
        std::uint32_t num_points) {
    std::vector<std::uint8_t> flags;
    flags.reserve(num_points);
    while (flags.size() < num_points) {
        const std::uint8_t f = ReadU8(slot, off);
        ++off;
        flags.push_back(f);
        if (f & kFlagRepeat) {
            const std::uint8_t rep = ReadU8(slot, off);
            ++off;
            if (flags.size() + rep > num_points) {
                throw std::runtime_error(
                    "truetype: glyf flag REPEAT overflows "
                    "numPoints");
            }
            for (std::uint8_t r = 0; r < rep; ++r) {
                flags.push_back(f);
            }
        }
    }
    return flags;
}

// Walk an axis (x or y) of coordinate deltas per the per-flag
// SHORT / SAME / SIGN bit pair. `short_bit` is the SHORT bit
// for this axis (0x02 or 0x04); `same_or_pos_bit` is the
// SAME-OR-POSITIVE bit (0x10 or 0x20). Coordinates are
// running-summed deltas; the absolute axis values are written
// into `axis_out` (length == flags.size()).
void DecodeGlyphAxis(std::span<const std::byte> slot,
                     std::size_t& off,
                     const std::vector<std::uint8_t>& flags,
                     std::uint8_t short_bit,
                     std::uint8_t same_or_pos_bit,
                     std::vector<std::int32_t>& axis_out) {
    axis_out.resize(flags.size());
    std::int32_t cur = 0;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        const std::uint8_t f = flags[i];
        std::int32_t delta = 0;
        if (f & short_bit) {
            const std::uint8_t v = ReadU8(slot, off);
            ++off;
            delta = (f & same_or_pos_bit) ? v : -v;
        } else {
            if (f & same_or_pos_bit) {
                delta = 0;  // repeat previous
            } else {
                delta = ReadI16(slot, off);
                off += 2;
            }
        }
        cur += delta;
        axis_out[i] = cur;
    }
}

// Forward declaration — composite expansion recurses into
// ExpandGlyph for the referenced component.
Glyph ExpandGlyph(std::span<const std::byte> input,
                  std::span<const std::byte> glyf,
                  const std::vector<std::uint32_t>& loca,
                  std::uint16_t num_glyphs,
                  std::uint32_t gid,
                  int depth);

// Parse a simple glyph slot. `slot` is the substring
// loca[gid]..loca[gid+1] taken from the glyf table. Header
// (numContours int16 + bbox 4×int16) has already been
// consumed by the caller; `off_in_slot` points at the
// endPtsOfContours array.
Glyph ParseSimpleGlyph(std::span<const std::byte> slot,
                       std::int16_t num_contours,
                       std::size_t off_in_slot) {
    Glyph g;
    if (num_contours == 0) {
        return g;  // empty (header but no contours)
    }
    g.contour_ends.reserve(num_contours);
    for (std::int16_t i = 0; i < num_contours; ++i) {
        g.contour_ends.push_back(
            ReadU16(slot, off_in_slot + 2 * i));
    }
    off_in_slot += 2 * static_cast<std::size_t>(num_contours);

    const std::uint16_t instr_len = ReadU16(slot, off_in_slot);
    off_in_slot += 2;
    off_in_slot += instr_len;
    if (off_in_slot > slot.size()) {
        throw std::runtime_error(
            "truetype: glyf instruction stream exceeds slot");
    }

    const std::uint32_t num_points =
        g.contour_ends.empty()
            ? 0u
            : (g.contour_ends.back() + 1u);
    if (num_points == 0) {
        return g;  // contours present but zero points (corrupt)
    }

    const auto flags = DecodeGlyphFlags(slot, off_in_slot,
                                         num_points);
    std::vector<std::int32_t> xs, ys;
    DecodeGlyphAxis(slot, off_in_slot, flags,
                    kFlagXShort, kFlagXSameOrPos, xs);
    DecodeGlyphAxis(slot, off_in_slot, flags,
                    kFlagYShort, kFlagYSameOrPos, ys);

    g.points.reserve(num_points);
    for (std::size_t i = 0; i < num_points; ++i) {
        g.points.push_back(
            GlyphPoint{xs[i], ys[i],
                       (flags[i] & kFlagOnCurve) != 0});
    }
    return g;
}

// Expand a composite glyph: walk components, recurse into
// each referenced gid, apply the component's affine transform
// + offset to the recursed point list, append to the parent.
// Contour-end indices are renumbered into the parent's point
// space.
Glyph ParseCompositeGlyph(std::span<const std::byte> input,
                          std::span<const std::byte> glyf,
                          const std::vector<std::uint32_t>& loca,
                          std::uint16_t num_glyphs,
                          std::span<const std::byte> slot,
                          std::size_t off_in_slot,
                          int depth) {
    Glyph parent;
    bool more = true;
    while (more) {
        const std::uint16_t flags =
            ReadU16(slot, off_in_slot);
        off_in_slot += 2;
        const std::uint16_t comp_gid =
            ReadU16(slot, off_in_slot);
        off_in_slot += 2;
        if (comp_gid >= num_glyphs) {
            throw std::runtime_error(
                "truetype: composite component gid out of range");
        }

        // Args 1, 2 — interpretation per ARGS_ARE_WORDS +
        // ARGS_ARE_XY_VALUES.
        std::int32_t arg1 = 0;
        std::int32_t arg2 = 0;
        if (flags & kCompArgsAreWords) {
            if (flags & kCompArgsAreXY) {
                arg1 = ReadI16(slot, off_in_slot);
                arg2 = ReadI16(slot, off_in_slot + 2);
            } else {
                arg1 = ReadU16(slot, off_in_slot);
                arg2 = ReadU16(slot, off_in_slot + 2);
            }
            off_in_slot += 4;
        } else {
            if (flags & kCompArgsAreXY) {
                arg1 = ReadI8(slot, off_in_slot);
                arg2 = ReadI8(slot, off_in_slot + 1);
            } else {
                arg1 = ReadU8(slot, off_in_slot);
                arg2 = ReadU8(slot, off_in_slot + 1);
            }
            off_in_slot += 2;
        }

        // Affine transform per WE_HAVE_A_SCALE / X_AND_Y_SCALE
        // / TWO_BY_TWO. Default identity.
        double a = 1.0, b = 0.0, c = 0.0, d = 1.0;
        if (flags & kCompHaveScale) {
            const double s = ReadF2DOT14(slot, off_in_slot);
            off_in_slot += 2;
            a = s; d = s;
        } else if (flags & kCompHaveXYScale) {
            a = ReadF2DOT14(slot, off_in_slot);
            d = ReadF2DOT14(slot, off_in_slot + 2);
            off_in_slot += 4;
        } else if (flags & kCompHaveTwoByTwo) {
            a = ReadF2DOT14(slot, off_in_slot);
            b = ReadF2DOT14(slot, off_in_slot + 2);
            c = ReadF2DOT14(slot, off_in_slot + 4);
            d = ReadF2DOT14(slot, off_in_slot + 6);
            off_in_slot += 8;
        }

        // ARGS_ARE_XY: arg1/arg2 are an xy offset directly.
        // !ARGS_ARE_XY: anchor-point matching — out of v1
        // scope; treat as zero offset.
        const double dx = (flags & kCompArgsAreXY)
            ? static_cast<double>(arg1) : 0.0;
        const double dy = (flags & kCompArgsAreXY)
            ? static_cast<double>(arg2) : 0.0;

        Glyph sub = ExpandGlyph(input, glyf, loca, num_glyphs,
                                 comp_gid, depth + 1);
        const std::uint32_t base_pt =
            static_cast<std::uint32_t>(parent.points.size());
        for (const auto& p : sub.points) {
            const double x =
                a * p.x + b * p.y + dx;
            const double y =
                c * p.x + d * p.y + dy;
            parent.points.push_back(GlyphPoint{
                static_cast<std::int32_t>(std::lround(x)),
                static_cast<std::int32_t>(std::lround(y)),
                p.on_curve});
        }
        for (auto e : sub.contour_ends) {
            parent.contour_ends.push_back(e + base_pt);
        }

        more = (flags & kCompMoreComponents) != 0;

        // After last component, optional WE_HAVE_INSTRUCTIONS
        // adds a uint16 numInstr + numInstr × uint8.
        if (!more && (flags & kCompHaveInstructions)) {
            const std::uint16_t n =
                ReadU16(slot, off_in_slot);
            off_in_slot += 2 + n;
            if (off_in_slot > slot.size()) {
                throw std::runtime_error(
                    "truetype: composite instructions exceed "
                    "slot");
            }
        }
    }
    return parent;
}

Glyph ExpandGlyph(std::span<const std::byte> input,
                  std::span<const std::byte> glyf,
                  const std::vector<std::uint32_t>& loca,
                  std::uint16_t num_glyphs,
                  std::uint32_t gid,
                  int depth) {
    if (depth > kMaxCompositeDepth) {
        throw std::runtime_error(
            "truetype: composite glyph recursion depth exceeded");
    }
    const std::uint32_t start = loca[gid];
    const std::uint32_t end   = loca[gid + 1];
    if (end == start) {
        return Glyph{};  // empty glyph
    }
    if (end < start || end > glyf.size()) {
        throw std::runtime_error(
            "truetype: glyf slot out of range");
    }
    const auto slot = glyf.subspan(start, end - start);
    if (slot.size() < 10) {
        throw std::runtime_error(
            "truetype: glyf slot truncated below header");
    }
    const std::int16_t num_contours = ReadI16(slot, 0);
    // header = numContours + xMin + yMin + xMax + yMax = 10 B
    if (num_contours >= 0) {
        return ParseSimpleGlyph(slot, num_contours, 10);
    }
    if (num_contours == -1) {
        return ParseCompositeGlyph(input, glyf, loca,
                                    num_glyphs, slot, 10, depth);
    }
    throw std::runtime_error(
        "truetype: glyf numContours not in {-1} ∪ [0, 32767]");
}

}  // namespace

TrueType Parse(std::span<const std::byte> input) {
    const auto dir = ReadTableDirectory(input);

    const auto head = GetTable(input, dir, Tag('h','e','a','d'));
    if (head.size() < 52) {
        throw std::runtime_error("truetype: head truncated");
    }
    const std::uint16_t units_per_em = ReadU16(head, 18);
    const std::int16_t  index_to_loc = ReadI16(head, 50);

    const auto hhea = GetTable(input, dir, Tag('h','h','e','a'));
    if (hhea.size() < 36) {
        throw std::runtime_error("truetype: hhea truncated");
    }
    const std::int16_t  ascent       = ReadI16(hhea, 4);
    const std::int16_t  descent      = ReadI16(hhea, 6);
    const std::uint16_t num_hmetrics = ReadU16(hhea, 34);

    const auto maxp = GetTable(input, dir, Tag('m','a','x','p'));
    if (maxp.size() < 6) {
        throw std::runtime_error("truetype: maxp truncated");
    }
    const std::uint16_t num_glyphs = ReadU16(maxp, 4);

    TrueType out;
    out.units_per_em = units_per_em;
    out.ascent       = ascent;
    out.descent      = descent;
    out.num_glyphs   = num_glyphs;

    const auto hmtx = GetTable(input, dir, Tag('h','m','t','x'));
    ParseHmtx(hmtx, num_hmetrics, num_glyphs, out.glyph_widths);

    // cmap is OPTIONAL. Simple fonts map character codes → glyphs
    // through a Unicode BMP format-4 subtable, but CID-keyed fonts
    // (CIDFontType2, the descendant of a Type0) carry no usable
    // cmap at all — their glyphs are reached by GID via the PDF
    // /CIDToGIDMap, never by character code. A missing cmap table,
    // or a cmap with no Unicode BMP format-4 subtable, therefore
    // leaves out.cmap empty rather than failing the whole parse;
    // callers that need code→glyph mapping see an empty table and
    // fall back to .notdef, while GID-addressed callers are
    // unaffected. Fonts that DO carry a format-4 BMP subtable parse
    // exactly as before.
    try {
        const auto cmap = GetTable(input, dir, Tag('c','m','a','p'));
        const auto sub  = SelectBmpSubtable(cmap);
        ParseCmapFormat4(sub, out.cmap);
    } catch (const std::runtime_error&) {
        out.cmap.clear();
    }

    const auto loca = GetTable(input, dir, Tag('l','o','c','a'));
    const auto glyf = GetTable(input, dir, Tag('g','l','y','f'));
    const auto loca_offsets =
        ParseLoca(loca, num_glyphs, index_to_loc);
    out.glyphs.reserve(num_glyphs);
    for (std::uint32_t gid = 0; gid < num_glyphs; ++gid) {
        out.glyphs.push_back(ExpandGlyph(input, glyf,
                                          loca_offsets, num_glyphs,
                                          gid, 0));
    }

    return out;
}

std::string ToCanonical(const TrueType& font) {
    std::string s;
    s.reserve(64 + font.glyph_widths.size() * 16
              + font.cmap.size() * 16);

    char buf[64];

    std::snprintf(buf, sizeof(buf),
                  "unitsPerEm %u\n", font.units_per_em);
    s += buf;
    std::snprintf(buf, sizeof(buf),
                  "ascent %d\n", font.ascent);
    s += buf;
    std::snprintf(buf, sizeof(buf),
                  "descent %d\n", font.descent);
    s += buf;
    std::snprintf(buf, sizeof(buf),
                  "numGlyphs %u\n", font.num_glyphs);
    s += buf;
    for (std::size_t i = 0; i < font.glyph_widths.size(); ++i) {
        std::snprintf(buf, sizeof(buf),
                      "width %zu %u\n", i, font.glyph_widths[i]);
        s += buf;
    }
    for (const auto& e : font.cmap) {
        std::snprintf(buf, sizeof(buf),
                      "cmap %04X %u\n", e.char_code, e.glyph_id);
        s += buf;
    }
    for (std::size_t gid = 0; gid < font.glyphs.size(); ++gid) {
        const auto& g = font.glyphs[gid];
        if (g.points.empty()) {
            std::snprintf(buf, sizeof(buf),
                          "glyph %zu empty\n", gid);
            s += buf;
            continue;
        }
        std::snprintf(buf, sizeof(buf),
                      "glyph %zu simple %zu %zu\n",
                      gid, g.contour_ends.size(), g.points.size());
        s += buf;
        for (const auto& p : g.points) {
            std::snprintf(buf, sizeof(buf),
                          "  %s %d %d\n",
                          p.on_curve ? "on" : "off",
                          p.x, p.y);
            s += buf;
        }
        for (auto e : g.contour_ends) {
            std::snprintf(buf, sizeof(buf),
                          "  contour_end %u\n", e);
            s += buf;
        }
    }
    return s;
}

}  // namespace foundation::truetype
