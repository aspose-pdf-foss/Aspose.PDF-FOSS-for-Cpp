// Body for Aspose::Pdf::Table — a table paragraph.

#include <aspose/pdf/table.hpp>

#include <string>
#include <utility>

namespace Aspose::Pdf {

Table::Table() = default;

Rows& Table::Rows() noexcept { return rows_; }
const Aspose::Pdf::Rows& Table::Rows() const noexcept { return rows_; }

const std::string& Table::ColumnWidths() const noexcept {
    return column_widths_;
}
void Table::ColumnWidths(std::string value) {
    column_widths_ = std::move(value);
}
const std::string& Table::DefaultColumnWidth() const noexcept {
    return default_column_width_;
}
void Table::DefaultColumnWidth(std::string value) {
    default_column_width_ = std::move(value);
}

const BorderInfo& Table::Border() const noexcept { return border_; }
void Table::Border(const BorderInfo& value) { border_ = value; }
const BorderInfo& Table::DefaultCellBorder() const noexcept {
    return default_cell_border_;
}
void Table::DefaultCellBorder(const BorderInfo& value) {
    default_cell_border_ = value;
}
const MarginInfo& Table::DefaultCellPadding() const noexcept {
    return default_cell_padding_;
}
void Table::DefaultCellPadding(const MarginInfo& value) {
    default_cell_padding_ = value;
}
const Aspose::Pdf::Text::TextState& Table::DefaultCellTextState()
        const noexcept {
    return default_cell_text_state_;
}
void Table::DefaultCellTextState(const Aspose::Pdf::Text::TextState& value) {
    default_cell_text_state_ = value;
}

Aspose::Pdf::HorizontalAlignment Table::Alignment() const noexcept {
    return alignment_;
}
void Table::Alignment(Aspose::Pdf::HorizontalAlignment value) {
    alignment_ = value;
}

float Table::Left() const noexcept { return left_; }
void Table::Left(float value) { left_ = value; }
float Table::Top() const noexcept { return top_; }
void Table::Top(float value) { top_ = value; }

const Aspose::Pdf::Color& Table::BackgroundColor() const noexcept {
    return background_color_;
}
void Table::BackgroundColor(const Aspose::Pdf::Color& value) {
    background_color_ = value;
}

bool Table::IsBordersIncluded() const noexcept { return is_borders_included_; }
void Table::IsBordersIncluded(bool value) { is_borders_included_ = value; }

int Table::RepeatingRowsCount() const noexcept { return repeating_rows_count_; }
void Table::RepeatingRowsCount(int value) { repeating_rows_count_ = value; }
int Table::RepeatingColumnsCount() const noexcept {
    return repeating_columns_count_;
}
void Table::RepeatingColumnsCount(int value) {
    repeating_columns_count_ = value;
}
bool Table::IsBroken() const noexcept { return is_broken_; }
void Table::IsBroken(bool value) noexcept { is_broken_ = value; }

}  // namespace Aspose::Pdf
