#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::tiff_decoder {

// Decoded-image result. pixels is row-major RGBA8 with no
// stride padding: exactly width*height*4 bytes, top-to-bottom,
// R before G before B before A. Alpha is widened to 0xFF when
// the source TIFF didn't carry an alpha channel.
struct DecodedImage {
    std::uint32_t width;       // in pixels, from ImageWidth tag
    std::uint32_t height;      // in pixels, from ImageLength tag
    std::uint32_t components;  // always 4 (RGBA)
    std::vector<std::byte> pixels;  // width * height * 4 bytes
};

// One-shot decode of a TIFF byte stream into RGBA8 pixels.
// Reads the first IFD only — multi-page TIFFs surface as
// their first-page image. input is the complete TIFF file
// bytes starting from the 8-byte header.
//
// Throws std::runtime_error on a malformed TIFF: missing or
// wrong header, unsupported tag value, missing in-scope tag,
// validation failure (e.g. SamplesPerPixel not in {1,3,4},
// BitsPerSample != 8), truncated strip, or decompressor
// failure on a strip body. Out-of-scope features (tiles, bit
// depths != 8, CMYK, YCbCr, PlanarConfiguration=2, predictor
// != 1, CCITT compression) raise with a message naming the
// missing capability. Zero-length input throws.
//
// Thread-safe: no shared mutable state, every call owns its
// working buffers.
DecodedImage Decode(std::span<const std::byte> input);

}
