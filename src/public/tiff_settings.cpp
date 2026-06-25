#include <aspose/pdf/tiff_settings.hpp>

namespace Aspose::Pdf::Devices {

TiffSettings::TiffSettings() = default;

TiffSettings::TiffSettings(ShapeType shapeType) { shape_ = shapeType; }
TiffSettings::TiffSettings(CompressionType compressionType) { compression_ = compressionType; }
TiffSettings::TiffSettings(ColorDepth colorDepth) { depth_ = colorDepth; }
TiffSettings::TiffSettings(const ::Aspose::Pdf::Devices::Margins& m) { margins_ = m; }

TiffSettings::TiffSettings(CompressionType c, ColorDepth d,
                           const ::Aspose::Pdf::Devices::Margins& m)
    : margins_(m), compression_(c), depth_(d) {}

TiffSettings::TiffSettings(CompressionType c, ColorDepth d,
                           const ::Aspose::Pdf::Devices::Margins& m,
                           bool skipBlankPages)
    : margins_(m), skip_blank_pages_(skipBlankPages),
      compression_(c), depth_(d) {}

TiffSettings::TiffSettings(CompressionType c, ColorDepth d,
                           const ::Aspose::Pdf::Devices::Margins& m,
                           bool skipBlankPages, ShapeType s)
    : margins_(m), skip_blank_pages_(skipBlankPages),
      compression_(c), depth_(d), shape_(s) {}

TiffSettings::TiffSettings(bool skipBlankPages) {
    skip_blank_pages_ = skipBlankPages;
}

const ::Aspose::Pdf::Devices::Margins& TiffSettings::Margins() const noexcept { return margins_; }

bool TiffSettings::SkipBlankPages() const noexcept { return skip_blank_pages_; }
void TiffSettings::SkipBlankPages(bool value) noexcept { skip_blank_pages_ = value; }

::Aspose::Pdf::Devices::CompressionType TiffSettings::Compression() const noexcept { return compression_; }
void TiffSettings::Compression(::Aspose::Pdf::Devices::CompressionType v) noexcept { compression_ = v; }

::Aspose::Pdf::Devices::ColorDepth TiffSettings::Depth() const noexcept { return depth_; }
void TiffSettings::Depth(::Aspose::Pdf::Devices::ColorDepth v) noexcept { depth_ = v; }

::Aspose::Pdf::Devices::ShapeType TiffSettings::Shape() const noexcept { return shape_; }
void TiffSettings::Shape(::Aspose::Pdf::Devices::ShapeType v) noexcept { shape_ = v; }

float TiffSettings::Brightness() const noexcept { return brightness_; }
void TiffSettings::Brightness(float v) noexcept { brightness_ = v; }

::Aspose::Pdf::PageCoordinateType TiffSettings::CoordinateType() const noexcept { return coordinate_type_; }
void TiffSettings::CoordinateType(::Aspose::Pdf::PageCoordinateType v) noexcept { coordinate_type_ = v; }

}  // namespace Aspose::Pdf::Devices
