#include <aspose/pdf/forms/rich_text_box_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

RichTextBoxField::RichTextBoxField(Aspose::Pdf::Page& page,
                                    Aspose::Pdf::Rectangle rect)
    : TextBoxField(page, std::move(rect)) {}

const std::string& RichTextBoxField::Style() const noexcept {
    return style_;
}
void RichTextBoxField::Style(std::string v) { style_ = std::move(v); }

const std::string& RichTextBoxField::RichTextValue() const noexcept {
    return rich_text_value_;
}
void RichTextBoxField::RichTextValue(std::string v) {
    rich_text_value_ = std::move(v);
}

const std::string& RichTextBoxField::FormattedValue() const noexcept {
    return formatted_value_;
}
void RichTextBoxField::FormattedValue(std::string v) {
    formatted_value_ = std::move(v);
}

const std::string& RichTextBoxField::Value() const noexcept {
    return TextBoxField::Value();
}
void RichTextBoxField::Value(std::string v) {
    TextBoxField::Value(std::move(v));
}

Aspose::Pdf::Annotations::Justification
RichTextBoxField::Justify() const noexcept { return justify_; }
void RichTextBoxField::Justify(
        Aspose::Pdf::Annotations::Justification v) noexcept {
    justify_ = v;
}

}  // namespace Aspose::Pdf::Forms
