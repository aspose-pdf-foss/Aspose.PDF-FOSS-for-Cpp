#include <aspose/pdf/forms/radio_button_option_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

RadioButtonOptionField::RadioButtonOptionField(
        Aspose::Pdf::Page& /*page*/,
        Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

const std::string&
RadioButtonOptionField::OptionName() const noexcept {
    return option_name_;
}
void RadioButtonOptionField::OptionName(std::string v) {
    option_name_ = std::move(v);
}

Forms::BoxStyle RadioButtonOptionField::Style() const noexcept {
    return style_;
}
void RadioButtonOptionField::Style(Forms::BoxStyle v) noexcept {
    style_ = v;
}

}  // namespace Aspose::Pdf::Forms
