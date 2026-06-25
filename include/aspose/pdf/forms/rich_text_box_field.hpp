#pragma once

// =============================================================================
// Aspose::Pdf::Forms::RichTextBoxField — rich-text input field
// (PDF §12.7.4.3 — text field with /Ff /RichText flag). Mirrors
// canonical Aspose.Pdf.Forms.RichTextBoxField; extends
// TextBoxField. Adds the rich-text string (XHTML-formatted /RV
// entry per PDF spec) + the style/format/justify accessors that
// expose how the rich text renders.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/justification.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Forms {

class RichTextBoxField : public TextBoxField {
public:
    RichTextBoxField() = default;
    RichTextBoxField(Aspose::Pdf::Page& page,
                     Aspose::Pdf::Rectangle rect);

    const std::string& Style() const noexcept;
    void Style(std::string value);

    const std::string& RichTextValue() const noexcept;
    void RichTextValue(std::string value);

    const std::string& FormattedValue() const noexcept;
    void FormattedValue(std::string value);

    // Value is also declared on the canonical RichTextBoxField
    // surface (shadows TextBoxField::Value). v1 chains through
    // TextBoxField's storage for the plain-text form.
    const std::string& Value() const noexcept;
    void Value(std::string value);

    Aspose::Pdf::Annotations::Justification Justify() const noexcept;
    void Justify(Aspose::Pdf::Annotations::Justification value) noexcept;

private:
    std::string style_;
    std::string rich_text_value_;
    std::string formatted_value_;
    Aspose::Pdf::Annotations::Justification justify_ =
        Aspose::Pdf::Annotations::Justification::LeftJustify;
};

}  // namespace Aspose::Pdf::Forms
