#include <aspose/pdf/annotations/text_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

TextAnnotation::TextAnnotation(Aspose::Pdf::Document& document)
    : MarkupAnnotation(document) {}

TextAnnotation::TextAnnotation(Aspose::Pdf::Page& page,
                                Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void TextAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

bool TextAnnotation::Open() const noexcept { return open_; }
void TextAnnotation::Open(bool v) noexcept { open_ = v; }

TextIcon TextAnnotation::Icon() const noexcept { return icon_; }
void TextAnnotation::Icon(TextIcon v) noexcept { icon_ = v; }

AnnotationType TextAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Text;
}

}  // namespace Aspose::Pdf::Annotations
