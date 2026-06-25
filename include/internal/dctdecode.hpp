#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::dctdecode {

// Decoded-image result. pixels is row-major RGBA8 with no stride
// padding: exactly width*height*4 bytes. Alpha is a constant 0xFF
// because JPEG has no alpha channel — the A lane is padding so
// callers can treat rows as "width * 4" bytes without a special
// case. If the JPEG was grayscale or YCbCr, libjpeg-turbo has
// already upsampled + color-converted to RGBA via its JCS_EXT_RGBA
// output type.
struct DecodedImage {
    std::uint32_t width;       // in pixels, from the JPEG SOF marker
    std::uint32_t height;      // in pixels, from the JPEG SOF marker
    std::uint32_t components;  // always 4 (RGBA) in v1
    std::vector<std::byte> pixels;  // width * height * 4 bytes
};

// One-shot lossy-decode adapter around libjpeg-turbo. input is a
// complete JPEG/JFIF byte stream (the unwrapped contents of a
// /DCTDecode-filtered PDF stream). Returns a DecodedImage with
// pixels converted to RGBA8.
//
// Throws std::runtime_error on any libjpeg error (malformed input,
// unsupported process, out-of-memory). Throws on zero-length input
// — an empty span is not a valid JPEG. Thread-safe: no shared
// mutable state, every call owns its jpeg_decompress_struct.
DecodedImage Decode(std::span<const std::byte> input);

}
