#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::ascii85 {

// Encode ``input`` as an ASCII85 stream (no "~>" EOD marker).
// The output is pure ASCII85 body bytes — '!' .. 'u' plus 'z' for
// the four-zero shortcut. The caller wraps the result with the
// EOD marker if it's emitting a PDF ASCII85 stream; keeping the
// codec marker-free lets PostScript callers reuse it unchanged.
//
// Throws std::runtime_error only on internal failure — well-formed
// input always encodes successfully (any byte sequence is valid
// ASCII85 input).
std::vector<std::byte> Encode(std::span<const std::byte> input);

// Decode an ASCII85 body (no "~>" EOD marker; the caller strips it
// before calling Decode). Embedded whitespace (space, tab, CR, LF,
// FF, NUL) is silently skipped — PDF wraps long ASCII85 bodies
// across multiple lines and the decoder must tolerate that.
//
// Throws std::runtime_error on malformed input: any non-whitespace
// non-'z' char outside '!'..'u', a 'z' shortcut in a partial
// group, a partial trailing group of length 1, or a 4-byte group
// whose value would overflow 0xFFFFFFFF.
std::vector<std::byte> Decode(std::span<const std::byte> input);

}
