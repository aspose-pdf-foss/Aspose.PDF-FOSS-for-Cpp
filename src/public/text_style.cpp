#include <aspose/pdf/annotations/text_style.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

const std::string& TextStyle::FontName() const noexcept {
    return font_name_;
}
void TextStyle::FontName(std::string v) noexcept {
    font_name_ = std::move(v);
}

double TextStyle::FontSize() const noexcept { return font_size_; }
void TextStyle::FontSize(double v) noexcept { font_size_ = v; }

TextAlignment TextStyle::Alignment() const noexcept {
    return alignment_;
}
void TextStyle::Alignment(TextAlignment v) noexcept {
    alignment_ = v;
}

Aspose::Pdf::HorizontalAlignment
        TextStyle::HorizontalAlignment() const noexcept {
    return horizontal_alignment_;
}
void TextStyle::HorizontalAlignment(
        Aspose::Pdf::HorizontalAlignment v) noexcept {
    horizontal_alignment_ = v;
}

}  // namespace Aspose::Pdf::Annotations
