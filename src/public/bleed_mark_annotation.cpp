#include <aspose/pdf/annotations/bleed_mark_annotation.hpp>

#include <aspose/pdf/page.hpp>

#include "printer_mark_geometry.hpp"

namespace Aspose::Pdf::Annotations {

BleedMarkAnnotation::BleedMarkAnnotation(
        Aspose::Pdf::Page& page,
        PrinterMarkCornerPosition position)
    : page_(&page) {
    Position(position);
    // In the margin outside the bleed box's corner (21 pt square at most).
    Rect(foundation::printer_marks::MoveToCorner(page.BleedBox(), page.MediaBox(), position, 21));
}

void BleedMarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType BleedMarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::BleedMark;
}

}  // namespace Aspose::Pdf::Annotations
