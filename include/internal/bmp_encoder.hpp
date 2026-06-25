#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::bmp_encoder {

// Pixel-format selector. Bgr emits a 24-bit BI_RGB BMP and
// drops the alpha channel on write (the round-trip through
// foundation::bmp_decoder widens alpha back to 0xFF on read,
// so for byte-identical round-trip the input alpha must be
// 0xFF). Bgra emits a 32-bit BI_BITFIELDS BMP that preserves
// alpha end-to-end. Default Bgra (alpha-preserving — middle-
// of-the-road safe for callers).
enum class ColorType : std::uint8_t {
    Bgr = 24,
    Bgra = 32,
};

struct Options {
    ColorType color_type = ColorType::Bgra;
};

// One-shot encode of a single RGBA8 pixel buffer into a BMP
// byte stream. `rgba8` is row-major top-to-bottom, exactly
// width * height * 4 bytes, R before G before B before A.
//
// Throws std::runtime_error on a malformed call:
//   - width == 0 or height == 0 or > 2^31-1
//   - rgba8.size() != width * height * 4
//   - color_type not in {Bgr, Bgra}
//
// Thread-safe: no shared mutable state, every call owns its
// working buffer.
std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options);

}
