#pragma once

// =============================================================================
// Aspose::Pdf::Devices::TiffSettings — TIFF-specific encoding knobs
// consumed by TiffDevice. DLL surface (Aspose.PDF 26.4.0): sealed,
// 9 ctors + 7 properties (Margins read-only, the rest read-write).
// =============================================================================

#include "color_depth.hpp"
#include "compression_type.hpp"
#include "margins.hpp"
#include "shape_type.hpp"
#include "page_coordinate_type.hpp"

namespace Aspose::Pdf::Devices {

class TiffSettings {
public:
    TiffSettings();
    explicit TiffSettings(ShapeType shapeType);
    explicit TiffSettings(CompressionType compressionType);
    explicit TiffSettings(ColorDepth colorDepth);
    explicit TiffSettings(const Margins& margins);
    TiffSettings(CompressionType compressionType,
                 ColorDepth colorDepth,
                 const Margins& margins);
    TiffSettings(CompressionType compressionType,
                 ColorDepth colorDepth,
                 const Margins& margins,
                 bool skipBlankPages);
    TiffSettings(CompressionType compressionType,
                 ColorDepth colorDepth,
                 const Margins& margins,
                 bool skipBlankPages,
                 ShapeType shapeType);
    explicit TiffSettings(bool skipBlankPages);

    const ::Aspose::Pdf::Devices::Margins& Margins() const noexcept;

    bool SkipBlankPages() const noexcept;
    void SkipBlankPages(bool value) noexcept;

    ::Aspose::Pdf::Devices::CompressionType Compression() const noexcept;
    void Compression(::Aspose::Pdf::Devices::CompressionType value) noexcept;

    ::Aspose::Pdf::Devices::ColorDepth Depth() const noexcept;
    void Depth(::Aspose::Pdf::Devices::ColorDepth value) noexcept;

    ::Aspose::Pdf::Devices::ShapeType Shape() const noexcept;
    void Shape(::Aspose::Pdf::Devices::ShapeType value) noexcept;

    float Brightness() const noexcept;
    void Brightness(float value) noexcept;

    ::Aspose::Pdf::PageCoordinateType CoordinateType() const noexcept;
    void CoordinateType(::Aspose::Pdf::PageCoordinateType value) noexcept;

private:
    ::Aspose::Pdf::Devices::Margins margins_{};
    bool skip_blank_pages_ = false;
    ::Aspose::Pdf::Devices::CompressionType compression_ =
        ::Aspose::Pdf::Devices::CompressionType::LZW;
    ::Aspose::Pdf::Devices::ColorDepth depth_ =
        ::Aspose::Pdf::Devices::ColorDepth::Default;
    ::Aspose::Pdf::Devices::ShapeType shape_ =
        ::Aspose::Pdf::Devices::ShapeType::None;
    float brightness_ = 0.5f;
    ::Aspose::Pdf::PageCoordinateType coordinate_type_ =
        ::Aspose::Pdf::PageCoordinateType::CropBox;
};

}
