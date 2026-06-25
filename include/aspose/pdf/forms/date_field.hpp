#pragma once

// =============================================================================
// Aspose::Pdf::Forms::DateField — date-input text field (PDF
// §12.7.4.3 — text field with date-format JS action). Mirrors
// canonical Aspose.Pdf.Forms.DateField; extends TextBoxField.
//
// Phased drops on this beat:
//   * Value (System.DateTime) — DateTime translations cascade
//   * AddImage(Aspose.Pdf.Image) — Aspose.Pdf.Image cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class DateField : public TextBoxField {
public:
    DateField() = default;
    explicit DateField(Aspose::Pdf::Document& doc);
    DateField(Aspose::Pdf::Page& page,
              Aspose::Pdf::Rectangle rect);
    DateField(Aspose::Pdf::Document& doc,
              Aspose::Pdf::Rectangle rect);

    // Bind the field to a page after construction (matches the
    // canonical Init(Page) helper used by the (Document, Rect)
    // ctor pattern). v1 stores the page pointer for downstream
    // save-through wiring.
    void Init(Aspose::Pdf::Page& page);

    const std::string& DateFormat() const noexcept;
    void DateFormat(std::string value);

private:
    Aspose::Pdf::Page* init_page_ = nullptr;
    std::string date_format_;
};

}  // namespace Aspose::Pdf::Forms
