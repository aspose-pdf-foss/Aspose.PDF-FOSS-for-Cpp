#include <aspose/pdf/annotations/stamp_annotation.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace Aspose::Pdf::Annotations {

StampAnnotation::StampAnnotation(Aspose::Pdf::Document& document)
    : MarkupAnnotation(document) {}

StampAnnotation::StampAnnotation(Aspose::Pdf::Page& page,
                                  Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void StampAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

StampIcon StampAnnotation::Icon() const noexcept { return icon_; }
void StampAnnotation::Icon(StampIcon v) noexcept { icon_ = v; }

const std::vector<std::byte>& StampAnnotation::Image() const noexcept {
    return image_;
}
void StampAnnotation::Image(std::vector<std::byte> value) {
    image_ = std::move(value);
}

AnnotationType StampAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Stamp;
}

}  // namespace Aspose::Pdf::Annotations
