#include <aspose/pdf/forms/combo_box_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

ComboBoxField::ComboBoxField(Aspose::Pdf::Document& doc)
    : ChoiceField(doc) {}

ComboBoxField::ComboBoxField(Aspose::Pdf::Page& /*page*/,
                              Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

ComboBoxField::ComboBoxField(Aspose::Pdf::Document& doc,
                              Aspose::Pdf::Rectangle rect)
    : ChoiceField(doc) {
    Rect(std::move(rect));
}

bool ComboBoxField::Editable() const noexcept { return editable_; }
void ComboBoxField::Editable(bool v) noexcept { editable_ = v; }

bool ComboBoxField::SpellCheck() const noexcept { return spell_check_; }
void ComboBoxField::SpellCheck(bool v) noexcept { spell_check_ = v; }

}  // namespace Aspose::Pdf::Forms
