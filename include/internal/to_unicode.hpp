#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::to_unicode {

// One codespace range declared by begincodespacerange.
// byte_count fixes the number of bytes a valid char code
// in this range consumes from a text-op string.
struct CodespaceRange {
    std::uint8_t byte_count;    // 1 or 2 in v1
    std::uint32_t start;
    std::uint32_t end;
};

// One character-to-Unicode mapping expanded from bfchar /
// bfrange. unicode is 1+ UTF-16 BMP codepoints (multi-
// codepoint results model ligatures like "ffi").
struct Mapping {
    std::uint8_t byte_count;                 // matches the
                                             // CMap-declared
                                             // src-hex width
    std::uint32_t char_code;
    std::vector<std::uint32_t> unicode;      // 1+ BMP cps
};

// Parsed ToUnicode CMap.
//
//   codespace — the (byte_count, start, end) triples declared
//               by begincodespacerange blocks.
//   mappings  — every char→unicode mapping expanded from
//               beginbfchar / beginbfrange. Sorted by
//               char_code; duplicates resolved last-wins
//               during parsing.
struct ToUnicode {
    std::vector<CodespaceRange> codespace;
    std::vector<Mapping> mappings;
};

// Parse a ToUnicode CMap (§9.10.3) from raw bytes. Throws
// std::runtime_error for:
//   * malformed hex token widths
//   * dst codepoint outside the BMP (including surrogates)
//   * bfrange array length != end - start + 1
//   * bfrange single-dst overflow past 0xFFFF
//   * bfrange with end < start
ToUnicode Parse(std::span<const std::byte> input);

// Freeze-gate form. Emits:
//   codespace <byteCount> <startHex> <endHex>\n ...
//   bf        <byteCount> <charHex>  <uniHex> [<uniHex>...]\n ...
//
// codespace lines first (sorted by padded startHex), then bf
// lines (sorted by padded charHex). Hex widths:
//   * codespace's startHex/endHex = 2 * that entry's byte_count
//   * bf's charHex = 2 * widest byte_count across codespace
//     entries (so mixed-width CMaps stay unambiguous)
//   * uniHex always 4 digits (BMP)
// Every line has a trailing newline including the last.
std::string ToCanonical(const ToUnicode& cmap);

}
