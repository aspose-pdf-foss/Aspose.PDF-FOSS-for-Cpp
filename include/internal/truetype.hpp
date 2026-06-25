#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::truetype {

struct CmapEntry {
    std::uint32_t char_code;
    std::uint32_t glyph_id;
};

// One point of a glyph contour. Coordinates are in font design
// units (FUnits) — the renderer scales by units_per_em / size.
// `on_curve` is the TrueType bit-0 flag: true = on-curve
// (anchor) point, false = off-curve (control) point.
struct GlyphPoint {
    std::int32_t x;
    std::int32_t y;
    bool on_curve;
};

// One parsed glyph entry. Composites are flattened at parse
// time, so every Glyph here is logically a "simple" glyph from
// the consumer's point of view: a flat point list plus the
// inclusive last-point index of each contour. Empty glyphs
// (TrueType notdef-padding entries common at high gids) carry
// empty `points` and `contour_ends`.
struct Glyph {
    std::vector<GlyphPoint> points;
    std::vector<std::uint32_t> contour_ends;  // inclusive, in order
};

struct TrueType {
    std::uint32_t units_per_em;
    std::int32_t ascent;
    std::int32_t descent;
    std::uint32_t num_glyphs;
    std::vector<std::uint32_t> glyph_widths;
    std::vector<CmapEntry> cmap;
    std::vector<Glyph> glyphs;  // one per gid, 0..num_glyphs-1
};

TrueType Parse(std::span<const std::byte> input);
std::string ToCanonical(const TrueType& font);

}
