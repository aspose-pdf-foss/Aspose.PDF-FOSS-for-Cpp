#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PopupAnnotation — pop-up annotation
// that displays text in a floating window associated with another
// markup annotation (PDF /Subtype /Popup per §12.5.6.14). Mirrors
// canonical Aspose.Pdf.Annotations.PopupAnnotation; extends
// Annotation directly (not MarkupAnnotation — the popup itself
// doesn't carry author/review metadata; it's the visual carrier
// for its Parent markup annotation).
// =============================================================================

#include <memory>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class PopupAnnotation : public Annotation {
public:
    explicit PopupAnnotation(Aspose::Pdf::Document& document);
    PopupAnnotation(Aspose::Pdf::Page& page,
                    Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    bool Open() const noexcept;
    void Open(bool value) noexcept;

    // Parent is a back-reference to the markup annotation this
    // popup belongs to. Held by shared_ptr to support polymorphic
    // Annotation subclasses without slicing. nullptr means
    // unattached.
    const std::shared_ptr<Annotation>& Parent() const noexcept;
    void Parent(std::shared_ptr<Annotation> value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    bool open_ = false;
    std::shared_ptr<Annotation> parent_;
};

}  // namespace Aspose::Pdf::Annotations
