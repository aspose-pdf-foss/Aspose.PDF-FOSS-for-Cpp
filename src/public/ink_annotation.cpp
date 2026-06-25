#include <aspose/pdf/annotations/ink_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

InkAnnotation::InkAnnotation(Aspose::Pdf::Document& document,
                              StrokeList inkList)
    : MarkupAnnotation(document),
      ink_list_(std::move(inkList)) {}

InkAnnotation::InkAnnotation(Aspose::Pdf::Page& page,
                              Aspose::Pdf::Rectangle rect,
                              StrokeList inkList)
    : page_(&page),
      ink_list_(std::move(inkList)) {
    Rect(std::move(rect));
}

void InkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

CapStyle InkAnnotation::CapStyle() const noexcept { return cap_style_; }
void InkAnnotation::CapStyle(Annotations::CapStyle v) noexcept {
    cap_style_ = v;
}

const InkAnnotation::StrokeList& InkAnnotation::InkList() const noexcept {
    return ink_list_;
}
void InkAnnotation::InkList(StrokeList v) noexcept {
    ink_list_ = std::move(v);
}

AnnotationType InkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Ink;
}

}  // namespace Aspose::Pdf::Annotations
