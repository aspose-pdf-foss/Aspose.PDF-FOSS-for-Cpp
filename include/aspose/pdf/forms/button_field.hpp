#pragma once

// =============================================================================
// Aspose::Pdf::Forms::ButtonField — push-button form field
// (PDF §12.7.4.2 — /FT /Btn with /Ff /Pushbutton flag). Mirrors
// canonical Aspose.Pdf.Forms.ButtonField; extends Field.
//
// Phased drops on this beat:
//   * AddImage(System.Drawing.Image) — Image cascade
//   * NormalIcon / RolloverIcon / AlternateIcon (XForm) — XForm
//     cascade
// =============================================================================

#include <string>

#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/icon_caption_position.hpp>
#include <aspose/pdf/forms/icon_fit.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class ButtonField : public Field {
public:
    ButtonField() = default;
    ButtonField(Aspose::Pdf::Page& page,
                Aspose::Pdf::Rectangle rect);
    ButtonField(Aspose::Pdf::Document& doc,
                Aspose::Pdf::Rectangle rect);

    // ---- Properties ----

    const std::string& NormalCaption() const noexcept;
    void NormalCaption(std::string value);

    const std::string& RolloverCaption() const noexcept;
    void RolloverCaption(std::string value);

    const std::string& AlternateCaption() const noexcept;
    void AlternateCaption(std::string value);

    // IconFit accessor — canonical surface is read-only (has_setter:
    // false); mutate via the returned reference.
    Forms::IconFit& IconFit() noexcept;
    const Forms::IconFit& IconFit() const noexcept;

    Forms::IconCaptionPosition ICPosition() const noexcept;
    void ICPosition(Forms::IconCaptionPosition value) noexcept;

private:
    std::string normal_caption_;
    std::string rollover_caption_;
    std::string alternate_caption_;
    Forms::IconFit icon_fit_;
    Forms::IconCaptionPosition ic_position_ =
        Forms::IconCaptionPosition::NoIcon;
};

}  // namespace Aspose::Pdf::Forms
