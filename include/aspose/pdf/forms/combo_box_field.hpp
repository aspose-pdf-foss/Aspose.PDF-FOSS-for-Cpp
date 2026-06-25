#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ComboBoxField — combobox form field (PDF
// §12.7.4.4 — /FT /Ch with /Ff /Combo flag). Mirrors canonical
// Aspose.Pdf.Forms.ComboBoxField; extends ChoiceField.
//
// Editable controls /Ff /Edit (free text entry alongside the
// dropdown choices). SpellCheck mirrors TextBoxField's flag.
// =============================================================================

#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class ComboBoxField : public ChoiceField {
public:
    ComboBoxField() = default;
    explicit ComboBoxField(Aspose::Pdf::Document& doc);
    ComboBoxField(Aspose::Pdf::Page& page,
                  Aspose::Pdf::Rectangle rect);
    ComboBoxField(Aspose::Pdf::Document& doc,
                  Aspose::Pdf::Rectangle rect);

    bool Editable() const noexcept;
    void Editable(bool value) noexcept;

    bool SpellCheck() const noexcept;
    void SpellCheck(bool value) noexcept;

private:
    bool editable_ = false;
    bool spell_check_ = true;
};

}  // namespace Aspose::Pdf::Forms
