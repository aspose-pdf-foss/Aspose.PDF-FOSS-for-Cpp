#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::IAppointment — navigation-target interface.
// Mirrors canonical Aspose.Pdf.Annotations.IAppointment (Aspose.PDF 26.4.0):
// the common interface implemented by destinations (ExplicitDestination) and
// actions (PdfAction). Surfaces a single canonical member, ToString().
// =============================================================================

#include <memory>
#include <string>

namespace Aspose::Pdf {
class NamedDestinationCollection;
class OutlineItemCollection;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class GoToAction;
class LinkAnnotation;

class IAppointment {
public:
    virtual ~IAppointment() = default;

    // Canonical IAppointment.ToString() — the PDF/textual form of the
    // navigation target (e.g. a destination string or action description).
    virtual std::string ToString() const = 0;

protected:
    IAppointment() = default;

    // Internal polymorphic copy. Properties typed as IAppointment store the
    // value by owning pointer (canonical .NET reference semantics); the
    // setter clones through this hook. Not part of the canonical surface —
    // protected + friended to the storing types only.
    virtual std::unique_ptr<IAppointment> CloneAppointment() const = 0;

    friend class GoToAction;
    friend class LinkAnnotation;
    friend class Aspose::Pdf::NamedDestinationCollection;
    friend class Aspose::Pdf::OutlineItemCollection;
};

}  // namespace Aspose::Pdf::Annotations
