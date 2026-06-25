#pragma once

// =============================================================================
// Aspose::Pdf::Forms::FileSelectBoxField — file-picker form field
// (PDF §12.7.4.3 — text field with /Ff /FileSelect flag).
// Mirrors canonical Aspose.Pdf.Forms.FileSelectBoxField; extends
// TextBoxField. Canonical declares no class-specific members —
// the file-select flag is canonical /Ff bit 20 handled at save time.
// =============================================================================

#include <aspose/pdf/forms/text_box_field.hpp>

namespace Aspose::Pdf::Forms {

class FileSelectBoxField : public TextBoxField {
public:
    FileSelectBoxField() = default;
};

}  // namespace Aspose::Pdf::Forms
