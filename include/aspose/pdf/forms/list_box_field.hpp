#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ListBoxField — multi-select listbox form
// field (PDF §12.7.4.4 — /FT /Ch with /Ff /Combo *not* set).
// Mirrors canonical Aspose.Pdf.Forms.ListBoxField; extends
// ChoiceField.
//
// Selected / SelectedItems on ListBoxField are setter-only per
// canonical (write-shadow over ChoiceField's read-write
// versions). In C++ the inherited surface already provides
// both getter and setter via the `using` declarations below.
// =============================================================================

#include <vector>

#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class ListBoxField : public ChoiceField {
public:
    ListBoxField() = default;
    ListBoxField(Aspose::Pdf::Page& page,
                 Aspose::Pdf::Rectangle rect);
    ListBoxField(Aspose::Pdf::Document& doc,
                 Aspose::Pdf::Rectangle rect);

    using ChoiceField::Selected;
    using ChoiceField::SelectedItems;

    int TopIndex() const noexcept;
    void TopIndex(int value) noexcept;

private:
    int top_index_ = 0;
};

}  // namespace Aspose::Pdf::Forms
