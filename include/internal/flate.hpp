#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::flate {

// Deflate-compress ``input`` into a complete zlib stream
// (RFC 1950 container around RFC 1951 DEFLATE). Returns a
// newly allocated byte vector sized to the exact compressed
// length.
//
// Throws std::runtime_error on internal allocation failure.
// Empty input is valid and produces a small but non-empty
// output (zlib header + one final block + Adler-32).
std::vector<std::byte> Encode(std::span<const std::byte> input);

// Inflate a zlib stream back to its original plaintext.
// Input must be a complete zlib-framed stream — partial
// streams throw. Returns a newly allocated byte vector with
// the exact decompressed length.
//
// Throws std::runtime_error on malformed input: bad zlib
// header, reserved BTYPE, missing end-of-block code,
// back-reference distance beyond available output,
// truncated stream, or Adler-32 mismatch with the trailer.
std::vector<std::byte> Decode(std::span<const std::byte> input);

}
