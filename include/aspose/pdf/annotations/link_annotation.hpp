#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::LinkAnnotation — hyperlink annotation.
// Mirrors canonical Aspose.Pdf.Annotations.LinkAnnotation
// (Aspose.PDF 26.4.0); extends Annotation directly (not
// MarkupAnnotation — hyperlinks aren't review markup).
// =============================================================================

#include <memory>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/highlighting_mode.hpp>
#include <aspose/pdf/annotations/i_appointment.hpp>
#include <aspose/pdf/annotations/pdf_action.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class LinkAnnotation : public Annotation {
public:
    LinkAnnotation(Aspose::Pdf::Page& page,
                   Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    // Canonical LinkAnnotation.Action — the action triggered when the link is
    // activated (e.g. a GoToURIAction for a hyperlink, a GoToAction for an
    // in-document jump). Stored by owning pointer (canonical .NET reference
    // semantics); the setter clones the supplied action. Getter returns a
    // non-owning pointer, nullptr when no action is set.
    PdfAction* Action() const noexcept;
    void Action(const PdfAction& value);

    // Canonical LinkAnnotation.Destination — an in-document navigation target.
    // Same owning-pointer storage as Action.
    IAppointment* Destination() const noexcept;
    void Destination(const IAppointment& value);

    HighlightingMode Highlighting() const noexcept;
    void Highlighting(HighlightingMode value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    HighlightingMode highlighting_ = HighlightingMode::Invert;
    std::unique_ptr<PdfAction> action_;
    std::unique_ptr<IAppointment> destination_;
};

}  // namespace Aspose::Pdf::Annotations
