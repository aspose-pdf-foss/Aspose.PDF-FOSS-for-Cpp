#include <aspose/pdf/annotations/underline_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

UnderlineAnnotation::UnderlineAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void UnderlineAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType UnderlineAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Underline;
}

}  // namespace Aspose::Pdf::Annotations
