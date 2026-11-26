#include <aspose/pdf/forms/checkbox_field.hpp>

#include "_pending.hpp"

#include <utility>

namespace Aspose::Pdf::Forms {

CheckboxField::CheckboxField(Aspose::Pdf::Document& doc)
    : Field(doc) {}

CheckboxField::CheckboxField(Aspose::Pdf::Page& /*page*/,
                              Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

CheckboxField::CheckboxField(Aspose::Pdf::Document& doc,
                              Aspose::Pdf::Rectangle rect)
    : Field(doc) {
    Rect(std::move(rect));
}

void* CheckboxField::Clone() const {
    PENDING("checkbox-clone");
}

void CheckboxField::AddOption(const std::string& /*optionName*/) {
    // Check-box options (one widget per export value) are not modelled.
    PENDING("checkbox-options");
}

void CheckboxField::AddOption(const std::string& /*optionName*/,
                               Aspose::Pdf::Rectangle /*rect*/) {
    PENDING("checkbox-options");
}

void CheckboxField::AddOption(const std::string& /*optionName*/,
                               int /*page*/,
                               Aspose::Pdf::Rectangle /*rect*/) {
    PENDING("checkbox-options");
}

std::vector<std::string> CheckboxField::AllowedStates() const {
    return {std::string("Off"), active_state_};
}

Forms::BoxStyle CheckboxField::Style() const noexcept { return style_; }
void CheckboxField::Style(Forms::BoxStyle v) noexcept { style_ = v; }

const std::string& CheckboxField::ActiveState() const noexcept {
    return active_state_;
}
void CheckboxField::ActiveState(std::string v) {
    active_state_ = std::move(v);
}

bool CheckboxField::Checked() const noexcept { return checked_; }
void CheckboxField::Checked(bool v) noexcept { checked_ = v; }

const std::string& CheckboxField::Value() const noexcept {
    return Field::Value();
}
void CheckboxField::Value(std::string v) {
    Field::Value(std::move(v));
}

const std::string& CheckboxField::ExportValue() const noexcept {
    return export_value_;
}
void CheckboxField::ExportValue(std::string v) {
    export_value_ = std::move(v);
}

}  // namespace Aspose::Pdf::Forms
