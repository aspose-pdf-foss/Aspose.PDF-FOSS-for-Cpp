#pragma once

// =============================================================================
// Aspose::Pdf::Devices::ImageDevice — abstract raster-image device.
// Carries the rendering surface (size, resolution, options) shared
// by PngDevice / JpegDevice / BmpDevice; subclasses add format-
// specific encoding via Process(Page, ostream).
// =============================================================================

#include "bitmap_info.hpp"
#include "form_presentation_mode.hpp"
#include "page_coordinate_type.hpp"
#include "page_device.hpp"
#include "rendering_options.hpp"
#include "resolution.hpp"

namespace Aspose::Pdf {
class Page;
class PageSize;
}

namespace Aspose::Pdf::Devices {

class ImageDevice : public PageDevice {
public:
    ImageDevice();
    explicit ImageDevice(const Resolution& resolution);
    ImageDevice(int width, int height);
    explicit ImageDevice(const ::Aspose::Pdf::PageSize& pageSize);
    ImageDevice(int width, int height, const Resolution& resolution);
    ImageDevice(const ::Aspose::Pdf::PageSize& pageSize,
                const Resolution& resolution);

    // Rasterise ``page`` and return the resulting bitmap. The DPI
    // used for rasterisation is Resolution() when set, otherwise 72.
    ::Aspose::Pdf::BitmapInfo GetBitmap(const ::Aspose::Pdf::Page& page);

    ::Aspose::Pdf::PageCoordinateType CoordinateType() const noexcept;
    void CoordinateType(::Aspose::Pdf::PageCoordinateType value) noexcept;

    ::Aspose::Pdf::RenderingOptions& RenderingOptions();
    void RenderingOptions(const ::Aspose::Pdf::RenderingOptions& value);

    ::Aspose::Pdf::Devices::FormPresentationMode
        FormPresentationMode() const noexcept;
    void FormPresentationMode(
        ::Aspose::Pdf::Devices::FormPresentationMode value) noexcept;

    const class Resolution& Resolution() const noexcept;

    int Width() const noexcept;
    int Height() const noexcept;

protected:
    int width_ = 0;
    int height_ = 0;
    class Resolution resolution_{72};
    ::Aspose::Pdf::PageCoordinateType coordinate_type_ =
        ::Aspose::Pdf::PageCoordinateType::CropBox;
    ::Aspose::Pdf::Devices::FormPresentationMode form_presentation_mode_ =
        ::Aspose::Pdf::Devices::FormPresentationMode::Production;
    ::Aspose::Pdf::RenderingOptions rendering_options_{};
};

}
