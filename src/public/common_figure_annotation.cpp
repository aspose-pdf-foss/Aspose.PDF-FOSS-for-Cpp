#include <aspose/pdf/annotations/common_figure_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

CommonFigureAnnotation::CommonFigureAnnotation(
        Aspose::Pdf::Document& document)
    : MarkupAnnotation(document) {}

Aspose::Pdf::Color CommonFigureAnnotation::InteriorColor() const noexcept {
    return interior_color_;
}
void CommonFigureAnnotation::InteriorColor(
        Aspose::Pdf::Color v) noexcept {
    interior_color_ = std::move(v);
}

const Aspose::Pdf::Rectangle&
        CommonFigureAnnotation::Frame() const noexcept {
    return frame_;
}
void CommonFigureAnnotation::Frame(Aspose::Pdf::Rectangle v) noexcept {
    frame_ = std::move(v);
}

}  // namespace Aspose::Pdf::Annotations
