#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::jpx {

// Decoded-image result. pixels is row-major RGBA8 with no stride
// padding: exactly width*height*4 bytes. Alpha is a constant 0xFF
// (JPEG 2000 alpha/opacity channels are out of v1 scope). Grayscale
// (1-component) is widened R=G=B; 3-component is colour-transformed
// (RCT/ICT inverse) to RGB.
struct DecodedImage {
    std::uint32_t width;       // pixels, from the SIZ marker
    std::uint32_t height;      // pixels, from the SIZ marker
    std::uint32_t components;  // always 4 (RGBA) in v1
    std::vector<std::byte> pixels;  // width * height * 4 bytes
};

// One-shot decode of a JPEG 2000 codestream (raw, SOC 0xFF4F) or a
// JP2 file (signature box). input is the unwrapped contents of a
// /JPXDecode-filtered PDF stream. Returns RGBA8.
//
// Throws std::runtime_error on truncation, malformed/unknown
// markers, or any v1-unsupported feature (multi-tile, >3 components,
// sub-sampling, non-LRCP progression, ROI, non-default code-block
// style, palettized/ICC colour). Throws on zero-length input.
// Thread-safe: no shared mutable state.
DecodedImage Decode(std::span<const std::byte> input);

}
