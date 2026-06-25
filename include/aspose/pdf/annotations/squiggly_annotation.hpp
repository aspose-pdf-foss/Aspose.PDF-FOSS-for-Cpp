#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::SquigglyAnnotation — squiggly-underline
// markup (PDF /Subtype /Squiggly per §12.5.6.10). Mirrors
// canonical Aspose.Pdf.Annotations.SquigglyAnnotation; extends
// TextMarkupAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/text_markup_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class SquigglyAnnotation : public TextMarkupAnnotation {
public:
    SquigglyAnnotation(Aspose::Pdf::Page& page,
                       Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
