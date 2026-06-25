#include <aspose/pdf/annotations/link_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

LinkAnnotation::LinkAnnotation(Aspose::Pdf::Page& page,
                                Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void LinkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

PdfAction* LinkAnnotation::Action() const noexcept { return action_.get(); }

void LinkAnnotation::Action(const PdfAction& value) {
    action_ = value.CloneAction();
}

IAppointment* LinkAnnotation::Destination() const noexcept {
    return destination_.get();
}

void LinkAnnotation::Destination(const IAppointment& value) {
    destination_ = value.CloneAppointment();
}

HighlightingMode LinkAnnotation::Highlighting() const noexcept {
    return highlighting_;
}
void LinkAnnotation::Highlighting(HighlightingMode v) noexcept {
    highlighting_ = v;
}

AnnotationType LinkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Link;
}

}  // namespace Aspose::Pdf::Annotations
