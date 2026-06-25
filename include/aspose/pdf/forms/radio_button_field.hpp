#pragma once

// =============================================================================
// Aspose::Pdf::Forms::RadioButtonField — radio-button group field
// (PDF §12.7.4.2 — /FT /Btn with /Ff /Radio flag). Mirrors
// canonical Aspose.Pdf.Forms.RadioButtonField; extends ChoiceField.
//
// Holds the list of RadioButtonOptionField children + selection
// state. Add(RadioButtonOptionField) appends; iteration is
// via the ChoiceField.Options collection (Option entries
// mirror the option-field names) — v1 keeps both in sync.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/forms/box_style.hpp>
#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/forms/radio_button_option_field.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class RadioButtonField : public ChoiceField {
public:
    explicit RadioButtonField(Aspose::Pdf::Page& page);
    explicit RadioButtonField(Aspose::Pdf::Document& doc);

    // ---- Mutation ----

    void Add(RadioButtonOptionField newItem);
    void AddOption(const std::string& optionName,
                   Aspose::Pdf::Rectangle rect);
    using ChoiceField::AddOption;  // expose single-arg overload
    using ChoiceField::Selected;
    using ChoiceField::Options;
    using ChoiceField::Value;

    // RadioButtonField has its own SetPosition that shadows
    // Field::SetPosition (canonical declares it on both). v1
    // delegates to the base impl.
    void SetPosition(Aspose::Pdf::Point point);

    // ---- Properties ----

    Forms::BoxStyle Style() const noexcept;
    void Style(Forms::BoxStyle value) noexcept;

    bool NoToggleToOff() const noexcept;
    void NoToggleToOff(bool value) noexcept;

private:
    // Save reads option_kids_ to emit the parent /Btn field + one Kid widget
    // (with /AP on/off states) per option.
    friend class Aspose::Pdf::Document;
    Forms::BoxStyle style_ = Forms::BoxStyle::Circle;
    bool no_toggle_to_off_ = false;
    std::vector<RadioButtonOptionField> option_kids_;
};

}  // namespace Aspose::Pdf::Forms
