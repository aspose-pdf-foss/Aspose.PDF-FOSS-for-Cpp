#include <aspose/pdf/annotations/bleed_mark_annotation.hpp>

namespace Aspose::Pdf::Annotations {

BleedMarkAnnotation::BleedMarkAnnotation(
        Aspose::Pdf::Page& page,
        PrinterMarkCornerPosition position)
    : page_(&page) {
    Position(position);
}

void BleedMarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType BleedMarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::BleedMark;
}

}  // namespace Aspose::Pdf::Annotations
