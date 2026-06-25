#pragma once

// =============================================================================
// Aspose::Pdf::BitmapInfo — flat raster image carrier.
//
// Public-API class matching the canonical Aspose.PDF BitmapInfo
// surface: 1 ctor + 4 read-only accessors + a nested PixelFormat
// enum with 5 packed-pixel formats. Cross-platform substitute for
// the BCL System.Drawing.Bitmap return type the canonical .NET
// surface uses on raster-producing devices, with a flat POD that
// keeps the public surface free of System.Drawing.Common
// (Windows-only) dependencies.
// =============================================================================

#include <cstdint>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

class BitmapInfo {
public:
    // Packed-pixel layout enum. Values match the canonical
    // Aspose.Pdf.BitmapInfo.PixelFormat ordinals.
    enum class PixelFormat {
        Rgb24  = 0,
        Bgr24  = 1,
        Rgba32 = 2,
        Argb32 = 3,
        Bgra32 = 4,
    };

    BitmapInfo(std::vector<std::uint8_t> pixelBytes,
               int width,
               int height,
               PixelFormat format)
        : pixel_bytes_(std::move(pixelBytes)),
          width_(width),
          height_(height),
          format_(format) {}

    const std::vector<std::uint8_t>& PixelBytes() const noexcept { return pixel_bytes_; }
    int         Width()  const noexcept { return width_; }
    int         Height() const noexcept { return height_; }
    PixelFormat Format() const noexcept { return format_; }

private:
    std::vector<std::uint8_t> pixel_bytes_;
    int width_;
    int height_;
    PixelFormat format_;
};

}  // namespace Aspose::Pdf
