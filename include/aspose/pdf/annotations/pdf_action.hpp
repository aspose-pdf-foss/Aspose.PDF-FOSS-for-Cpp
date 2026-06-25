#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PdfAction — abstract base of the action hierarchy.
// Mirrors canonical Aspose.Pdf.Annotations.PdfAction (Aspose.PDF 26.4.0):
// PdfAction is itself an IAppointment. Concrete actions: GoToURIAction
// (URI hyperlink), GoToAction (in-document navigation).
//
// Phased drops (drops.yaml cpp track):
//   * Next (ActionCollection) — action-chaining collection deferred
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/i_appointment.hpp>

namespace Aspose::Pdf {
class OutlineItemCollection;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class PdfAction : public IAppointment {
public:
    ~PdfAction() override = default;

    // Canonical PdfAction.GetECMAScriptString() — the action's ECMAScript
    // (JavaScript) representation. Empty for non-script actions.
    virtual std::string GetECMAScriptString() const;

    // IAppointment — default action text form. Concrete actions refine this.
    std::string ToString() const override;

protected:
    PdfAction() = default;

    // Internal polymorphic copy used by PdfAction-typed properties
    // (LinkAnnotation::Action). Concrete actions implement this; the
    // IAppointment clone hook is satisfied by forwarding to it.
    virtual std::unique_ptr<PdfAction> CloneAction() const = 0;

    std::unique_ptr<IAppointment> CloneAppointment() const override;

    friend class LinkAnnotation;
    friend class Aspose::Pdf::OutlineItemCollection;
};

}  // namespace Aspose::Pdf::Annotations
