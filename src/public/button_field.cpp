#include <aspose/pdf/forms/button_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

ButtonField::ButtonField(Aspose::Pdf::Page& /*page*/,
                          Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

ButtonField::ButtonField(Aspose::Pdf::Document& doc,
                          Aspose::Pdf::Rectangle rect)
    : Field(doc) {
    Rect(std::move(rect));
}

const std::string& ButtonField::NormalCaption() const noexcept {
    return normal_caption_;
}
void ButtonField::NormalCaption(std::string v) {
    normal_caption_ = std::move(v);
}

const std::string& ButtonField::RolloverCaption() const noexcept {
    return rollover_caption_;
}
void ButtonField::RolloverCaption(std::string v) {
    rollover_caption_ = std::move(v);
}

const std::string& ButtonField::AlternateCaption() const noexcept {
    return alternate_caption_;
}
void ButtonField::AlternateCaption(std::string v) {
    alternate_caption_ = std::move(v);
}

Forms::IconFit& ButtonField::IconFit() noexcept { return icon_fit_; }
const Forms::IconFit& ButtonField::IconFit() const noexcept {
    return icon_fit_;
}

Forms::IconCaptionPosition ButtonField::ICPosition() const noexcept {
    return ic_position_;
}
void ButtonField::ICPosition(Forms::IconCaptionPosition v) noexcept {
    ic_position_ = v;
}

}  // namespace Aspose::Pdf::Forms
