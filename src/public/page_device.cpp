#include <aspose/pdf/page_device.hpp>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <system_error>

namespace Aspose::Pdf::Devices {

void PageDevice::Process(const ::Aspose::Pdf::Page& page,
                         const std::string& outputFileName) {
    std::ofstream out(outputFileName, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Devices::PageDevice::Process: cannot open '" +
                outputFileName + "' for writing");
    }
    Process(page, out);
    if (!out) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Devices::PageDevice::Process: write failed on '" +
                outputFileName + "'");
    }
}

}  // namespace Aspose::Pdf::Devices
