#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::bmp_decoder {

// Decoded-image result. pixels is row-major RGBA8 with no
// stride padding: exactly width*height*4 bytes, top-to-bottom,
// R before G before B before A. Alpha is widened to 0xFF when
// the source BMP didn't carry an alpha channel (24-bit BI_RGB);
// for 32-bit BI_BITFIELDS the alpha byte is preserved verbatim.
struct DecodedImage {
    std::uint32_t width;       // from biWidth
    std::uint32_t height;      // from biHeight (positive only)
    std::uint32_t components;  // always 4 (RGBA)
    std::vector<std::byte> pixels;  // width * height * 4 bytes
};

// One-shot decode of a BMP byte stream into RGBA8 pixels.
// `input` is the complete BMP file bytes starting from the
// 14-byte BITMAPFILEHEADER. The decoder inverts the BMP's
// bottom-up scanline order so callers see a uniform
// top-down RGBA8 buffer.
//
// Throws std::runtime_error on a malformed BMP: input too
// short, bfType != "BM", biSize != 40, biWidth or biHeight
// non-positive, biPlanes != 1, biBitCount not in {24, 32},
// (biBitCount, biCompression) not in {(24, BI_RGB),
// (32, BI_BITFIELDS)}, BITFIELDS masks not canonical
// (R=0x00FF0000, G=0x0000FF00, B=0x000000FF), biClrUsed
// non-zero (palette out of scope), bfOffBits below the
// minimum (54 / 66), or pixel data running past EOF.
//
// Thread-safe: no shared mutable state, every call owns its
// working buffer.
DecodedImage Decode(std::span<const std::byte> input);

}
