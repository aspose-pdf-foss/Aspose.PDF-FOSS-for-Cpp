#pragma once

// =============================================================================
// Aspose::Pdf::Devices::PageDevice — abstract single-page device base.
// Concrete subclasses (PngDevice, JpegDevice, BmpDevice, TextDevice)
// override Process(page, stream); the file-path overload here is a
// concrete convenience wrapper.
// =============================================================================

#include <ostream>
#include <string>

#include "device.hpp"

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Devices {

class PageDevice : public Device {
public:
    virtual void Process(const ::Aspose::Pdf::Page& page,
                         std::ostream& output) = 0;

    virtual void Process(const ::Aspose::Pdf::Page& page,
                         const std::string& outputFileName);

protected:
    PageDevice() = default;
};

}
