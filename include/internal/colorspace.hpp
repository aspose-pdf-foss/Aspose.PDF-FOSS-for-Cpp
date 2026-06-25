#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace foundation::colorspace {

enum class Family : std::uint8_t {
    DeviceGray = 1,
    DeviceRGB  = 3,
    DeviceCMYK = 4,
};

struct ColorRgb {
    double r;
    double g;
    double b;
};

constexpr std::size_t ComponentCount(Family f) noexcept {
    return static_cast<std::size_t>(f);
}

ColorRgb ToSrgb(Family family,
                std::span<const double> components);

std::uint8_t Quantize(double value) noexcept;

}
