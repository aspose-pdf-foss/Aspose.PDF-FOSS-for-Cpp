#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::asciihex {

// Encode ``input`` as an ASCII hex body (no '>' EOD marker).
// Each input byte becomes exactly two uppercase hex digits;
// output size is always 2 * input.size(). No whitespace is
// inserted — line-wrapping is the PDF-stream writer's concern,
// not the codec's. Callers add the '>' EOD marker when wrapping
// the output as a PDF ASCIIHexDecode stream.
//
// Throws std::runtime_error only on internal failure — any byte
// sequence is valid ASCIIHex input, so well-formed input always
// encodes successfully.
std::vector<std::byte> Encode(std::span<const std::byte> input);

// Decode an ASCII hex body. Skips embedded whitespace anywhere
// (space, HT, LF, CR, FF, NUL per PDF 32000 §7.4.2), accepts
// both uppercase and lowercase hex digits, stops at '>' if
// encountered, and pads an odd-trailing hex digit with '0'
// per spec. The caller may or may not have stripped the '>'
// EOD marker — Decode handles both.
//
// Throws std::runtime_error on any non-whitespace, non-hex,
// non-'>' character, naming the offending byte in the message.
std::vector<std::byte> Decode(std::span<const std::byte> input);

}
