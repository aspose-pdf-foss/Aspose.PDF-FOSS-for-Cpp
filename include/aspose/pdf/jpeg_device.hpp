#pragma once

// =============================================================================
// Aspose::Pdf::Devices::JpegDevice — concrete sealed ImageDevice
// emitting baseline JFIF JPEG. DLL surface (Aspose.PDF 26.4.0):
// 10 ctors (quality is a ctor parameter, no Quality property),
// Process(Page, ostream) override.
// =============================================================================

#include <ostream>

#include "image_device.hpp"

namespace Aspose::Pdf {
class Page;
class PageSize;
}

namespace Aspose::Pdf::Devices {

class JpegDevice final : public ImageDevice {
public:
    JpegDevice();
    explicit JpegDevice(const class Resolution& resolution);
    explicit JpegDevice(int quality);
    JpegDevice(const class Resolution& resolution, int quality);
    JpegDevice(int width, int height);
    explicit JpegDevice(const ::Aspose::Pdf::PageSize& pageSize);
    JpegDevice(int width, int height, const class Resolution& resolution);
    JpegDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class Resolution& resolution);
    JpegDevice(int width, int height,
               const class Resolution& resolution, int quality);
    JpegDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class Resolution& resolution, int quality);

    void Process(const ::Aspose::Pdf::Page& page,
                 std::ostream& output) override;

private:
    int quality_ = 75;
};

}
