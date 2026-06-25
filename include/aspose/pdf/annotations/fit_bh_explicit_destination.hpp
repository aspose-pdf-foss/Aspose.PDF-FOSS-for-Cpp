#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitBHExplicitDestination — an explicit destination
// that fits the bounding box width with the given top coordinate at the
// window top (PDF /FitBH verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitBHExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitBHExplicitDestination : public ExplicitDestination {
public:
    FitBHExplicitDestination(Aspose::Pdf::Page& page, double top);
    FitBHExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                             double top);
    FitBHExplicitDestination(int page_number, double top);

    // Canonical FitBHExplicitDestination.Top.
    double Top() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double top_;
};

}  // namespace Aspose::Pdf::Annotations
