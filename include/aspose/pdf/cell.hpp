#pragma once

// =============================================================================
// Aspose::Pdf::Cell — a table cell (PDF table model). Mirrors canonical
// Aspose.Pdf.Cell (Aspose.PDF 26.4.0): per-cell border / background / margins /
// alignment / span. Cell content is text, supplied through the owning
// Cells.Add(string / TextFragment) overloads (the canonical Paragraphs
// collection lands with the Paragraphs/G0 beat).
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf {

class Cells;
class Row;
class Table;

class Cell {
public:
    Cell();
    explicit Cell(const Rectangle& rect);

    // Canonical Cell.IsNoBorder — suppress the cell's own border.
    bool IsNoBorder() const noexcept;
    void IsNoBorder(bool value);

    // Canonical Cell.Margin — inner padding.
    const MarginInfo& Margin() const noexcept;
    void Margin(const MarginInfo& value);

    // Canonical Cell.Border — the cell border styling.
    const BorderInfo& Border() const noexcept;
    void Border(const BorderInfo& value);

    // Canonical Cell.BackgroundColor.
    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(const Aspose::Pdf::Color& value);

    // Canonical Cell.BackgroundImageFile — path of a background image.
    const std::string& BackgroundImageFile() const noexcept;
    void BackgroundImageFile(std::string value);

    // Canonical Cell.BackgroundImage — the cell's background image as raw
    // PNG / JPEG file bytes (rendered behind the cell text at Save).
    const std::vector<std::byte>& BackgroundImage() const noexcept;
    void BackgroundImage(std::vector<std::byte> value);

    // Canonical Cell.Alignment — horizontal text alignment.
    HorizontalAlignment Alignment() const noexcept;
    void Alignment(HorizontalAlignment value);

    // Canonical Cell.DefaultCellTextState — text styling for the cell content.
    const Aspose::Pdf::Text::TextState& DefaultCellTextState() const noexcept;
    void DefaultCellTextState(const Aspose::Pdf::Text::TextState& value);

    // Canonical Cell.IsOverrideByFragment.
    bool IsOverrideByFragment() const noexcept;
    void IsOverrideByFragment(bool value);

    // Canonical Cell.IsWordWrapped.
    bool IsWordWrapped() const noexcept;
    void IsWordWrapped(bool value);

    // Canonical Cell.VerticalAlignment.
    Aspose::Pdf::VerticalAlignment VerticalAlignment() const noexcept;
    void VerticalAlignment(Aspose::Pdf::VerticalAlignment value);

    // Canonical Cell.ColSpan / RowSpan.
    int ColSpan() const noexcept;
    void ColSpan(int value);
    int RowSpan() const noexcept;
    void RowSpan(int value);

    // Canonical Cell.Width — the laid-out cell width (read-only).
    double Width() const noexcept;

private:
    bool is_no_border_ = false;
    MarginInfo margin_;
    BorderInfo border_;
    Aspose::Pdf::Color background_color_;
    std::string background_image_file_;
    std::vector<std::byte> background_image_;
    HorizontalAlignment alignment_ = HorizontalAlignment::Left;
    Aspose::Pdf::Text::TextState default_cell_text_state_;
    bool is_override_by_fragment_ = false;
    bool is_word_wrapped_ = true;
    Aspose::Pdf::VerticalAlignment vertical_alignment_ =
        Aspose::Pdf::VerticalAlignment::Top;
    int col_span_ = 1;
    int row_span_ = 1;
    double width_ = 0.0;
    std::string text_;  // cell content (set via Cells.Add)

    friend class Cells;
    friend class Row;
    friend class Table;
    friend class Document;  // the table renderer reads cell text
};

}  // namespace Aspose::Pdf
