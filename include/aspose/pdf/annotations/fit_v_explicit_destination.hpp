#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitVExplicitDestination — an explicit destination
// that fits the page height with the given left coordinate at the window left
// (PDF /FitV verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitVExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitVExplicitDestination : public ExplicitDestination {
public:
    FitVExplicitDestination(Aspose::Pdf::Page& page, double left);
    FitVExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                            double left);
    FitVExplicitDestination(int page_number, double left);

    // Canonical FitVExplicitDestination.Left.
    double Left() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double left_;
};

}  // namespace Aspose::Pdf::Annotations
