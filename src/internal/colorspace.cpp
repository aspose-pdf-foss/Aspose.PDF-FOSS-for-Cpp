#include "colorspace.hpp"
#include <algorithm>
#include <cmath>
// std::to_string + std::invalid_argument — MSVC's libc++/STL
// doesn't pull either in transitively; libstdc++ + libc++ do.
// Spell them explicitly so the lib builds on every conforming
// toolchain.
#include <stdexcept>
#include <string>

namespace {

// Helper: clamp a double to [0, 1] with explicit NaN→0 handling.
// std::clamp(NaN, 0, 1) is implementation-defined (NaN is not less than anything),
// so we implement it by hand per the spec's intent: NaN → 0, <0 → 0, >1 → 1.
inline double Clamp01(double v) noexcept {
    // NaN check must come first: NaN < 0 is false, NaN > 1 is false, so
    // a naive min/max chain would leave NaN unchanged. We want NaN → 0.
    if (std::isnan(v)) {
        return 0.0;
    }
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

// Helper: round-half-away-from-zero for [0,1] → [0,255]
inline std::uint8_t RoundHalfAwayFromZero(double v) noexcept {
    // Clamp first (handles NaN→0, out-of-range)
    v = Clamp01(v);
    // Multiply, add 0.5, truncate toward zero (C++ cast does truncation)
    double scaled = v * 255.0 + 0.5;
    // Cast to int then to uint8 avoids undefined behavior on overflow
    int truncated = static_cast<int>(scaled);
    if (truncated < 0) return 0;
    if (truncated > 255) return 255;
    return static_cast<std::uint8_t>(truncated);
}

} // namespace

namespace foundation::colorspace {

ColorRgb ToSrgb(Family family,
                std::span<const double> components) {
    const std::size_t expected = ComponentCount(family);
    if (components.size() != expected) {
        throw std::invalid_argument(
            "ToSrgb: components span size (" +
            std::to_string(components.size()) +
            ") does not match expected component count (" +
            std::to_string(expected) +
            ") for family " +
            std::to_string(static_cast<int>(family)));
    }

    switch (family) {
        case Family::DeviceGray: {
            double gray = Clamp01(components[0]);
            return ColorRgb{gray, gray, gray};
        }
        case Family::DeviceRGB: {
            return ColorRgb{
                Clamp01(components[0]),
                Clamp01(components[1]),
                Clamp01(components[2])
            };
        }
        case Family::DeviceCMYK: {
            double C = Clamp01(components[0]);
            double M = Clamp01(components[1]);
            double Y = Clamp01(components[2]);
            double K = Clamp01(components[3]);
            double one_minus_K = 1.0 - K;
            return ColorRgb{
                (1.0 - C) * one_minus_K,
                (1.0 - M) * one_minus_K,
                (1.0 - Y) * one_minus_K
            };
        }
        default:
            // Should be unreachable due to enum range
            return ColorRgb{0.0, 0.0, 0.0};
    }
}

std::uint8_t Quantize(double value) noexcept {
    return RoundHalfAwayFromZero(value);
}

}
