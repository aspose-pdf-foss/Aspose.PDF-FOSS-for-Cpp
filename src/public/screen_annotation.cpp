#include <aspose/pdf/annotations/screen_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

ScreenAnnotation::ScreenAnnotation(Aspose::Pdf::Page& page,
                                    Aspose::Pdf::Rectangle rect,
                                    std::string mediaFile)
    : page_(&page),
      media_file_(std::move(mediaFile)) {
    Rect(std::move(rect));
}

void ScreenAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

const std::string& ScreenAnnotation::Title() const noexcept {
    return title_;
}
void ScreenAnnotation::Title(std::string v) noexcept {
    title_ = std::move(v);
}

AnnotationType ScreenAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Screen;
}

}  // namespace Aspose::Pdf::Annotations
