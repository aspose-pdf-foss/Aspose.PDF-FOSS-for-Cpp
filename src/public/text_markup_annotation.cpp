#include <aspose/pdf/annotations/text_markup_annotation.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Annotations {

std::string TextMarkupAnnotation::GetMarkedText() const {
    const AnnotationCollection* owner = OwnerCollection();
    if (owner == nullptr || owner->doc_ == nullptr)
        throw std::logic_error("TextMarkupAnnotation::GetMarkedText: the annotation is not on a page");
    std::vector<std::array<double, 4>> boxes;
    for (std::size_t i = 0; i + 4 <= quad_points_.size(); i += 4) {
        double x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300;
        for (std::size_t k = i; k < i + 4; ++k) {
            x0 = std::min(x0, quad_points_[k].X()); x1 = std::max(x1, quad_points_[k].X());
            y0 = std::min(y0, quad_points_[k].Y()); y1 = std::max(y1, quad_points_[k].Y());
        }
        boxes.push_back({x0, y0, x1, y1});
    }
    if (boxes.empty()) {
        const auto& r = Rect();
        boxes.push_back({r.LLX(), r.LLY(), r.URX(), r.URY()});
    }
    return owner->doc_->MarkedTextInternal(owner->leaf_, boxes);
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
