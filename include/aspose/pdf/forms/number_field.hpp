#pragma once

// =============================================================================
// Aspose::Pdf::Forms::NumberField — numeric-input text field
// (PDF §12.7.4.3 — text field with numeric-format JavaScript).
// Mirrors canonical Aspose.Pdf.Forms.NumberField; extends
// TextBoxField. Adds the AllowedChars property (the set of
// characters the JS action accepts at input time).
// =============================================================================

#include <string>

#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class NumberField : public TextBoxField {
public:
    NumberField() = default;
    NumberField(Aspose::Pdf::Page& page,
                Aspose::Pdf::Rectangle rect);
    NumberField(Aspose::Pdf::Document& doc,
                Aspose::Pdf::Rectangle rect);

    const std::string& AllowedChars() const noexcept;
    void AllowedChars(std::string value);

private:
    std::string allowed_chars_;
};

}  // namespace Aspose::Pdf::Forms
