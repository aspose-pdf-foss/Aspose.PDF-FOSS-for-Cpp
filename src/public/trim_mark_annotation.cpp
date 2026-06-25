#include <aspose/pdf/annotations/trim_mark_annotation.hpp>

namespace Aspose::Pdf::Annotations {

TrimMarkAnnotation::TrimMarkAnnotation(Aspose::Pdf::Page& page,
                                        PrinterMarkCornerPosition position)
    : page_(&page) {
    Position(position);
}

void TrimMarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType TrimMarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::TrimMark;
}

}  // namespace Aspose::Pdf::Annotations
