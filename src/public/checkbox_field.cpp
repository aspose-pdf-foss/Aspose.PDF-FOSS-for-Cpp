#include <aspose/pdf/forms/checkbox_field.hpp>

#include "_pending.hpp"

#include <algorithm>
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
    auto* c = new CheckboxField();
    c->PartialName(PartialName());
    c->Rect(Rect());
    c->style_ = style_;
    c->active_state_ = active_state_;
    c->checked_ = checked_;
    c->export_value_ = export_value_;
    c->options_ = options_;
    c->Field::Value(Field::Value());
    return c;
}

void CheckboxField::AddOption(const std::string& optionName) {
    // Below the lowest widget, the same size as the field's.
    const auto& r = Rect();
    double lowest = r.LLY();
    for (const auto& o : options_) lowest = std::min(lowest, o.rect.LLY());
    const double h = r.URY() - r.LLY() > 0 ? r.URY() - r.LLY() : 20.0;
    AddOption(optionName, 0, Aspose::Pdf::Rectangle(r.LLX(), lowest - h, r.URX(), lowest, false));
}

void CheckboxField::AddOption(const std::string& optionName,
                               Aspose::Pdf::Rectangle rect) {
    AddOption(optionName, 0, std::move(rect));
}

void CheckboxField::AddOption(const std::string& optionName, int page,
                               Aspose::Pdf::Rectangle rect) {
    if (IsLoadedField()) PENDING("checkbox-options-on-loaded-field");
    options_.push_back(Option{optionName, std::move(rect), page});
}

std::vector<std::string> CheckboxField::AllowedStates() const {
    std::vector<std::string> out{std::string("Off"), active_state_};
    for (const auto& o : options_)
        if (std::find(out.begin(), out.end(), o.name) == out.end()) out.push_back(o.name);
    return out;
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
