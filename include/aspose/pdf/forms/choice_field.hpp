#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ChoiceField — abstract intermediate base for
// choice form fields (RadioButtonField; the listbox / combobox
// pair lands at F5). PDF §12.7.4.4 — /FT /Ch fields. Mirrors
// canonical Aspose.Pdf.Forms.ChoiceField; extends Field.
//
// The Options accessor returns a reference to a per-field
// OptionCollection lazily created on first access.
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/option_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class ChoiceField : public Field {
public:
    ChoiceField(Aspose::Pdf::Page& page,
                Aspose::Pdf::Rectangle rect);
    explicit ChoiceField(Aspose::Pdf::Document& doc);
    ChoiceField(Aspose::Pdf::Document& doc,
                Aspose::Pdf::Rectangle rect);

    // ---- Mutation ----

    void AddOption(const std::string& optionName);
    void AddOption(const std::string& exportName,
                   const std::string& name);
    void DeleteOption(const std::string& optionName);

    // ---- Properties ----

    bool CommitImmediately() const noexcept;
    void CommitImmediately(bool value) noexcept;

    bool MultiSelect() const noexcept;
    void MultiSelect(bool value) noexcept;

    int Selected() const noexcept;
    void Selected(int value) noexcept;

    const std::vector<int>& SelectedItems() const noexcept;
    void SelectedItems(std::vector<int> value) noexcept;

    OptionCollection& Options();

    const std::string& Value() const noexcept;
    void Value(std::string value);

protected:
    ChoiceField() noexcept = default;

private:
    bool commit_immediately_ = false;
    bool multi_select_ = false;
    int selected_ = -1;
    std::vector<int> selected_items_;
    mutable std::unique_ptr<OptionCollection> options_;
    std::string value_;
};

}  // namespace Aspose::Pdf::Forms
