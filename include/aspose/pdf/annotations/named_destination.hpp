#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::NamedDestination — a navigation target referenced
// by name (PDF §12.3.2.3 — a /Dest name resolved through the document's
// /Names /Dests name tree). Mirrors canonical
// Aspose.Pdf.Annotations.NamedDestination (Aspose.PDF 26.4.0): an IAppointment
// that carries the destination name and binds to its owning Document.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/i_appointment.hpp>

namespace Aspose::Pdf {
class Document;
}  // namespace Aspose::Pdf

namespace Aspose::Pdf::Annotations {

class NamedDestination : public IAppointment {
public:
    NamedDestination(Aspose::Pdf::Document& doc, std::string name);

    // Canonical NamedDestination.Name — the /Dest name.
    const std::string& Name() const noexcept;

    // IAppointment — the textual form is the destination name.
    std::string ToString() const override;

protected:
    std::unique_ptr<IAppointment> CloneAppointment() const override;

private:
    Aspose::Pdf::Document* doc_;
    std::string name_;
};

}  // namespace Aspose::Pdf::Annotations
