#pragma once

// =============================================================================
// Aspose::Pdf::Devices::BmpDevice — concrete sealed ImageDevice
// emitting uncompressed BMP. DLL surface (Aspose.PDF 26.4.0):
// 6 ctors, Process(Page, ostream) override, no own properties.
// =============================================================================

#include <ostream>

#include "image_device.hpp"

namespace Aspose::Pdf {
class Page;
class PageSize;
}

namespace Aspose::Pdf::Devices {

class BmpDevice final : public ImageDevice {
public:
    BmpDevice();
    explicit BmpDevice(const class Resolution& resolution);
    BmpDevice(int width, int height, const class Resolution& resolution);
    BmpDevice(const ::Aspose::Pdf::PageSize& pageSize,
              const class Resolution& resolution);
    BmpDevice(int width, int height);
    explicit BmpDevice(const ::Aspose::Pdf::PageSize& pageSize);

    void Process(const ::Aspose::Pdf::Page& page,
                 std::ostream& output) override;
};

}
