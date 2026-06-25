#include <aspose/pdf/png_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_size.hpp>

#include "page_renderer.hpp"
#include "png_encoder.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>

namespace Aspose::Pdf::Devices {

PngDevice::PngDevice() = default;
PngDevice::PngDevice(const class Resolution& resolution)
    : ImageDevice(resolution) {}
PngDevice::PngDevice(int width, int height,
                     const class Resolution& resolution)
    : ImageDevice(width, height, resolution) {}
PngDevice::PngDevice(const ::Aspose::Pdf::PageSize& pageSize,
                     const class Resolution& resolution)
    : ImageDevice(pageSize, resolution) {}
PngDevice::PngDevice(int width, int height)
    : ImageDevice(width, height) {}
PngDevice::PngDevice(const ::Aspose::Pdf::PageSize& pageSize)
    : ImageDevice(pageSize) {}

void PngDevice::Process(const ::Aspose::Pdf::Page& page,
                        std::ostream& output) {
    const auto& bytes = page.doc_->bytes_;
    const double dpi = static_cast<double>(Resolution().X());
    // TransparentBackground passes through to the renderer's
    // canvas-init step (0,0,0,0 vs 255,255,255,255) — matches
    // python / csharp peer behaviour. Avoids the
    // white-pixel-to-alpha-zero post-pass that would clobber
    // intentional white page content.
    auto rendered = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        page.leaf_index_,
        dpi,
        TransparentBackground());

    // Public PngDevice emits the Rgba (color type 6) variant.
    // Matches csharp's PngDevice (which passes new Options() →
    // default Rgba) and the pre-existing inline encoder.
    foundation::png_encoder::Options opts;
    opts.color_type = foundation::png_encoder::ColorType::Rgba;

    const auto encoded = foundation::png_encoder::Encode(
        rendered.width,
        rendered.height,
        std::as_bytes(std::span<const std::uint8_t>(rendered.pixels)),
        opts);

    output.write(reinterpret_cast<const char*>(encoded.data()),
                 static_cast<std::streamsize>(encoded.size()));
}

bool PngDevice::TransparentBackground() const noexcept {
    return transparent_background_;
}
void PngDevice::TransparentBackground(bool value) noexcept {
    transparent_background_ = value;
}

}  // namespace Aspose::Pdf::Devices
