// foundation/ccitt — PDF /CCITTFaxDecode filter (T.4 G3 + T.6 G4).
// See the project spec for the spec anchor and
// the project spec for the C++ surface
// contract.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace foundation::ccitt {

struct Params {
    std::int8_t   K = 0;
    bool          EndOfLine = false;
    bool          EncodedByteAlign = false;
    bool          EndOfBlock = true;
    bool          BlackIs1 = false;
    std::uint32_t Columns = 1728;
    std::uint32_t Rows = 0;
    std::uint32_t DamagedRowsBeforeError = 0;
};

struct DecodedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::byte> bits;
};

DecodedImage Decode(std::span<const std::byte> stream,
                    const Params& params);

// Encode a bilevel raster (image.bits, MSB-first, (Columns+7)/8 bytes per
// row, same polarity convention as Decode's output for the given BlackIs1)
// to a CCITT fax stream. Supports K < 0 (T.6 Group 4, 2D) and K == 0 (T.4
// Group 3 1D). Honours BlackIs1, EndOfLine (G3) and EncodedByteAlign (G3),
// and appends EOFB (G4) / RTC (G3) when EndOfBlock. image.width -> Columns,
// image.height -> Rows. Throws std::runtime_error on K > 0 (mixed 1D/2D not
// implemented) or a raster smaller than width*height.
std::vector<std::byte> Encode(const DecodedImage& image,
                              const Params& params);

DecodedImage Parse(std::span<const std::byte> file);

std::string ToCanonical(const DecodedImage& image);

}
