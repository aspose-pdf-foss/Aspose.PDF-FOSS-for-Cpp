#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::XYZExplicitDestination — an explicit destination
// that positions a page at a given (left, top) point and zoom factor
// (PDF /XYZ verb per §12.3.2.2). Mirrors canonical
// Aspose.Pdf.Annotations.XYZExplicitDestination (Aspose.PDF 26.4.0).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/explicit_destination.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class XYZExplicitDestination : public ExplicitDestination {
public:
    XYZExplicitDestination(Aspose::Pdf::Page& page, double left, double top,
                           double zoom);
    XYZExplicitDestination(Aspose::Pdf::Document& document, int page_number,
                           double left, double top, double zoom);
    XYZExplicitDestination(int page_number, double left, double top,
                           double zoom);

    // Canonical XYZExplicitDestination.Left / Top / Zoom.
    double Left() const noexcept;
    double Top() const noexcept;
    double Zoom() const noexcept;

    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    double left_;
    double top_;
    double zoom_;
};

}  // namespace Aspose::Pdf::Annotations
