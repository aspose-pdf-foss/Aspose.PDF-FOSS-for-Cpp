#include <aspose/pdf/document_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <fstream>
#include <ios>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace Aspose::Pdf::Devices {

void DocumentDevice::Process(const ::Aspose::Pdf::Document& document,
                             std::ostream& output) {
    auto& self = const_cast<::Aspose::Pdf::Document&>(document);
    const int last = static_cast<int>(self.Pages().Count());
    Process(document, /*fromPage=*/1, /*toPage=*/last, output);
}

void DocumentDevice::Process(const ::Aspose::Pdf::Document& document,
                             const std::string& outputFileName) {
    std::ofstream out(outputFileName, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Devices::DocumentDevice::Process: cannot open '" +
                outputFileName + "' for writing");
    }
    Process(document, out);
}

void DocumentDevice::Process(const ::Aspose::Pdf::Document& document,
                             int fromPage, int toPage,
                             const std::string& outputFileName) {
    std::ofstream out(outputFileName, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Devices::DocumentDevice::Process: cannot open '" +
                outputFileName + "' for writing");
    }
    Process(document, fromPage, toPage, out);
}

}  // namespace Aspose::Pdf::Devices
