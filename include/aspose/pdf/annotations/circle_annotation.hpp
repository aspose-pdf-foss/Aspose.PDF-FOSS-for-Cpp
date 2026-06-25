#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::CircleAnnotation — ellipse annotation
// (PDF /Subtype /Circle per §12.5.6.8). Mirrors canonical
// Aspose.Pdf.Annotations.CircleAnnotation; extends
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

class CircleAnnotation : public CommonFigureAnnotation {
public:
    explicit CircleAnnotation(Aspose::Pdf::Document& document);
    CircleAnnotation(Aspose::Pdf::Page& page,
                     Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
