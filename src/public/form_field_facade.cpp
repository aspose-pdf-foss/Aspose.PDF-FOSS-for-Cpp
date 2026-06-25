#include <aspose/pdf/facades/form_field_facade.hpp>

#include <utility>

namespace Aspose::Pdf::Facades {

void FormFieldFacade::Reset() {
    *this = FormFieldFacade{};
}

int FormFieldFacade::BorderStyle() const noexcept { return border_style_; }
void FormFieldFacade::BorderStyle(int value) noexcept { border_style_ = value; }

float FormFieldFacade::BorderWidth() const noexcept { return border_width_; }
void FormFieldFacade::BorderWidth(float value) noexcept {
    border_width_ = value;
}

FontStyle FormFieldFacade::Font() const noexcept { return font_; }
void FormFieldFacade::Font(FontStyle value) noexcept { font_ = value; }

const std::string& FormFieldFacade::CustomFont() const noexcept {
    return custom_font_;
}
void FormFieldFacade::CustomFont(std::string value) {
    custom_font_ = std::move(value);
}

float FormFieldFacade::FontSize() const noexcept { return font_size_; }
void FormFieldFacade::FontSize(float value) noexcept { font_size_ = value; }

EncodingType FormFieldFacade::TextEncoding() const noexcept {
    return text_encoding_;
}
void FormFieldFacade::TextEncoding(EncodingType value) noexcept {
    text_encoding_ = value;
}

int FormFieldFacade::Alignment() const noexcept { return alignment_; }
void FormFieldFacade::Alignment(int value) noexcept { alignment_ = value; }

int FormFieldFacade::Rotation() const noexcept { return rotation_; }
void FormFieldFacade::Rotation(int value) noexcept { rotation_ = value; }

const std::string& FormFieldFacade::Caption() const noexcept {
    return caption_;
}
void FormFieldFacade::Caption(std::string value) {
    caption_ = std::move(value);
}

int FormFieldFacade::ButtonStyle() const noexcept { return button_style_; }
void FormFieldFacade::ButtonStyle(int value) noexcept {
    button_style_ = value;
}

const std::vector<float>& FormFieldFacade::Position() const noexcept {
    return position_;
}
void FormFieldFacade::Position(std::vector<float> value) {
    position_ = std::move(value);
}

int FormFieldFacade::PageNumber() const noexcept { return page_number_; }
void FormFieldFacade::PageNumber(int value) noexcept {
    page_number_ = value;
}

const std::vector<std::string>& FormFieldFacade::Items() const noexcept {
    return items_;
}
void FormFieldFacade::Items(std::vector<std::string> value) {
    items_ = std::move(value);
}

const std::vector<std::vector<std::string>>&
FormFieldFacade::ExportItems() const noexcept {
    return export_items_;
}
void FormFieldFacade::ExportItems(std::vector<std::vector<std::string>> value) {
    export_items_ = std::move(value);
}

}  // namespace Aspose::Pdf::Facades
