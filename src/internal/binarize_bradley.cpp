#include "binarize_bradley.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::binarize_bradley {

namespace {

std::vector<std::uint64_t> BuildIntegralImage(
    std::span<const std::byte> grayscale,
    std::uint32_t width,
    std::uint32_t height) {
    // Integral image S(x, y) = sum of I(i, j) for i<=x, j<=y.
    // Stored 1-indexed with a zero-padded row/column at index 0
    // so out-of-bounds lookups (y<0 / x<0) return 0 naturally.
    const std::uint64_t stride =
        static_cast<std::uint64_t>(width) + 1;
    std::vector<std::uint64_t> integral(
        stride * (static_cast<std::uint64_t>(height) + 1), 0);
    for (std::uint32_t y = 0; y < height; ++y) {
        std::uint64_t row_sum = 0;
        const std::uint64_t base_in =
            static_cast<std::uint64_t>(y) * width;
        const std::uint64_t base_out_curr =
            static_cast<std::uint64_t>(y + 1) * stride;
        const std::uint64_t base_out_prev =
            static_cast<std::uint64_t>(y) * stride;
        for (std::uint32_t x = 0; x < width; ++x) {
            row_sum +=
                static_cast<std::uint8_t>(grayscale[base_in + x]);
            integral[base_out_curr + (x + 1)] =
                integral[base_out_prev + (x + 1)] + row_sum;
        }
    }
    return integral;
}

std::vector<std::byte> ThresholdViaIntegral(
    std::span<const std::byte> grayscale,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t window_size,
    std::uint32_t threshold_percent,
    const std::vector<std::uint64_t>& integral) {
    const std::uint32_t half = window_size / 2;
    const std::uint32_t stride_out = (width + 7) / 8;
    const std::uint64_t integral_stride =
        static_cast<std::uint64_t>(width) + 1;
    const std::uint64_t out_len =
        static_cast<std::uint64_t>(height) * stride_out;
    // Initialise to all-WHITE so right-padding bits past the
    // image width stay 1.
    std::vector<std::byte> output(out_len, std::byte{0xFF});
    const std::uint64_t factor = 100 - threshold_percent;

    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint32_t y1 = y > half ? y - half : 0;
        std::uint32_t y2 = y + half;
        if (y2 > height - 1) y2 = height - 1;
        const std::uint64_t i_y_lo = y1;
        const std::uint64_t i_y_hi = y2 + 1;
        const std::uint64_t row_off_hi = i_y_hi * integral_stride;
        const std::uint64_t row_off_lo = i_y_lo * integral_stride;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t x1 = x > half ? x - half : 0;
            std::uint32_t x2 = x + half;
            if (x2 > width - 1) x2 = width - 1;
            const std::uint64_t count =
                static_cast<std::uint64_t>(x2 - x1 + 1) *
                (y2 - y1 + 1);
            const std::uint64_t i_x_lo = x1;
            const std::uint64_t i_x_hi = x2 + 1;
            const std::uint64_t window_sum =
                integral[row_off_hi + i_x_hi]
                - integral[row_off_lo + i_x_hi]
                - integral[row_off_hi + i_x_lo]
                + integral[row_off_lo + i_x_lo];
            const std::uint64_t pixel = static_cast<std::uint8_t>(
                grayscale[static_cast<std::uint64_t>(y) * width + x]);
            // Cross-multiplication: I * count * 100 <= sum * (100 - t).
            if (pixel * count * 100ULL <= window_sum * factor) {
                // BLACK — clear the MSB-first bit for (x).
                const int bit = 7 - static_cast<int>(x % 8);
                const std::uint64_t idx =
                    static_cast<std::uint64_t>(y) * stride_out +
                    (x >> 3);
                output[idx] = std::byte{
                    static_cast<std::uint8_t>(
                        std::to_integer<std::uint8_t>(output[idx])
                        & static_cast<std::uint8_t>(~(1u << bit)))};
            }
        }
    }
    return output;
}

}  // namespace

std::vector<std::byte> Apply(
    std::span<const std::byte> grayscale,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t window_size,
    std::uint32_t threshold_percent) {
    if (width == 0 || width > 65535) {
        throw std::runtime_error(
            "binarize_bradley: width out of range: " +
            std::to_string(width));
    }
    if (height == 0 || height > 65535) {
        throw std::runtime_error(
            "binarize_bradley: height out of range: " +
            std::to_string(height));
    }
    const std::uint64_t expected_len =
        static_cast<std::uint64_t>(width) * height;
    if (grayscale.size() != expected_len) {
        throw std::runtime_error(
            "binarize_bradley: grayscale length " +
            std::to_string(grayscale.size()) +
            " != width*height " +
            std::to_string(expected_len));
    }
    const std::uint32_t max_dim = std::max(width, height);
    if (window_size < 2 || window_size > max_dim) {
        throw std::runtime_error(
            "binarize_bradley: window_size " +
            std::to_string(window_size) +
            " not in [2, max(width, height)]");
    }
    if (threshold_percent > 100) {
        throw std::runtime_error(
            "binarize_bradley: threshold_percent " +
            std::to_string(threshold_percent) +
            " not in [0, 100]");
    }

    const auto integral = BuildIntegralImage(grayscale, width, height);
    return ThresholdViaIntegral(
        grayscale, width, height, window_size,
        threshold_percent, integral);
}

}  // namespace foundation::binarize_bradley
