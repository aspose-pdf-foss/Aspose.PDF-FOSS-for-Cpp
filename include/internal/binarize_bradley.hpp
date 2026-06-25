// foundation/binarize_bradley — Bradley adaptive thresholding
// (Bradley & Roth, 2007). 8-bit grayscale → 1-bit bilevel via
// integral-image-based local mean + integer cross-multiplication.
//
// Used by Aspose::Pdf::Devices::TiffDevice's BinarizeBradley
// static utility. The shared spec yaml pins the algorithm
// down to byte-exact output across every target — Bradley is
// fully integer-deterministic when the threshold check is
// rewritten as a cross-multiplication
// (I * count * 100 <= sum * (100 - t)) instead of the textbook
// floating-point form. Every target's freeze-gate test loads
// the same .case + .expected fixture pairs and asserts byte-
// equal output.
//
// from_scratch — Bradley is ~80 LOC of integer arithmetic.
// No permissive C++ library exposes the algorithm as a
// reusable function (OpenCV's adaptiveThreshold uses a
// different decision rule and would carry whole-package C
// deps through the foundation layer for a small operation).

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::binarize_bradley {

// Run Bradley adaptive thresholding over a row-major 8-bit
// grayscale buffer. Returns the row-major bilevel output,
// MSB-first per byte, right-padded with WHITE (1) bits when
// `width` is not a multiple of 8.
//
// `grayscale` must be row-major, top-left origin, 8 bits per
// pixel; size must equal width * height.
// `width`, `height` must be in (0, 65535].
// `window_size` must be in [2, max(width, height)]. Caller
// passes max(2, width / 8) per Bradley's paper.
// `threshold_percent` must be in [0, 100]. Default
// caller-side is 15.
//
// Throws std::runtime_error on any out-of-range argument or
// length mismatch. The error message names the offending
// value.
std::vector<std::byte> Apply(
    std::span<const std::byte> grayscale,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t window_size,
    std::uint32_t threshold_percent);

}  // namespace foundation::binarize_bradley
