#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "transform.hpp"
#include "colorspace.hpp"

namespace foundation::rasterizer {

enum class FillRule : std::uint8_t {
    NonZero = 0,
    EvenOdd = 1,
};

enum class SegmentKind : std::uint8_t {
    Move  = 0,
    Line  = 1,
    Rect  = 2,
    Close = 3,
};

struct PathSegment {
    SegmentKind kind;
    foundation::transform::Point pts[2];
};

struct Path {
    std::vector<PathSegment> segments;
};

struct Raster {
    std::vector<std::uint8_t> pixels;
    std::uint32_t width;
    std::uint32_t height;
};

// Axis-aligned device-pixel clip rectangle, half-open [x0, x1) ×
// [y0, y1). Passed (optionally) to the pixel-writing primitives to
// restrict output to a rectangular region — the PDF §8.5.4 clipping
// path, in the common rectangular case. A null clip pointer means no
// clipping. Supplying a clip is output-preserving when the rectangle
// covers the whole raster: the primitives already skip pixels with no
// coverage, so intersecting the write extent with the clip only avoids
// visiting pixels outside it.
struct ClipRect {
    std::int32_t x0, y0, x1, y1;
};

void Fill(Raster& dst,
          const Path& path,
          const foundation::transform::Matrix& ctm,
          const foundation::colorspace::ColorRgb& color,
          double alpha,
          FillRule rule,
          const ClipRect* clip = nullptr);

struct DecodedImage {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t components;
    std::vector<std::byte> pixels;
};

DecodedImage Decode(std::span<const std::byte> bytes);

}
