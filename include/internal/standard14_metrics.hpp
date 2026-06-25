#pragma once

// Foundation primitive: Standard-14 font metrics.
//
// PDF 32000-1:2008 mandates that a conforming reader MUST have
// metrics for the 14 Adobe Standard fonts (Annex D.1). These
// fonts are never embedded — a PDF that references them carries
// only the font dictionary, not the glyph data, and the reader
// is expected to supply widths + descriptor metrics.
//
// This primitive is a pure lookup: given a canonical Standard-14
// font name, return its FontDescriptor and per-glyph widths. No
// parsing, no allocation, constexpr storage. The underlying data
// is Adobe's Core14_AFMs corpus, exported via pdfminer.six (see
// generate_data.py in this directory). Data origin: Adobe AFM
// files, which Adobe released with an unrestricted redistribution
// license for these specific 14 fonts.
//
// Scope limit: widths are keyed by Unicode CHARACTER
// (UTF-8 encoded as a short std::string_view — one or more
// bytes per codepoint), not by PDF byte code. Callers must
// translate text-op bytes through the font's /Encoding into
// AGL glyph names first, then resolve glyph names to Unicode
// via an AGL map. Both steps live in separate foundation
// primitives (future: `encoding_tables`, `agl_map`).
//
// Not in scope:
//   * Kerning pairs (very rarely used with Standard-14 in
//     modern PDFs; AFMs do carry them, but v1 ignores them).
//   * Ligature tables (same reasoning).
//   * Non-Standard-14 fonts — embedded TrueType/Type1 metrics
//     are covered by their own primitives (truetype_parser,
//     cff_parser) that parse the embedded font program directly.

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace foundation::standard14_metrics {

// Font-level metrics as defined in the AFM, in the canonical
// 1000-unit em-square. All integer — the AFM values are
// already integer apart from italic_angle, which is scaled by
// 10 so we can carry it as int without losing precision
// (e.g. -12.0° becomes -120).
//
// Symbol and ZapfDingbats are PostScript symbol fonts; their
// AFMs do not declare ascent / descent / cap_height / x_height,
// so those fields are 0 for those two. bbox remains populated.
struct FontDescriptor {
    int ascent;
    int descent;
    int cap_height;
    int x_height;
    int italic_angle_tenths;
    std::array<int, 4> bbox;   // x0, y0, x1, y1
    bool is_fixed_pitch;
};

// Returns a pointer to the descriptor for `font_name`, or
// nullptr if the name is not one of the 14 Standard fonts.
// Name matching is case-sensitive and must use Adobe's canonical
// form ("Times-Roman", not "TimesRoman" or "times-roman").
const FontDescriptor* GetDescriptor(std::string_view font_name) noexcept;

// Returns the advance width of `utf8_char` in `font_name`, in
// 1000-unit em-square units. `utf8_char` is a single Unicode
// codepoint, UTF-8 encoded (1–4 bytes). Returns 0 when the
// font is unknown OR the character is absent from the font's
// AFM (e.g. asking for a Cyrillic character in Helvetica, or
// for a letter in ZapfDingbats).
//
// 0 is the correct "not present" signal here — a width of zero
// is illegal for any real glyph per PDF 32000 §9.2.4, so callers
// can test for it without ambiguity.
int GetCharWidth(std::string_view font_name,
                 std::string_view utf8_char) noexcept;

// Canonical Standard-14 names as Adobe spells them. Use when
// enumerating fonts (e.g. for diagnostic output). The span is
// stable storage; callers may hold the string_views indefinitely.
std::span<const std::string_view> KnownFontNames() noexcept;

}  // namespace foundation::standard14_metrics
