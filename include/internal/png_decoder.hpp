#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::png_decoder {

// Decoded-image result. pixels is row-major RGBA8 with no
// stride padding: exactly width*height*4 bytes, top-to-bottom,
// R before G before B before A. Alpha is widened to 0xFF when
// the source PNG didn't carry an alpha channel (Grayscale or
// Rgb color types); for Rgba the alpha byte is preserved
// verbatim. Grayscale samples are replicated into R = G = B.
struct DecodedImage {
    std::uint32_t width;       // from IHDR width
    std::uint32_t height;      // from IHDR height
    std::uint32_t components;  // always 4 (RGBA)
    std::vector<std::byte> pixels;  // width * height * 4 bytes
};

// One-shot decode of a PNG 1.2 baseline byte stream into RGBA8
// pixels. `input` is the complete PNG file bytes starting from
// the 8-byte signature.
//
// Throws std::runtime_error on a malformed PNG: signature
// mismatch, CRC mismatch on any chunk, missing IHDR / IDAT /
// IEND, IHDR validation failure (bit_depth != 8, color_type
// not in {0,2,6}, compression != 0, filter != 0, interlace
// != 0, dimensions out of range), IDAT concatenated length
// not equal to expected scanline byte count, unsupported
// filter type byte, foundation::flate failure. Out-of-scope
// features (bit depth != 8, palette images, grayscale+alpha,
// Adam7 interlace) raise with a message naming the missing
// capability.
//
// Thread-safe: no shared mutable state, every call owns its
// working buffers.
DecodedImage Decode(std::span<const std::byte> input);

}
