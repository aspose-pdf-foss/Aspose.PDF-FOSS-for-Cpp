#pragma once

// =============================================================================
// Aspose::Pdf::Forms::PasswordBoxField — password-entry form field
// (PDF §12.7.4.3 — text field with /Ff /Password flag). Mirrors
// canonical Aspose.Pdf.Forms.PasswordBoxField; extends TextBoxField.
// Canonical declares no class-specific members — the password
// flag is canonical /Ff bit 14 handled at save time.
// =============================================================================

#include <aspose/pdf/forms/text_box_field.hpp>

namespace Aspose::Pdf::Forms {

class PasswordBoxField : public TextBoxField {
public:
    PasswordBoxField() = default;
};

}  // namespace Aspose::Pdf::Forms
