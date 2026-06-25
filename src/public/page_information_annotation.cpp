#include <aspose/pdf/annotations/page_information_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

PageInformationAnnotation::PageInformationAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void PageInformationAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType PageInformationAnnotation::TypeImpl() const noexcept {
    return AnnotationType::PageInformation;
}

}  // namespace Aspose::Pdf::Annotations
