#pragma once

// =============================================================================
// Aspose::Pdf::Forms::Option — one item in a choice field
// (ListBoxField / ComboBoxField). PDF §12.7.4.4 /Opt array entry.
// Mirrors canonical Aspose.Pdf.Forms.Option.
// =============================================================================

#include <string>

namespace Aspose::Pdf::Forms {

class Option {
public:
    Option() noexcept = default;

    const std::string& Value() const noexcept;
    void Value(std::string value);

    const std::string& Name() const noexcept;
    void Name(std::string value);

    bool Selected() const noexcept;
    void Selected(bool value) noexcept;

    // Position of this option in the owning OptionCollection.
    // Read-only — set by the collection when the option is added.
    int Index() const noexcept;

private:
    friend class OptionCollection;
    void IndexInternal(int value) noexcept;

    std::string value_;
    std::string name_;
    bool selected_ = false;
    int index_ = -1;
};

}  // namespace Aspose::Pdf::Forms
