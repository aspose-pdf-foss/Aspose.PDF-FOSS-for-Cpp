#include <aspose/pdf/annotations/strike_out_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

StrikeOutAnnotation::StrikeOutAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void StrikeOutAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType StrikeOutAnnotation::TypeImpl() const noexcept {
    return AnnotationType::StrikeOut;
}

}  // namespace Aspose::Pdf::Annotations
