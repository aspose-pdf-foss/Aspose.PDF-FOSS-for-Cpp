#include <aspose/pdf/annotations/popup_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

PopupAnnotation::PopupAnnotation(Aspose::Pdf::Document& /*document*/) {}

PopupAnnotation::PopupAnnotation(Aspose::Pdf::Page& page,
                                  Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void PopupAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

bool PopupAnnotation::Open() const noexcept { return open_; }
void PopupAnnotation::Open(bool v) noexcept { open_ = v; }

const std::shared_ptr<Annotation>& PopupAnnotation::Parent() const noexcept {
    return parent_;
}
void PopupAnnotation::Parent(std::shared_ptr<Annotation> v) noexcept {
    parent_ = std::move(v);
}

AnnotationType PopupAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Popup;
}

}  // namespace Aspose::Pdf::Annotations
