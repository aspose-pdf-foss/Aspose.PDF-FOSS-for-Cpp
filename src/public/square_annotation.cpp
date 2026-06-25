#include <aspose/pdf/annotations/square_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

SquareAnnotation::SquareAnnotation(Aspose::Pdf::Document& document)
    : CommonFigureAnnotation(document) {}

SquareAnnotation::SquareAnnotation(Aspose::Pdf::Page& page,
                                    Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void SquareAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType SquareAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Square;
}

}  // namespace Aspose::Pdf::Annotations
