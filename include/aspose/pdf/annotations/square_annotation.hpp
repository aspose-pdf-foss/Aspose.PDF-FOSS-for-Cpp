#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::SquareAnnotation — rectangle annotation
// (PDF /Subtype /Square per §12.5.6.8). Mirrors canonical
// Aspose.Pdf.Annotations.SquareAnnotation; extends
// CommonFigureAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/common_figure_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class SquareAnnotation : public CommonFigureAnnotation {
public:
    explicit SquareAnnotation(Aspose::Pdf::Document& document);
    SquareAnnotation(Aspose::Pdf::Page& page,
                     Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
