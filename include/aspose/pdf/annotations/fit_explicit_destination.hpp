#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitExplicitDestination — an explicit destination
// that fits the whole page in the window (PDF /Fit verb per §12.3.2.2).
// Mirrors canonical Aspose.Pdf.Annotations.FitExplicitDestination
// (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitExplicitDestination : public ExplicitDestination {
public:
    explicit FitExplicitDestination(Aspose::Pdf::Page& page);
    FitExplicitDestination(Aspose::Pdf::Document& document, int page_number);
    explicit FitExplicitDestination(int page_number);

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;
};

}  // namespace Aspose::Pdf::Annotations
