#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::dct_encoder {

// Photometric interpretation for the on-wire pixel data.
// Rgb (default) treats RGBA8 input as a colour image — the
// encoder drops alpha then converts RGB→YCbCr per JFIF
// Appendix A and emits a 3-component baseline (Y at 1×1, Cb
// at 1×1, Cr at 1×1 — 4:4:4 sampling). Grayscale requires
// R == G == B at every input pixel (the encoder errors if
// they disagree); the encoder drops alpha and emits a
// 1-component baseline carrying only the luma DC + AC tables.
enum class Photometric : std::uint8_t {
    Grayscale = 1,
    Rgb = 2,
};

// Encoder-side knobs. Defaults (quality=75, photometric=Rgb,
// dpi=72) describe the production middle-of-the-road JPEG —
// good visual quality at moderate file size, the same default
// libjpeg-turbo's cjpeg uses.
struct Options {
    std::uint8_t quality = 75;
    Photometric photometric = Photometric::Rgb;
    std::uint32_t dpi = 72;
};

// One-shot encode of a single RGBA8 pixel buffer into a JPEG
// baseline byte stream (T.81 Annex F sequential DCT, 8-bit
// samples, 4:4:4 chroma, no APP markers). ``rgba8`` is row-
// major top-to-bottom, exactly width * height * 4 bytes,
// R before G before B before A.
//
// Throws std::runtime_error on a malformed call:
//   - width == 0 or height == 0 or > 65535
//   - rgba8.size() != width * height * 4
//   - photometric == Grayscale with non-grayscale input
//     (R != G or G != B at any pixel)
//   - dpi == 0
//
// Quality outside 0..100 is clamped to the closed range per
// the IJG reference scaling formula.
//
// Thread-safe: no shared mutable state, every call owns its
// working buffers.
std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options);

}
