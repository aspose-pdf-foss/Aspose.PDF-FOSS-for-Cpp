#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::jbig2 {

// Decoded bilevel image. Layout matches dctdecode / jpx /
// icc_profile / image_compositor — the standard
// lossy_oracle_agreement gate template's expected shape.
//
// For bilevel output: components = 1, pixels packs 8 pixels per
// byte MSB-first (leftmost pixel of a row is bit 7 of that
// row's first byte). ((width + 7) / 8) bytes per row; padding
// bits at the right end of each row are zero. Bit polarity
// follows JBIG2: 1 = black, 0 = white.
struct DecodedImage {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t components;       // always 1 (bilevel)
    std::vector<std::byte> pixels;  // ((w+7)/8) * h bytes
};

// Decode a JBIG2 page-segment stream. `page` is the embedded
// segment-stream form (no JBIG2 file header — what PDF
// /JBIG2Decode wraps). `globals` is the optional shared-segment
// stream from the PDF's /JBIG2Globals filter parameter.
//
// Scope: page-info (48) / generic region (38, 39) / symbol
// dictionary (0) / text region (4, 6, 7) / end-of-page (49) /
// end-of-stripe (50) / end-of-file (51) segment types,
// GBTEMPLATE 0/1/2/3, arithmetic-coded only (no Huffman). Includes
// the generic refinement region (§6.3) with refine/aggregate
// symbol coding (SDREFAGG) and per-instance text-region refinement
// (SBREFINE). `globals` (PDF /JBIG2Globals) is processed as a
// leading segment stream carrying symbol dictionaries that page
// text regions reference. See `the project spec` for
// the remaining roadmap (Huffman variants, pattern/halftone
// regions, MMR generic regions, standalone refinement segments).
//
// Throws std::invalid_argument on malformed input or unsupported
// features (segment type out of scope, Huffman coding, indefinite
// length, MMR generic regions). Throws std::runtime_error on
// internal logic errors (unreachable on well-formed input).
DecodedImage Decode(std::span<const std::byte> page,
                    std::span<const std::byte> globals = {});

}
