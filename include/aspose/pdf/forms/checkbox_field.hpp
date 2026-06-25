#pragma once

// =============================================================================
// Aspose::Pdf::Forms::CheckboxField — checkbox form field
// (PDF §12.7.4.2 — /FT /Btn without /Ff /Pushbutton or
// /Radio flags). Mirrors canonical Aspose.Pdf.Forms.CheckboxField;
// extends Field.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/forms/box_style.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class CheckboxField : public Field {
public:
    CheckboxField() = default;
    explicit CheckboxField(Aspose::Pdf::Document& doc);
    CheckboxField(Aspose::Pdf::Page& page,
                  Aspose::Pdf::Rectangle rect);
    CheckboxField(Aspose::Pdf::Document& doc,
                  Aspose::Pdf::Rectangle rect);

    // Canonical Clone returns System.Object → void* per the
    // System.Object translation. v1 stub returns nullptr; full
    // deep-copy lands with the AcroForm save-through beat.
    void* Clone() const noexcept;

    // ---- Mutation ----

    void AddOption(const std::string& optionName);
    void AddOption(const std::string& optionName,
                   Aspose::Pdf::Rectangle rect);
    void AddOption(const std::string& optionName, int page,
                   Aspose::Pdf::Rectangle rect);

    // ---- Properties ----

    // Allowed appearance-state names per PDF §12.5.6.19 /MK
    // dict + /AP /N entries. v1 returns {"Off", ActiveState()}.
    std::vector<std::string> AllowedStates() const;

    Forms::BoxStyle Style() const noexcept;
    void Style(Forms::BoxStyle value) noexcept;

    const std::string& ActiveState() const noexcept;
    void ActiveState(std::string value);

    bool Checked() const noexcept;
    void Checked(bool value) noexcept;

    // CheckboxField re-declares Value (shadows Field::Value); v1
    // chains through Field storage.
    const std::string& Value() const noexcept;
    void Value(std::string value);

    const std::string& ExportValue() const noexcept;
    void ExportValue(std::string value);

private:
    Forms::BoxStyle style_ = Forms::BoxStyle::Check;
    std::string active_state_ = "Yes";
    bool checked_ = false;
    std::string export_value_ = "Yes";
};

}  // namespace Aspose::Pdf::Forms
