#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::png_encoder {

// PNG IHDR color_type byte values per PNG 1.2 §4.1.1. Three
// in-scope variants:
//
//   * Grayscale (0)  — 1 sample per pixel; encoder requires
//                      R == G == B at every input pixel
//                      (otherwise rejects). Alpha dropped on
//                      encode; the round-trip widens alpha
//                      back to 0xFF on read, so for byte-
//                      identical round-trip the input alpha
//                      must be 0xFF.
//   * Rgb (2)        — 3 samples per pixel; alpha dropped on
//                      encode.
//   * Rgba (6)       — 4 samples per pixel; alpha preserved
//                      end-to-end.
enum class ColorType : std::uint8_t {
    Grayscale = 0,
    Rgb = 2,
    Rgba = 6,
};

struct Options {
    ColorType color_type = ColorType::Rgba;
};

// One-shot encode of a single RGBA8 pixel buffer into a PNG
// 1.2 baseline byte stream. `rgba8` is row-major top-to-bottom,
// exactly width*height*4 bytes, R before G before B before A.
//
// Throws std::runtime_error on a malformed call:
//   - width == 0 or height == 0 or > 2^31-1
//   - rgba8.size() != width*height*4
//   - color_type not in {Grayscale (0), Rgb (2), Rgba (6)}
//   - color_type == Grayscale with non-grayscale input
//     (R != G or G != B at any pixel)
//
// Thread-safe: no shared mutable state, every call owns its
// working buffers.
std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options);

}
