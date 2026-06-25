#pragma once

// =============================================================================
// Aspose::Pdf::Row — a table row (PDF table model). Mirrors canonical
// Aspose.Pdf.Row (Aspose.PDF 26.4.0): its Cells plus per-row border /
// background / height / default cell styling. Copyable (deep-copies its cells).
// =============================================================================

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf {

class Table;

class Row {
public:
    Row() = default;

    // Canonical Row.Cells — the row's cell collection (mutate in place).
    Aspose::Pdf::Cells& Cells() noexcept;
    const Aspose::Pdf::Cells& Cells() const noexcept;

    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(const Aspose::Pdf::Color& value);
    const BorderInfo& Border() const noexcept;
    void Border(const BorderInfo& value);
    const BorderInfo& DefaultCellBorder() const noexcept;
    void DefaultCellBorder(const BorderInfo& value);
    double MinRowHeight() const noexcept;
    void MinRowHeight(double value);
    double FixedRowHeight() const noexcept;
    void FixedRowHeight(double value);
    bool IsInNewPage() const noexcept;
    void IsInNewPage(bool value);
    bool IsRowBroken() const noexcept;
    void IsRowBroken(bool value);
    const Aspose::Pdf::Text::TextState& DefaultCellTextState() const noexcept;
    void DefaultCellTextState(const Aspose::Pdf::Text::TextState& value);
    const MarginInfo& DefaultCellPadding() const noexcept;
    void DefaultCellPadding(const MarginInfo& value);
    Aspose::Pdf::VerticalAlignment VerticalAlignment() const noexcept;
    void VerticalAlignment(Aspose::Pdf::VerticalAlignment value);

private:
    Aspose::Pdf::Cells cells_;
    Aspose::Pdf::Color background_color_;
    BorderInfo border_;
    BorderInfo default_cell_border_;
    double min_row_height_ = 0.0;
    double fixed_row_height_ = 0.0;
    bool is_in_new_page_ = false;
    bool is_row_broken_ = false;
    Aspose::Pdf::Text::TextState default_cell_text_state_;
    MarginInfo default_cell_padding_;
    Aspose::Pdf::VerticalAlignment vertical_alignment_ =
        Aspose::Pdf::VerticalAlignment::Top;

    friend class Table;
};

}  // namespace Aspose::Pdf
