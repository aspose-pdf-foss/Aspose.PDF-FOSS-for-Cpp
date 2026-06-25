#pragma once

// Foundation primitive: PDF standard encodings (byte → PostScript glyph name).
//
// PDF 32000-1:2008 §9.6.6.4 + Annex D define a small set of
// "standard" encodings — fixed 256-slot maps from byte values
// to PostScript glyph names. A non-/ToUnicode font selects one
// via its /Encoding entry (or inherits the implicit default
// for its font /Subtype) and may further override individual
// slots via a /Differences array.
//
// This primitive owns the static-table side of that mapping.
// /Differences override application is the caller's job (it's
// per-font state, not encoding state). Combined with
// foundation::agl the chain reads:
//
//   byte → glyph_name (here, with /Differences override
//                       composed by the caller)
//          → Unicode codepoint(s) (foundation::agl)
//
// Sources for the static tables:
//   * StandardEncoding, MacRomanEncoding, WinAnsiEncoding,
//     PDFDocEncoding — pdfminer.six's `latin_enc.ENCODING`
//     master list (BSD-licensed, itself sourced from PDF
//     32000 Annex D and the AGL distribution).
//
// Out of scope for v1:
//   * SymbolEncoding + ZapfDingbatsEncoding — these are
//     font-built-in encodings with glyph names ("Alpha",
//     "spade", etc.) that do NOT resolve through the Adobe
//     Glyph List. They need a separate API + lookup table
//     (deferred to encoding_tables v1.1 when a real Symbol/
//     ZapfDingbats consumer surfaces).
//   * MacExpertEncoding — uncommon; supplementary expert
//     glyph set used by typographic high-end work. Deferred.
//   * /Differences composition — caller-side state. The
//     v1.1 text-decoder primitive that wraps encoding_tables
//     + agl + truetype will own that composition.

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace foundation::encoding_tables {

// The 4 v1 standard encodings. The enum order is stable —
// callers may store the int value across versions.
enum class Encoding {
    StandardEncoding = 0,
    MacRomanEncoding = 1,
    WinAnsiEncoding  = 2,
    PDFDocEncoding   = 3,
};

// Look up the PostScript glyph name at byte `code` in
// `encoding`. Returns an empty string_view when the byte is
// unmapped in that encoding (e.g. WinAnsi 0x9D is officially
// undefined).
//
// Returned string_view points into static storage and is
// stable for the program's lifetime. Caller must NOT free.
//
// Glyph names are AGL-resolvable: hand off the result to
// foundation::agl::Lookup() to get the Unicode codepoint(s).
std::string_view ByteToGlyphName(
    Encoding encoding, std::uint8_t code) noexcept;

// All 256 byte slots for `encoding`, in order. Returns a
// fixed-size view; unmapped slots are empty string_views.
// Useful when an entire 256-slot table needs to be walked
// (e.g. building a /Differences-merged composite map).
std::span<const std::string_view> Table(
    Encoding encoding) noexcept;

// Parse the textual encoding name from a PDF font dict's
// /Encoding entry into an Encoding enum value. Recognises
// the four canonical names exactly as PDF 32000 spells
// them: "StandardEncoding", "MacRomanEncoding",
// "WinAnsiEncoding", "PDFDocEncoding". Returns std::nullopt
// for any unknown name (including the v1-deferred
// SymbolEncoding / ZapfDingbatsEncoding / MacExpertEncoding).
//
// `name` must be the raw glyph-name spelling — no leading
// '/' (foundation::lexer + foundation::objects strip the
// slash before producing the Name token).
std::optional<Encoding> EncodingFromName(
    std::string_view name) noexcept;

}  // namespace foundation::encoding_tables
