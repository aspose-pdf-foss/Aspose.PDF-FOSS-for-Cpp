#pragma once

// =============================================================================
// Aspose::Pdf::Table — a table paragraph (PDF table model). Mirrors canonical
// Aspose.Pdf.Table (Aspose.PDF 26.4.0): a BaseParagraph holding Rows plus
// table-level column widths, borders, default cell styling and placement. The
// render-on-save layout (via Page.Paragraphs) lands with the C-4b beat.
// =============================================================================

#include <string>

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/text_state.hpp>

namespace Aspose::Pdf {

class Table : public BaseParagraph {
public:
    Table();

    // Canonical Table.Rows — the table's rows (mutate in place).
    Aspose::Pdf::Rows& Rows() noexcept;
    const Aspose::Pdf::Rows& Rows() const noexcept;

    // Canonical Table.ColumnWidths — space-separated per-column widths, e.g.
    // "100 200 150".
    const std::string& ColumnWidths() const noexcept;
    void ColumnWidths(std::string value);
    const std::string& DefaultColumnWidth() const noexcept;
    void DefaultColumnWidth(std::string value);

    // Canonical table-level border / cell-default styling.
    const BorderInfo& Border() const noexcept;
    void Border(const BorderInfo& value);
    const BorderInfo& DefaultCellBorder() const noexcept;
    void DefaultCellBorder(const BorderInfo& value);
    const MarginInfo& DefaultCellPadding() const noexcept;
    void DefaultCellPadding(const MarginInfo& value);
    const Aspose::Pdf::Text::TextState& DefaultCellTextState() const noexcept;
    void DefaultCellTextState(const Aspose::Pdf::Text::TextState& value);

    // Canonical Table.Alignment — horizontal placement of the whole table.
    Aspose::Pdf::HorizontalAlignment Alignment() const noexcept;
    void Alignment(Aspose::Pdf::HorizontalAlignment value);

    // Canonical Table.Left / Top — placement of the table's top-left corner.
    float Left() const noexcept;
    void Left(float value);
    float Top() const noexcept;
    void Top(float value);

    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(const Aspose::Pdf::Color& value);

    // Canonical Table.IsBordersIncluded.
    bool IsBordersIncluded() const noexcept;
    void IsBordersIncluded(bool value);

    // Canonical repeating-header counts for multi-page tables.
    int RepeatingRowsCount() const noexcept;
    void RepeatingRowsCount(int value);
    int RepeatingColumnsCount() const noexcept;
    void RepeatingColumnsCount(int value);

    // Canonical Table.IsBroken — when true, a table taller than the page
    // continues onto newly-created continuation pages at Save (the first
    // RepeatingRowsCount rows repeat as a header on each page).
    bool IsBroken() const noexcept;
    void IsBroken(bool value) noexcept;

private:
    Aspose::Pdf::Rows rows_;
    std::string column_widths_;
    std::string default_column_width_;
    BorderInfo border_;
    BorderInfo default_cell_border_;
    MarginInfo default_cell_padding_;
    Aspose::Pdf::Text::TextState default_cell_text_state_;
    Aspose::Pdf::HorizontalAlignment alignment_ =
        Aspose::Pdf::HorizontalAlignment::Left;
    float left_ = 0.0f;
    float top_ = 0.0f;
    Aspose::Pdf::Color background_color_;
    bool is_borders_included_ = true;
    int repeating_rows_count_ = 0;
    int repeating_columns_count_ = 0;
    bool is_broken_ = false;
};

}  // namespace Aspose::Pdf
