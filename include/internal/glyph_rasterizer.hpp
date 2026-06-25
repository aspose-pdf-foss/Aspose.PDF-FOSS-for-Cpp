#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "transform.hpp"
#include "colorspace.hpp"
#include "rasterizer.hpp"

namespace foundation::glyph_rasterizer {

enum class OutlineOp : std::uint8_t {
    Move  = 0,
    Line  = 1,
    Curve = 2,
    Close = 3,
};

struct OutlineSegment {
    OutlineOp op;
    foundation::transform::Point pts[3];
};

struct Outline {
    std::vector<OutlineSegment> segments;
};

void Draw(foundation::rasterizer::Raster& dst,
          const Outline& outline,
          const foundation::transform::Matrix& ctm,
          const foundation::colorspace::ColorRgb& color,
          double alpha,
          const foundation::rasterizer::ClipRect* clip = nullptr);

struct DecodedImage {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t components;
    std::vector<std::byte> pixels;
};

DecodedImage Decode(std::span<const std::byte> bytes);

}
