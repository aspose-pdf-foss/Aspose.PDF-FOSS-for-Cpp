#include <aspose/pdf/annotations/color_bar_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

ColorBarAnnotation::ColorBarAnnotation(Aspose::Pdf::Page& page,
                                        Aspose::Pdf::Rectangle rect,
                                        ColorsOfCMYK colorOfCMYK)
    : page_(&page),
      color_of_cmyk_(colorOfCMYK) {
    Rect(std::move(rect));
}

void ColorBarAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

ColorsOfCMYK ColorBarAnnotation::ColorOfCMYK() const noexcept {
    return color_of_cmyk_;
}
void ColorBarAnnotation::ColorOfCMYK(ColorsOfCMYK v) noexcept {
    color_of_cmyk_ = v;
}

AnnotationType ColorBarAnnotation::TypeImpl() const noexcept {
    return AnnotationType::ColorBar;
}

}  // namespace Aspose::Pdf::Annotations
