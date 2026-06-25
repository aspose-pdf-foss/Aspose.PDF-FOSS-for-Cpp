#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::HighlightAnnotation — text-highlight
// markup (PDF /Subtype /Highlight per §12.5.6.10). Mirrors
// canonical Aspose.Pdf.Annotations.HighlightAnnotation; extends
// TextMarkupAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/text_markup_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class HighlightAnnotation : public TextMarkupAnnotation {
public:
    HighlightAnnotation(Aspose::Pdf::Page& page,
                        Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
