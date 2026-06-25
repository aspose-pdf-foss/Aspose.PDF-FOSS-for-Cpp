#include <aspose/pdf/forms/radio_button_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

RadioButtonField::RadioButtonField(Aspose::Pdf::Page& /*page*/) {}

RadioButtonField::RadioButtonField(Aspose::Pdf::Document& doc)
    : ChoiceField(doc) {}

void RadioButtonField::Add(RadioButtonOptionField newItem) {
    Option o;
    o.Name(newItem.OptionName());
    o.Value(newItem.OptionName());
    Options().Add(std::move(o));
    option_kids_.emplace_back(std::move(newItem));
}

void RadioButtonField::AddOption(const std::string& optionName,
                                  Aspose::Pdf::Rectangle rect) {
    ChoiceField::AddOption(optionName);
    // Keep the per-option widget rectangle so Save can emit one Kid widget
    // per option — a radio group renders as N positioned buttons.
    RadioButtonOptionField kid;
    kid.OptionName(optionName);
    kid.Rect(std::move(rect));
    option_kids_.push_back(std::move(kid));
}

void RadioButtonField::SetPosition(Aspose::Pdf::Point point) {
    Field::SetPosition(point);
}

Forms::BoxStyle RadioButtonField::Style() const noexcept {
    return style_;
}
void RadioButtonField::Style(Forms::BoxStyle v) noexcept {
    style_ = v;
}

bool RadioButtonField::NoToggleToOff() const noexcept {
    return no_toggle_to_off_;
}
void RadioButtonField::NoToggleToOff(bool v) noexcept {
    no_toggle_to_off_ = v;
}

}  // namespace Aspose::Pdf::Forms
