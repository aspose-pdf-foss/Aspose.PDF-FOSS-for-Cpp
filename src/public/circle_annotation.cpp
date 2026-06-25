#include <aspose/pdf/annotations/circle_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

CircleAnnotation::CircleAnnotation(Aspose::Pdf::Document& document)
    : CommonFigureAnnotation(document) {}

CircleAnnotation::CircleAnnotation(Aspose::Pdf::Page& page,
                                    Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void CircleAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType CircleAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Circle;
}

}  // namespace Aspose::Pdf::Annotations
