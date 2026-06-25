#pragma once

// =============================================================================
// foundation::palette_quantiser — RGBA8 → palettised indexed image.
//
// Builds a colour palette of at most ``max_colors`` entries (≤ 256)
// and the corresponding per-pixel index buffer for an RGBA8 input.
// Used by Devices::TiffDevice for ColorDepth::Format4bpp /
// Format8bpp output and by IIndexBitmapConverter (Get1Bpp/4Bpp/8Bpp).
//
// Algorithm: median-cut (Heckbert 1982). Drops the alpha channel
// before quantisation — TIFF palette mode doesn't carry alpha and
// callers wanting alpha should keep RGBA8 / 24bpp.
//
// Spec source: the .NET BCL contract (System.Drawing.Imaging.
// ColorPalette + Bitmap.Clone palette formats) — see
// the project spec The PDF spec doesn't
// pin palette quantisation; commercial Aspose.PDF .NET delegates
// to the BCL surface, so the BCL contract IS the spec. The
// contract pins shape (palette ≤ max_colors entries, indexed
// pixel buffer, exact-match self-identity, …); the algorithm is
// implementor's choice.
// =============================================================================

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::palette_quantiser {

struct IndexedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    // Palette entries: RGB byte triples. Size in [1, max_colors].
    std::vector<std::array<std::uint8_t, 3>> palette;
    // One byte per pixel; values index into ``palette``. Length is
    // exactly ``width * height``. Caller packs into 4bpp / 1bpp
    // shapes if needed.
    std::vector<std::uint8_t> indices;
};

// Quantise an RGBA8 image into at most ``max_colors`` palette
// entries plus per-pixel indices. ``max_colors`` must be in
// [2, 256] — outside that the function throws std::invalid_argument.
//
// ``rgba`` is row-major, top-left origin, 4 bytes per pixel
// (R G B A); alpha is dropped. Returns an IndexedImage carrying
// the constructed palette and the indexed pixel buffer.
IndexedImage Quantise(std::span<const std::uint8_t> rgba,
                      std::uint32_t width,
                      std::uint32_t height,
                      std::uint32_t max_colors);

}  // namespace foundation::palette_quantiser
