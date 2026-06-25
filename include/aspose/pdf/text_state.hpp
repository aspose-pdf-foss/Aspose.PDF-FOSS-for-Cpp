#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextState — text rendering state. Mirrors canonical
// Aspose.Pdf.Text.TextState (Aspose.PDF 26.4.0) as a strict subset: v1 ships
// the font-mutation surface (Font, FontSize) used by the changeFont path.
// TextFragmentState (the per-fragment state returned by TextFragment.TextState)
// derives from this.
// =============================================================================

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/font.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>

#include <string>

namespace Aspose::Pdf::Text {

class TextState {
public:
    TextState() = default;
    virtual ~TextState() = default;

    // Canonical TextState.Font — the active font (get/set). Fully qualified
    // throughout to disambiguate the member function from the Font type.
    const Aspose::Pdf::Text::Font& Font() const noexcept;
    virtual void Font(const Aspose::Pdf::Text::Font& value);

    // Canonical TextState.FontSize — the active font size in points.
    float FontSize() const noexcept;
    virtual void FontSize(float value);

    // Canonical fill / stroke / background colours.
    const Aspose::Pdf::Color& ForegroundColor() const noexcept;
    void ForegroundColor(const Aspose::Pdf::Color& value);
    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(const Aspose::Pdf::Color& value);
    const Aspose::Pdf::Color& StrokingColor() const noexcept;
    void StrokingColor(const Aspose::Pdf::Color& value);

    // Canonical text-decoration / position flags.
    bool Underline() const noexcept;
    void Underline(bool value);
    bool StrikeOut() const noexcept;
    void StrikeOut(bool value);
    bool Subscript() const noexcept;
    void Subscript(bool value);
    bool Superscript() const noexcept;
    void Superscript(bool value);
    bool Invisible() const noexcept;
    void Invisible(bool value);

    // Canonical spacing / scaling (points; HorizontalScaling is a percentage).
    float CharacterSpacing() const noexcept;
    void CharacterSpacing(float value);
    float WordSpacing() const noexcept;
    void WordSpacing(float value);
    float LineSpacing() const noexcept;
    void LineSpacing(float value);
    float HorizontalScaling() const noexcept;
    void HorizontalScaling(float value);

    // Canonical horizontal alignment of the text.
    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value);

    // Canonical TextState.MeasureString — advance width of `str` in the current
    // font at the current FontSize (points). Exact for the Standard-14 fonts via
    // the standard14_metrics primitive; CharacterSpacing / WordSpacing /
    // HorizontalScaling are applied. Embedded or unset (non-Standard-14) fonts
    // fall back to the space advance per glyph.
    double MeasureString(const std::string& str) const;

protected:
    Aspose::Pdf::Text::Font font_;
    float font_size_ = 0.0f;
    Aspose::Pdf::Color foreground_color_;
    Aspose::Pdf::Color background_color_;
    Aspose::Pdf::Color stroking_color_;
    bool underline_ = false;
    bool strike_out_ = false;
    bool subscript_ = false;
    bool superscript_ = false;
    bool invisible_ = false;
    float character_spacing_ = 0.0f;
    float word_spacing_ = 0.0f;
    float line_spacing_ = 0.0f;
    float horizontal_scaling_ = 100.0f;
    Aspose::Pdf::HorizontalAlignment horizontal_alignment_ =
        Aspose::Pdf::HorizontalAlignment::None;
};

}  // namespace Aspose::Pdf::Text
