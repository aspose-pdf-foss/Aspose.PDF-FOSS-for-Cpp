#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "transform.hpp"
#include "colorspace.hpp"
#include "rasterizer.hpp"

namespace foundation::image_compositor {

struct ImageSource {
    std::uint32_t width;
    std::uint32_t height;
    // RGBA8, width*height*4 bytes. Non-owning view over the caller's
    // decoded buffer; the caller keeps it alive across Composite.
    std::span<const std::byte> pixels;
};

void Composite(foundation::rasterizer::Raster& dst,
               const ImageSource& src,
               const foundation::transform::Matrix& ctm,
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
