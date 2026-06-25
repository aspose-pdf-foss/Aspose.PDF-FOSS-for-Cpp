#include <aspose/pdf/annotations/registration_mark_annotation.hpp>

namespace Aspose::Pdf::Annotations {

RegistrationMarkAnnotation::RegistrationMarkAnnotation(
        Aspose::Pdf::Page& page,
        PrinterMarkSidePosition position)
    : page_(&page),
      position_(position) {}

void RegistrationMarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

PrinterMarkSidePosition
RegistrationMarkAnnotation::Position() const noexcept { return position_; }
void RegistrationMarkAnnotation::Position(
        PrinterMarkSidePosition v) noexcept { position_ = v; }

AnnotationType RegistrationMarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::RegistrationMark;
}

}  // namespace Aspose::Pdf::Annotations
