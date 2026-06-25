#include <aspose/pdf/jpeg_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_size.hpp>

#include "dct_encoder.hpp"
#include "page_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <vector>

namespace Aspose::Pdf::Devices {

JpegDevice::JpegDevice() = default;
JpegDevice::JpegDevice(const class Resolution& resolution)
    : ImageDevice(resolution) {}
JpegDevice::JpegDevice(int quality) {
    quality_ = quality;
}
JpegDevice::JpegDevice(const class Resolution& resolution, int quality)
    : ImageDevice(resolution) {
    quality_ = quality;
}
JpegDevice::JpegDevice(int width, int height) : ImageDevice(width, height) {}
JpegDevice::JpegDevice(const ::Aspose::Pdf::PageSize& pageSize)
    : ImageDevice(pageSize) {}
JpegDevice::JpegDevice(int width, int height, const class Resolution& resolution)
    : ImageDevice(width, height, resolution) {}
JpegDevice::JpegDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class Resolution& resolution)
    : ImageDevice(pageSize, resolution) {}
JpegDevice::JpegDevice(int width, int height,
                       const class Resolution& resolution, int quality)
    : ImageDevice(width, height, resolution) {
    quality_ = quality;
}
JpegDevice::JpegDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class Resolution& resolution, int quality)
    : ImageDevice(pageSize, resolution) {
    quality_ = quality;
}

void JpegDevice::Process(const ::Aspose::Pdf::Page& page,
                         std::ostream& output) {
    const auto& bytes = page.doc_->bytes_;
    const double dpi = static_cast<double>(Resolution().X());
    const auto rendered = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        page.leaf_index_,
        dpi);

    foundation::dct_encoder::Options options;
    options.quality = static_cast<std::uint8_t>(quality_);
    options.photometric = foundation::dct_encoder::Photometric::Rgb;
    options.dpi = static_cast<std::uint32_t>(dpi > 0 ? dpi : 72);

    const auto pixels_bytes = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(rendered.pixels.data()),
        rendered.pixels.size());

    const auto encoded = foundation::dct_encoder::Encode(
        rendered.width, rendered.height, pixels_bytes, options);

    output.write(reinterpret_cast<const char*>(encoded.data()),
                 static_cast<std::streamsize>(encoded.size()));
}

}  // namespace Aspose::Pdf::Devices
