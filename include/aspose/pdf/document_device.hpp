#pragma once

// =============================================================================
// Aspose::Pdf::Devices::DocumentDevice — abstract multi-page document
// device. Inherits PageDevice's Process(Page, …) and adds four
// Process(Document, …) overloads. The page-range stream variant is
// abstract; the other three are concrete convenience wrappers.
// =============================================================================

#include <ostream>
#include <string>

#include "page_device.hpp"

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Devices {

class DocumentDevice : public PageDevice {
public:
    virtual void Process(const ::Aspose::Pdf::Document& document,
                         int fromPage, int toPage,
                         std::ostream& output) = 0;

    virtual void Process(const ::Aspose::Pdf::Document& document,
                         std::ostream& output);

    virtual void Process(const ::Aspose::Pdf::Document& document,
                         const std::string& outputFileName);

    virtual void Process(const ::Aspose::Pdf::Document& document,
                         int fromPage, int toPage,
                         const std::string& outputFileName);

protected:
    DocumentDevice() = default;
};

}
