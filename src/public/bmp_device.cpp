#include <aspose/pdf/bmp_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_size.hpp>

#include "bmp_encoder.hpp"
#include "page_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>

namespace Aspose::Pdf::Devices {

BmpDevice::BmpDevice() = default;
BmpDevice::BmpDevice(const class Resolution& resolution)
    : ImageDevice(resolution) {}
BmpDevice::BmpDevice(int width, int height, const class Resolution& resolution)
    : ImageDevice(width, height, resolution) {}
BmpDevice::BmpDevice(const ::Aspose::Pdf::PageSize& pageSize,
                     const class Resolution& resolution)
    : ImageDevice(pageSize, resolution) {}
BmpDevice::BmpDevice(int width, int height) : ImageDevice(width, height) {}
BmpDevice::BmpDevice(const ::Aspose::Pdf::PageSize& pageSize)
    : ImageDevice(pageSize) {}

void BmpDevice::Process(const ::Aspose::Pdf::Page& page,
                        std::ostream& output) {
    const auto& bytes = page.doc_->bytes_;
    const double dpi = static_cast<double>(Resolution().X());
    const auto rendered = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        page.leaf_index_,
        dpi);

    // Public BmpDevice emits the 24-bit BI_RGB variant — alpha
    // is dropped on encode. The 32-bit BI_BITFIELDS variant is
    // available on the foundation primitive but not surfaced
    // here (canonical Aspose.PDF.Devices.BmpDevice has no
    // pixel-format property).
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;

    const auto encoded = foundation::bmp_encoder::Encode(
        rendered.width,
        rendered.height,
        std::as_bytes(std::span<const std::uint8_t>(rendered.pixels)),
        opts);

    output.write(reinterpret_cast<const char*>(encoded.data()),
                 static_cast<std::streamsize>(encoded.size()));
}

}  // namespace Aspose::Pdf::Devices
