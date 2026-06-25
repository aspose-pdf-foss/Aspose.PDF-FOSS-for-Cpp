#include <aspose/pdf/image_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/rendering_options.hpp>

#include "page_renderer.hpp"

#include <span>

namespace Aspose::Pdf::Devices {

ImageDevice::ImageDevice() = default;

ImageDevice::ImageDevice(const class Resolution& resolution) {
    resolution_ = resolution;
}

ImageDevice::ImageDevice(int width, int height) {
    width_ = width;
    height_ = height;
}

ImageDevice::ImageDevice(const ::Aspose::Pdf::PageSize& pageSize) {
    width_ = static_cast<int>(pageSize.Width());
    height_ = static_cast<int>(pageSize.Height());
}

ImageDevice::ImageDevice(int width, int height,
                         const class Resolution& resolution) {
    width_ = width;
    height_ = height;
    resolution_ = resolution;
}

ImageDevice::ImageDevice(const ::Aspose::Pdf::PageSize& pageSize,
                         const class Resolution& resolution) {
    width_ = static_cast<int>(pageSize.Width());
    height_ = static_cast<int>(pageSize.Height());
    resolution_ = resolution;
}

::Aspose::Pdf::BitmapInfo
ImageDevice::GetBitmap(const ::Aspose::Pdf::Page& page) {
    const auto& bytes = page.doc_->bytes_;
    const double dpi = static_cast<double>(resolution_.X());
    auto rendered = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        page.leaf_index_, dpi);
    return ::Aspose::Pdf::BitmapInfo(
        std::move(rendered.pixels),
        static_cast<int>(rendered.width),
        static_cast<int>(rendered.height),
        ::Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
}

::Aspose::Pdf::PageCoordinateType ImageDevice::CoordinateType() const noexcept {
    return coordinate_type_;
}
void ImageDevice::CoordinateType(::Aspose::Pdf::PageCoordinateType value) noexcept {
    coordinate_type_ = value;
}

::Aspose::Pdf::RenderingOptions& ImageDevice::RenderingOptions() {
    return rendering_options_;
}
void ImageDevice::RenderingOptions(const ::Aspose::Pdf::RenderingOptions& value) {
    rendering_options_ = value;
}

::Aspose::Pdf::Devices::FormPresentationMode
ImageDevice::FormPresentationMode() const noexcept {
    return form_presentation_mode_;
}
void ImageDevice::FormPresentationMode(
        ::Aspose::Pdf::Devices::FormPresentationMode value) noexcept {
    form_presentation_mode_ = value;
}

const class Resolution& ImageDevice::Resolution() const noexcept {
    return resolution_;
}

int ImageDevice::Width() const noexcept { return width_; }
int ImageDevice::Height() const noexcept { return height_; }

}  // namespace Aspose::Pdf::Devices
