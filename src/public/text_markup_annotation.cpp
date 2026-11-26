#include <aspose/pdf/annotations/text_markup_annotation.hpp>

#include "_pending.hpp"

#include <utility>

namespace Aspose::Pdf::Annotations {

std::string TextMarkupAnnotation::GetMarkedText() const {
    // Text under the QuadPoints is not extracted yet.
    PENDING("markup-marked-text");
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
