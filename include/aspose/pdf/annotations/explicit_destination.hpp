#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::ExplicitDestination — an in-document navigation
// target. Mirrors canonical Aspose.Pdf.Annotations.ExplicitDestination
// (Aspose.PDF 26.4.0): an IAppointment exposing a read-only PageNumber.
//
// Canonical ExplicitDestination has no public constructor (it is produced by
// the navigation API — e.g. GoToAction) and is the base of the typed
// destination family (XYZExplicitDestination, FitExplicitDestination, …).
// The page-number-taking constructor here is protected: it is used by
// GoToAction and by the subclasses, and is not part of the public surface.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/i_appointment.hpp>

namespace Aspose::Pdf::Annotations {

class ExplicitDestination : public IAppointment {
public:
    ~ExplicitDestination() override = default;

    // Canonical ExplicitDestination.PageNumber — the 1-based target page.
    int PageNumber() const noexcept;

    // IAppointment
    std::string ToString() const override;

protected:
    // Subclass / internal construction with a 1-based target page number.
    // Not part of the canonical public surface — canonical destinations are
    // produced via the typed subclasses (XYZExplicitDestination, …) or by
    // GoToAction.
    explicit ExplicitDestination(int page_number) noexcept;

    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    int page_number_ = 0;

    friend class GoToAction;
};

}  // namespace Aspose::Pdf::Annotations
