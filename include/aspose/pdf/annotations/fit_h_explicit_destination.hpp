#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitHExplicitDestination — an explicit destination
// that fits the page width with the given top coordinate at the window top
// (PDF /FitH verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitHExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitHExplicitDestination : public ExplicitDestination {
public:
    FitHExplicitDestination(Aspose::Pdf::Page& page, double top);
    FitHExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                            double top);
    FitHExplicitDestination(int page_number, double top);

    // Canonical FitHExplicitDestination.Top.
    double Top() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double top_;
};

}  // namespace Aspose::Pdf::Annotations
