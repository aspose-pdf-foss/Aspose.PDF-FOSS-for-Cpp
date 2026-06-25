#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitBVExplicitDestination — an explicit destination
// that fits the bounding box height with the given left coordinate at the
// window left (PDF /FitBV verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitBVExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitBVExplicitDestination : public ExplicitDestination {
public:
    FitBVExplicitDestination(Aspose::Pdf::Page& page, double left);
    FitBVExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                             double left);
    FitBVExplicitDestination(int page_number, double left);

    // Canonical FitBVExplicitDestination.Left.
    double Left() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double left_;
};

}  // namespace Aspose::Pdf::Annotations
