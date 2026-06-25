#include <aspose/pdf/annotations/highlight_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

HighlightAnnotation::HighlightAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void HighlightAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType HighlightAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Highlight;
}

}  // namespace Aspose::Pdf::Annotations
