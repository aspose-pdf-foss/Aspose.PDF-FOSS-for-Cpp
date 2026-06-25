#pragma once

// =============================================================================
// Aspose::Pdf::IIndexBitmapConverter — pluggable strategy turning a
// packed RGBA bitmap into a palette-quantised bitmap for indexed-
// colour TIFF output. Consumed by TiffDevice ctors that accept a
// custom converter.
//
// DLL surface (Aspose.PDF 26.4.0): public interface, 3 abstract
// methods returning System.Drawing.Bitmap.
//
// C++ port substitutes Aspose::Pdf::BitmapInfo (the cross-platform
// flat POD shipped in place of BCL Bitmap) for System.Drawing.Bitmap.
// Implementers receive a Rgba32-format BitmapInfo and return a
// BitmapInfo whose pixels carry at most 2 / 16 / 256 distinct RGB
// triples for the 1 / 4 / 8 bpp methods respectively. TiffDevice
// reconstructs the palette + per-pixel index buffer from the
// returned bytes.
// =============================================================================

#include <memory>

#include "bitmap_info.hpp"

namespace Aspose::Pdf {

class IIndexBitmapConverter {
public:
    virtual ~IIndexBitmapConverter() = default;

    // Quantise the input RGBA bitmap to ≤ 2 distinct RGB colours
    // (1-bit-per-pixel target).
    virtual std::unique_ptr<BitmapInfo>
        Get1BppImage(const BitmapInfo& src) = 0;

    // Quantise the input RGBA bitmap to ≤ 16 distinct RGB colours
    // (4-bit-per-pixel target, 16-colour palette).
    virtual std::unique_ptr<BitmapInfo>
        Get4BppImage(const BitmapInfo& src) = 0;

    // Quantise the input RGBA bitmap to ≤ 256 distinct RGB colours
    // (8-bit-per-pixel target, 256-colour palette).
    virtual std::unique_ptr<BitmapInfo>
        Get8BppImage(const BitmapInfo& src) = 0;
};

}  // namespace Aspose::Pdf
