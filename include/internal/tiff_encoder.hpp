#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::tiff_encoder {

// One page worth of RGBA8 pixels. Width and height are in
// pixels; rgba is exactly width*height*4 bytes, row-major
// top-to-bottom, R before G before B before A. Non-owning
// view — the encoder reads but doesn't retain across the
// Encode call.
struct Page {
    std::uint32_t width;
    std::uint32_t height;
    std::span<const std::byte> rgba;
};

// Compression schemes recognised by the TIFF encoder.
// Maps to the TIFF 6.0 Compression-tag (259) values
// {1, 5, 8, 32773} respectively at write time.
enum class Compression {
    None,
    Lzw,
    Deflate,
    PackBits,
};

// Photometric interpretation for the on-wire pixel data.
// Maps to the TIFF 6.0 PhotometricInterpretation-tag (262)
// values {0, 1, 2, 3} respectively at write time.
// WhiteIsZero is enumerated for completeness but is out of
// v1 scope — the encoder rejects it.
enum class Photometric {
    WhiteIsZero,
    BlackIsZero,
    Rgb,
    Palette,
};

// Encoder-side knobs. Defaults are RGBA pass-through at 72
// DPI uncompressed, 8 bits per sample. color_map is non-empty
// (3 * (1 << bits_per_sample) std::uint16_t entries scaled to
// 0..65535: all reds, then all greens, then all blues) when
// photometric == Palette; empty otherwise.
//
// Valid (samples_per_pixel, photometric, bits_per_sample) tuples:
//   (1, BlackIsZero, 1)  — 1bpp grayscale, luma-thresholded
//   (1, Palette,     4)  — 4bpp palette, 16-entry color_map
//   (1, BlackIsZero, 8)  — 8bpp grayscale, strict R=G=B
//   (1, Palette,     8)  — 8bpp palette, 256-entry color_map
//   (3, Rgb,         8)  — 24bpp RGB
//   (4, Rgb,         8)  — 32bpp RGBA
struct Options {
    Compression compression = Compression::None;
    Photometric photometric = Photometric::Rgb;
    std::uint32_t samples_per_pixel = 4;
    std::uint32_t bits_per_sample = 8;
    std::uint32_t dpi = 72;
    std::span<const std::uint16_t> color_map;
};

// Encode `pages` into a baseline TIFF byte stream. Returns
// the encoded bytes. Throws std::runtime_error on empty
// pages, per-page rgba length mismatch, photometric /
// samples_per_pixel mismatch, photometric=BlackIsZero with
// non-grayscale input, photometric=Palette without
// color_map, or non-positive dpi.
//
// Multi-page TIFFs emit chained IFDs in `pages` order; each
// page's IFD names its own strip data (single strip per page,
// RowsPerStrip = ImageLength). Strip compression delegates to
// foundation::lzw / foundation::flate / foundation::runlength.
//
// Thread-safe: no shared mutable state, every call owns its
// working buffers.
std::vector<std::byte> Encode(std::span<const Page> pages,
                              Options const& options);

}
