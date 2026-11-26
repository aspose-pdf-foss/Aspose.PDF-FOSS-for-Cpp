#include <aspose/pdf/annotations/trim_mark_annotation.hpp>

#include <aspose/pdf/page.hpp>

#include "printer_mark_geometry.hpp"

namespace Aspose::Pdf::Annotations {

TrimMarkAnnotation::TrimMarkAnnotation(Aspose::Pdf::Page& page,
                                        PrinterMarkCornerPosition position)
    : page_(&page) {
    Position(position);
    // In the margin outside the trim box's corner (24 pt square at most).
    Rect(foundation::printer_marks::MoveToCorner(page.TrimBox(), page.MediaBox(), position, 24));
}

void TrimMarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType TrimMarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::TrimMark;
}

}  // namespace Aspose::Pdf::Annotations
