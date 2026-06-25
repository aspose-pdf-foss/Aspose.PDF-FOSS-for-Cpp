#pragma once

// =============================================================================
// Aspose::Pdf::Forms::RadioButtonOptionField — one option-button
// inside a RadioButtonField (PDF §12.7.4.2 — /Ff /Radio + the
// /Kids array of the parent radio field). Mirrors canonical
// Aspose.Pdf.Forms.RadioButtonOptionField; extends Field.
//
// Phased drops on this beat:
//   * Caption (Aspose.Pdf.Text.TextFragment) — TextFragment cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/box_style.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Forms {

class RadioButtonOptionField : public Field {
public:
    RadioButtonOptionField() = default;
    RadioButtonOptionField(Aspose::Pdf::Page& page,
                            Aspose::Pdf::Rectangle rect);

    // ---- Properties ----

    const std::string& OptionName() const noexcept;
    void OptionName(std::string value);

    Forms::BoxStyle Style() const noexcept;
    void Style(Forms::BoxStyle value) noexcept;

private:
    std::string option_name_;
    Forms::BoxStyle style_ = Forms::BoxStyle::Circle;
};

}  // namespace Aspose::Pdf::Forms
