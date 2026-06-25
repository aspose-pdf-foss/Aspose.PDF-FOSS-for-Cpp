#include <aspose/pdf/annotations/watermark_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

WatermarkAnnotation::WatermarkAnnotation(Aspose::Pdf::Page& page,
                                          Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void WatermarkAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

double WatermarkAnnotation::Opacity() const noexcept { return opacity_; }
void WatermarkAnnotation::Opacity(double v) noexcept { opacity_ = v; }

AnnotationType WatermarkAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Watermark;
}

}  // namespace Aspose::Pdf::Annotations
