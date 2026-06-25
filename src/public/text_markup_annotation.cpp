#include <aspose/pdf/annotations/text_markup_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

std::string TextMarkupAnnotation::GetMarkedText() const {
    // v1 stub — full text extraction across the QuadPoints
    // requires the text-extraction wire-through (separate
    // cluster). Returns empty until that lands.
    return std::string{};
}

const std::vector<Aspose::Pdf::Point>&
        TextMarkupAnnotation::QuadPoints() const noexcept {
    return quad_points_;
}
void TextMarkupAnnotation::QuadPoints(
        std::vector<Aspose::Pdf::Point> v) noexcept {
    quad_points_ = std::move(v);
}

}  // namespace Aspose::Pdf::Annotations
