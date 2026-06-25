#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitBExplicitDestination — an explicit destination
// that fits the page's bounding box in the window (PDF /FitB verb per
// §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitBExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitBExplicitDestination : public ExplicitDestination {
public:
    explicit FitBExplicitDestination(Aspose::Pdf::Page& page);
    FitBExplicitDestination(Aspose::Pdf::Document& document, int page_number);
    explicit FitBExplicitDestination(int page_number);

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;
};

}  // namespace Aspose::Pdf::Annotations
