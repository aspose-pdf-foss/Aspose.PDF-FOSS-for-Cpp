#pragma once

// Foundation primitive: Adobe Glyph List (AGL) lookup.
//
// PostScript glyph names (e.g. "Aacute", "ffi", "uni4E2D") are
// the bridge between a PDF font's /Encoding (or /Differences
// override) and Unicode. PDF 32000-1:2008 §9.10.2 mandates the
// fallback chain when a font has no /ToUnicode CMap:
//
//   byte → glyph_name (via /Encoding + /Differences) →
//          Unicode codepoint(s) (via the Adobe Glyph List)
//
// This primitive owns the second arrow. The byte → glyph_name
// step lives in foundation::encoding_tables.
//
// Source data: pdfminer.six's `glyphname2unicode` table, which
// is itself sourced from Adobe's published Glyph List
// (https://github.com/adobe-type-tools/agl-aglfn). Both sources
// are public domain. ~4280 entries; 81 of them map to multi-
// codepoint sequences (ligatures like "ffi" → U+0066 U+0066
// U+0069, plus a long tail of pre-composed Hebrew + Arabic
// glyph names that pdfminer encodes as multi-codepoint
// strings).
//
// In addition to the static table, the lookup recognises the
// per-§4 fallbacks defined in the AGL specification:
//
//   uniHHHH       — single 4-hex-digit BMP codepoint
//   uniHHHHHHHH…  — sequences of 4-hex-digit BMP codepoints
//                    (each group of 4 hex digits is one
//                    codepoint; surrogates U+D800-U+DFFF
//                    rejected)
//   uHHHHHH       — single 4-to-6-hex-digit codepoint (any
//                    plane; surrogates rejected)
//   <name>.<suffix> — drops everything from the first dot;
//                    looks up the prefix
//   <name>_<name>… — joins per-component lookups (ligatures
//                    expressed as compound names)
//
// Reference: https://github.com/adobe-type-tools/agl-specification

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace foundation::agl {

// Look up `glyph_name` and return its Unicode codepoints.
// Returns an empty vector when the name is unknown OR violates
// the AGL spec (e.g. uniXXXX with non-hex digits, surrogates).
//
// Always returns by value — the multi-codepoint table is sparse
// and the per-call vector keeps the API uniform whether the
// result is one codepoint or many. Vectors of >1 codepoint
// arise from pre-composed ligature names ("ffi", "fl", etc.)
// and the full table's Hebrew/Arabic compound entries.
//
// The lookup applies the AGL §4 transformations in order:
//   1. Strip everything from the first dot (suffix variant).
//   2. If the result has underscores, split and recurse.
//   3. Try the static AGL table.
//   4. Try uniHHHH... (4-digit groups, BMP only).
//   5. Try uHHHHHH (4-6 digits, any plane).
//   6. Return empty.
std::vector<std::uint32_t> Lookup(
    std::string_view glyph_name) noexcept;

// Test-only inverse access to the static table — count of
// entries plus iteration over (name, codepoint-list) pairs.
// Useful for build-time sanity checks. The codepoint list is
// a stable reference into the static table.
struct Entry {
    std::string_view name;
    std::span<const std::uint32_t> codepoints;
};

std::span<const Entry> StaticEntries() noexcept;

}  // namespace foundation::agl
