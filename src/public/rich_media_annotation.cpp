#include <aspose/pdf/annotations/rich_media_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

RichMediaAnnotation::RichMediaAnnotation(Aspose::Pdf::Page& page,
                                          Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void RichMediaAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

void RichMediaAnnotation::Update() {
    // v1 no-op — stream re-emission is wired into the Document
    // save-time edit-flow which v1 doesn't yet expose.
}

const std::string&
RichMediaAnnotation::CustomFlashVariables() const noexcept {
    return custom_flash_variables_;
}
void RichMediaAnnotation::CustomFlashVariables(std::string v) noexcept {
    custom_flash_variables_ = std::move(v);
}

RichMediaAnnotation::ContentType
RichMediaAnnotation::Type() const noexcept { return type_; }
void RichMediaAnnotation::Type(ContentType v) noexcept { type_ = v; }

RichMediaAnnotation::ActivationEvent
RichMediaAnnotation::ActivateOn() const noexcept { return activate_on_; }
void RichMediaAnnotation::ActivateOn(ActivationEvent v) noexcept {
    activate_on_ = v;
}

AnnotationType RichMediaAnnotation::TypeImpl() const noexcept {
    return AnnotationType::RichMedia;
}

}  // namespace Aspose::Pdf::Annotations
