#include <aspose/pdf/annotations/poly_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

const std::vector<Aspose::Pdf::Point>&
        PolyAnnotation::Vertices() const noexcept {
    return vertices_;
}
void PolyAnnotation::Vertices(
        std::vector<Aspose::Pdf::Point> v) noexcept {
    vertices_ = std::move(v);
}

Aspose::Pdf::Color PolyAnnotation::InteriorColor() const noexcept {
    return interior_color_;
}
void PolyAnnotation::InteriorColor(Aspose::Pdf::Color v) noexcept {
    interior_color_ = std::move(v);
}

LineEnding PolyAnnotation::StartingStyle() const noexcept {
    return starting_style_;
}
void PolyAnnotation::StartingStyle(LineEnding v) noexcept {
    starting_style_ = v;
}

LineEnding PolyAnnotation::EndingStyle() const noexcept {
    return ending_style_;
}
void PolyAnnotation::EndingStyle(LineEnding v) noexcept {
    ending_style_ = v;
}

PolyIntent PolyAnnotation::Intent() const noexcept { return intent_; }
void PolyAnnotation::Intent(PolyIntent v) noexcept { intent_ = v; }

}  // namespace Aspose::Pdf::Annotations
