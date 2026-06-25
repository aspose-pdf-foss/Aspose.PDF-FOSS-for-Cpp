#include <aspose/pdf/annotations/squiggly_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

SquigglyAnnotation::SquigglyAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void SquigglyAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType SquigglyAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Squiggly;
}

}  // namespace Aspose::Pdf::Annotations
