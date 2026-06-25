#pragma once

// =============================================================================
// foundation::page_renderer — software page rasteriser.
//
// Given the raw bytes of a PDF file plus a 0-based page index, produce
// an RGBA8 pixel buffer at the requested resolution by walking the
// page's content stream and dispatching every operator through the
// the foundation primitives (rasterizer, image_compositor,
// glyph_rasterizer, content_stream, …).
//
// See the project spec for the full anchor:
// scope (operator subset, supported colour spaces / fonts / image
// filters), out-of-scope items, and the PSNR-tolerance freeze gate
// against mutool draw + pdftocairo.
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::page_renderer {

// Output of one Render call. Pixels are RGBA8, row-major, top-left
// origin (image-conventional, y-down). Buffer size is exactly
// width * height * 4 bytes.
struct RenderedPage {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<std::uint8_t> pixels;
};

// Render one page of a PDF document at the requested DPI.
//
// Throws std::out_of_range when page_index >= pages_tree.leaves.size().
// Throws std::runtime_error on any of the spec-listed error conditions
// (unsupported filter, malformed page, encrypted document, etc.).
//
// page_index is 0-based — distinct from PDF Page::Number which is
// 1-based per §7.7.3.3.
//
// transparent_background defaults to false (opaque white initial
// canvas, matching the v1 default rendering). When true, the canvas
// starts fully-transparent (0,0,0,0); page content composites against
// transparent pixels — supports the public-API PngDevice
// TransparentBackground flag and any alpha-aware downstream consumer.
RenderedPage Render(std::span<const std::byte> input,
                    std::uint64_t page_index,
                    double target_dpi,
                    bool transparent_background = false);

}  // namespace foundation::page_renderer
