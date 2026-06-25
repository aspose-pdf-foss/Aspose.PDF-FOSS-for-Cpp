#pragma once

// =============================================================================
// Aspose::Pdf::Devices::PngDevice — concrete sealed ImageDevice
// emitting PNG. DLL surface (Aspose.PDF 26.4.0): 6 ctors,
// Process(Page, ostream) override, TransparentBackground rw bool.
// =============================================================================

#include <ostream>

#include "image_device.hpp"

namespace Aspose::Pdf {
class Page;
class PageSize;
}

namespace Aspose::Pdf::Devices {

class PngDevice final : public ImageDevice {
public:
    PngDevice();
    explicit PngDevice(const class Resolution& resolution);
    PngDevice(int width, int height, const class Resolution& resolution);
    PngDevice(const ::Aspose::Pdf::PageSize& pageSize,
              const class Resolution& resolution);
    PngDevice(int width, int height);
    explicit PngDevice(const ::Aspose::Pdf::PageSize& pageSize);

    void Process(const ::Aspose::Pdf::Page& page,
                 std::ostream& output) override;

    bool TransparentBackground() const noexcept;
    void TransparentBackground(bool value) noexcept;

private:
    bool transparent_background_ = false;
};

}
