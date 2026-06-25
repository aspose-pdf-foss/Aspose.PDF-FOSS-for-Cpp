#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace foundation::cff {

// Type 2 CharString programs draw cubic-only paths. Path
// segments are decomposed into one of four kinds:
//   M  moveto    coords = {x, y}
//   L  lineto    coords = {x, y}
//   C  curveto   coords = {x1, y1, x2, y2, x3, y3}
//   Z  closepath coords = {}
enum class PathOp : std::uint8_t {
    M = 0,
    L = 1,
    C = 2,
    Z = 3,
};

// Path coordinates are absolute; the Type 2 interpreter
// resolves Type 2's delta-style operands against a running
// cursor before emitting a segment.
struct PathSegment {
    PathOp op;
    std::vector<std::int32_t> coords;
};

// One parsed glyph entry.
//
//   gid     0..num_glyphs-1.
//   name    Non-CID fonts: glyph name from charset (e.g. "A",
//           ".notdef"). CID fonts: empty string — see `cid`.
//   cid     CID-keyed fonts: the glyph's CID. 0 for non-CID.
//   advance Resolved advance width in font units (after
//           applying default/nominalWidthX from the Private
//           DICT that owns this glyph; for CID fonts the
//           per-FontDict Private DICT is consulted via
//           FDSelect).
//   path    Path-segment list produced by executing the
//           Type 2 program for this glyph.
struct Glyph {
    std::uint32_t gid;
    std::string name;
    std::uint32_t cid;
    std::int32_t advance;
    std::vector<PathSegment> path;
};

// Parsed CFF font.
//
//   units_per_em  round(1.0 / FontMatrix[0]) — 1000 for the
//                 default Adobe FontMatrix.
//   font_name     PostScript name from Name INDEX entry 0
//                 (raw bytes; caller decodes if needed).
//   is_cid        true iff Top DICT carries ROS (CID-keyed
//                 font with FDArray + FDSelect).
//   glyphs        One entry per glyph in glyph-id order.
struct Cff {
    std::uint32_t units_per_em;
    std::string font_name;
    bool is_cid;
    std::vector<Glyph> glyphs;
};

// Parse a raw CFF byte stream (the contents of an OpenType
// /CFF table or a PDF /Type1C / /CIDFontType0C font program).
// Throws std::runtime_error on the conditions listed in the
// spec anchor (cff.yaml §error model).
Cff Parse(std::span<const std::byte> input);

// Freeze-gate canonical form — emit text matching the
// .canonical sidecars produced by oracle_fonttools and
// oracle_freetype. See the anchor's ToCanonical contract.
std::string ToCanonical(const Cff& font);

}
