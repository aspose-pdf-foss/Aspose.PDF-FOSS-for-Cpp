#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FitRExplicitDestination — an explicit destination
// that fits the rectangle (left, bottom, right, top) in the window
// (PDF /FitR verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.FitRExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class FitRExplicitDestination : public ExplicitDestination {
public:
    FitRExplicitDestination(Aspose::Pdf::Page& page, double left, double bottom,
                            double right, double top);
    FitRExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                            double left, double bottom, double right,
                            double top);
    FitRExplicitDestination(int page_number, double left, double bottom,
                            double right, double top);

    // Canonical FitRExplicitDestination.Left / Bottom / Right / Top.
    double Left() const noexcept;
    double Bottom() const noexcept;
    double Right() const noexcept;
    double Top() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double left_;
    double bottom_;
    double right_;
    double top_;
};

}  // namespace Aspose::Pdf::Annotations
