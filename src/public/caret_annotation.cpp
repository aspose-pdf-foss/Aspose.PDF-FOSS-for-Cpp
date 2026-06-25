#include <aspose/pdf/annotations/caret_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

CaretAnnotation::CaretAnnotation(Aspose::Pdf::Document& document)
    : MarkupAnnotation(document) {}

CaretAnnotation::CaretAnnotation(Aspose::Pdf::Page& page,
                                  Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void CaretAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

const Aspose::Pdf::Rectangle& CaretAnnotation::Frame() const noexcept {
    return frame_;
}
void CaretAnnotation::Frame(Aspose::Pdf::Rectangle v) noexcept {
    frame_ = std::move(v);
}

CaretSymbol CaretAnnotation::Symbol() const noexcept { return symbol_; }
void CaretAnnotation::Symbol(CaretSymbol v) noexcept { symbol_ = v; }

AnnotationType CaretAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Caret;
}

}  // namespace Aspose::Pdf::Annotations
