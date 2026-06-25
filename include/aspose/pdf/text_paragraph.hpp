#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextParagraph — a laid-out block of text: a sequence of
// lines word-wrapped to a rectangle and horizontally / vertically aligned.
// Mirrors canonical Aspose.Pdf.Text.TextParagraph (Aspose.PDF 26.4.0). Build
// it with AppendLine, set its Rectangle (wrap box) or Position (origin) and
// alignment, then place it on a page with TextBuilder::AppendParagraph.
//
// v1 subset: the string AppendLine overloads ship (with optional per-line
// TextState / line spacing); the TextFragment-as-line overloads, BeginEdit /
// EndEdit, FormattingOptions, TextRectangle and Rotation are deferred (see the
// cpp drops). Wrapping and Left/Center/Right/Justify/FullJustify alignment use
// real Standard-14 glyph metrics via TextState::MeasureString.
// =============================================================================

#include <optional>
#include <string>
#include <vector>

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/font.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf::Text {

class TextBuilder;

class TextParagraph {
public:
    TextParagraph();

    // Canonical AppendLine — add a line of text, optionally with a TextState
    // (font size) and/or an explicit line spacing.
    void AppendLine(const std::string& line);
    void AppendLine(const std::string& line, float lineSpacing);
    void AppendLine(const std::string& line,
                    const Aspose::Pdf::Text::TextState& textState);
    void AppendLine(const std::string& line,
                    const Aspose::Pdf::Text::TextState& textState,
                    float lineSpacing);

    // Canonical HorizontalAlignment — Left / Center / Right within the box.
    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value) noexcept;

    // Canonical VerticalAlignment — Top / Center / Bottom within the box.
    Aspose::Pdf::VerticalAlignment VerticalAlignment() const noexcept;
    void VerticalAlignment(Aspose::Pdf::VerticalAlignment value) noexcept;

    // Canonical Rectangle — the wrap / clip box (origin + width drive layout).
    Aspose::Pdf::Rectangle Rectangle() const;
    void Rectangle(const Aspose::Pdf::Rectangle& value);

    // Canonical Position — the origin used when no Rectangle is set.
    Aspose::Pdf::Text::Position Position() const noexcept;
    void Position(Aspose::Pdf::Text::Position value) noexcept;

    // Canonical Margin — insets applied to the box before layout.
    const Aspose::Pdf::MarginInfo& Margin() const noexcept;
    void Margin(Aspose::Pdf::MarginInfo value) noexcept;

    // Canonical FirstLineIndent / SubsequentLinesIndent — left indents (pts).
    float FirstLineIndent() const noexcept;
    void FirstLineIndent(float value) noexcept;
    float SubsequentLinesIndent() const noexcept;
    void SubsequentLinesIndent(float value) noexcept;

private:
    friend class TextBuilder;

    struct Line {
        std::string text;
        Aspose::Pdf::Text::Font font;   // empty name → default (Helvetica)
        Aspose::Pdf::Color color;       // transparent → default (black)
        float font_size = 0.0f;     // 0 → default (12 pt)
        float line_spacing = 0.0f;  // 0 → auto (1.2 × font size)
    };

    std::vector<Line> lines_;
    Aspose::Pdf::HorizontalAlignment halign_ =
        Aspose::Pdf::HorizontalAlignment::Left;
    Aspose::Pdf::VerticalAlignment valign_ =
        Aspose::Pdf::VerticalAlignment::Top;
    std::optional<Aspose::Pdf::Rectangle> rect_;
    Aspose::Pdf::Text::Position position_{0.0, 0.0};
    Aspose::Pdf::MarginInfo margin_;
    float first_line_indent_ = 0.0f;
    float subsequent_lines_indent_ = 0.0f;
};

}  // namespace Aspose::Pdf::Text
