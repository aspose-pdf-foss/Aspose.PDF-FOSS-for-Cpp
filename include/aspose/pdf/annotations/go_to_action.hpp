#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::GoToAction — an in-document navigation action.
// Mirrors canonical Aspose.Pdf.Annotations.GoToAction (Aspose.PDF 26.4.0):
// a PdfAction whose Destination is an in-document target (a page). Used as
// LinkAnnotation::Action to make a link jump to another page. The
// (Page, ExplicitDestinationType, double[]) constructor builds the matching
// typed destination (XYZExplicitDestination, FitHExplicitDestination, …) from
// the fit-mode verb and its coordinate operands.
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/explicit_destination.hpp>
#include <aspose/pdf/annotations/explicit_destination_type.hpp>
#include <aspose/pdf/annotations/i_appointment.hpp>
#include <aspose/pdf/annotations/pdf_action.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class GoToAction : public PdfAction {
public:
    GoToAction();
    explicit GoToAction(int page);
    explicit GoToAction(Aspose::Pdf::Page& page);
    GoToAction(Aspose::Pdf::Page& page, ExplicitDestinationType type,
               const std::vector<double>& values);
    explicit GoToAction(const ExplicitDestination& destination);
    GoToAction(Aspose::Pdf::Document& doc, std::string name);

    // Canonical GoToAction.Destination — the navigation target. For a
    // page-based action this is an ExplicitDestination; the getter returns a
    // non-owning pointer (nullptr for a named/empty destination).
    IAppointment* Destination() const noexcept;
    void Destination(const IAppointment& value);

    std::string GetECMAScriptString() const override;
    std::string ToString() const override;

protected:
    std::unique_ptr<PdfAction> CloneAction() const override;

private:
    std::unique_ptr<IAppointment> destination_;
    std::string named_destination_;
};

}  // namespace Aspose::Pdf::Annotations
