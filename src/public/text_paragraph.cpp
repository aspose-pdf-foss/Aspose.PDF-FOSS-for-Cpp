// Body for the laid-out-text class Aspose::Pdf::Text::TextParagraph. The
// wrap / align layout is performed by TextBuilder::AppendParagraph (see
// text_builder.cpp), which reads these accumulated lines and properties.

#include <aspose/pdf/text_paragraph.hpp>

#include <utility>

namespace Aspose::Pdf::Text {

TextParagraph::TextParagraph() = default;

void TextParagraph::AppendLine(const std::string& line) {
    lines_.push_back(Line{line, {}, {}, 0.0f, 0.0f});
}

void TextParagraph::AppendLine(const std::string& line, float lineSpacing) {
    lines_.push_back(Line{line, {}, {}, 0.0f, lineSpacing});
}

void TextParagraph::AppendLine(const std::string& line,
                               const Aspose::Pdf::Text::TextState& textState) {
    lines_.push_back(Line{line, textState.Font(), textState.ForegroundColor(),
                          textState.FontSize(), 0.0f});
}

void TextParagraph::AppendLine(const std::string& line,
                               const Aspose::Pdf::Text::TextState& textState,
                               float lineSpacing) {
    lines_.push_back(Line{line, textState.Font(), textState.ForegroundColor(),
                          textState.FontSize(), lineSpacing});
}

Aspose::Pdf::HorizontalAlignment TextParagraph::HorizontalAlignment()
    const noexcept {
    return halign_;
}
void TextParagraph::HorizontalAlignment(
    Aspose::Pdf::HorizontalAlignment value) noexcept {
    halign_ = value;
}

Aspose::Pdf::VerticalAlignment TextParagraph::VerticalAlignment()
    const noexcept {
    return valign_;
}
void TextParagraph::VerticalAlignment(
    Aspose::Pdf::VerticalAlignment value) noexcept {
    valign_ = value;
}

Aspose::Pdf::Rectangle TextParagraph::Rectangle() const {
    return rect_.has_value() ? *rect_ : Aspose::Pdf::Rectangle::Empty();
}
void TextParagraph::Rectangle(const Aspose::Pdf::Rectangle& value) {
    rect_ = value;
}

Aspose::Pdf::Text::Position TextParagraph::Position() const noexcept {
    return position_;
}
void TextParagraph::Position(Aspose::Pdf::Text::Position value) noexcept {
    position_ = value;
}

const Aspose::Pdf::MarginInfo& TextParagraph::Margin() const noexcept {
    return margin_;
}
void TextParagraph::Margin(Aspose::Pdf::MarginInfo value) noexcept {
    margin_ = value;
}

float TextParagraph::FirstLineIndent() const noexcept {
    return first_line_indent_;
}
void TextParagraph::FirstLineIndent(float value) noexcept {
    first_line_indent_ = value;
}

float TextParagraph::SubsequentLinesIndent() const noexcept {
    return subsequent_lines_indent_;
}
void TextParagraph::SubsequentLinesIndent(float value) noexcept {
    subsequent_lines_indent_ = value;
}

}  // namespace Aspose::Pdf::Text
