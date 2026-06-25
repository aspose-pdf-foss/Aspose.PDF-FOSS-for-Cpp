#include <aspose/pdf/forms/text_box_field.hpp>

#include <stdexcept>
#include <utility>

namespace Aspose::Pdf::Forms {

TextBoxField::TextBoxField() = default;

TextBoxField::TextBoxField(Aspose::Pdf::Document& doc)
    : Field(doc) {}

TextBoxField::TextBoxField(Aspose::Pdf::Document& doc,
                            Aspose::Pdf::Rectangle rect)
    : Field(doc) {
    Rect(std::move(rect));
}

TextBoxField::TextBoxField(Aspose::Pdf::Page& /*page*/,
                            Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

TextBoxField::TextBoxField(Aspose::Pdf::Page& /*page*/,
                            std::vector<Aspose::Pdf::Rectangle> rects)
    : rects_(std::move(rects)) {
    if (!rects_.empty()) Rect(rects_.front());
}

void TextBoxField::AddBarcode(const std::string& code) {
    barcode_ = code;
}

bool TextBoxField::Multiline() const noexcept { return multiline_; }
void TextBoxField::Multiline(bool v) noexcept { multiline_ = v; }

bool TextBoxField::SpellCheck() const noexcept { return spell_check_; }
void TextBoxField::SpellCheck(bool v) noexcept { spell_check_ = v; }

bool TextBoxField::Scrollable() const noexcept { return scrollable_; }
void TextBoxField::Scrollable(bool v) noexcept { scrollable_ = v; }

bool TextBoxField::ForceCombs() const noexcept { return force_combs_; }
void TextBoxField::ForceCombs(bool v) noexcept { force_combs_ = v; }

int TextBoxField::MaxLen() const noexcept { return max_len_; }
void TextBoxField::MaxLen(int v) noexcept { max_len_ = v; }

Aspose::Pdf::VerticalAlignment
TextBoxField::TextVerticalAlignment() const noexcept {
    return text_vertical_alignment_;
}
void TextBoxField::TextVerticalAlignment(
        Aspose::Pdf::VerticalAlignment v) noexcept {
    text_vertical_alignment_ = v;
}

const std::string& TextBoxField::Value() const noexcept {
    return Field::Value();
}
void TextBoxField::Value(std::string v) { Field::Value(std::move(v)); }

}  // namespace Aspose::Pdf::Forms
