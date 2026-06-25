// Body for Aspose::Pdf::Row — a table row.

#include <aspose/pdf/row.hpp>

namespace Aspose::Pdf {

Cells& Row::Cells() noexcept { return cells_; }
const Aspose::Pdf::Cells& Row::Cells() const noexcept { return cells_; }

const Aspose::Pdf::Color& Row::BackgroundColor() const noexcept {
    return background_color_;
}
void Row::BackgroundColor(const Aspose::Pdf::Color& value) {
    background_color_ = value;
}

const BorderInfo& Row::Border() const noexcept { return border_; }
void Row::Border(const BorderInfo& value) { border_ = value; }

const BorderInfo& Row::DefaultCellBorder() const noexcept {
    return default_cell_border_;
}
void Row::DefaultCellBorder(const BorderInfo& value) {
    default_cell_border_ = value;
}

double Row::MinRowHeight() const noexcept { return min_row_height_; }
void Row::MinRowHeight(double value) { min_row_height_ = value; }
double Row::FixedRowHeight() const noexcept { return fixed_row_height_; }
void Row::FixedRowHeight(double value) { fixed_row_height_ = value; }

bool Row::IsInNewPage() const noexcept { return is_in_new_page_; }
void Row::IsInNewPage(bool value) { is_in_new_page_ = value; }
bool Row::IsRowBroken() const noexcept { return is_row_broken_; }
void Row::IsRowBroken(bool value) { is_row_broken_ = value; }

const Aspose::Pdf::Text::TextState& Row::DefaultCellTextState()
        const noexcept {
    return default_cell_text_state_;
}
void Row::DefaultCellTextState(const Aspose::Pdf::Text::TextState& value) {
    default_cell_text_state_ = value;
}

const MarginInfo& Row::DefaultCellPadding() const noexcept {
    return default_cell_padding_;
}
void Row::DefaultCellPadding(const MarginInfo& value) {
    default_cell_padding_ = value;
}

Aspose::Pdf::VerticalAlignment Row::VerticalAlignment() const noexcept {
    return vertical_alignment_;
}
void Row::VerticalAlignment(Aspose::Pdf::VerticalAlignment value) {
    vertical_alignment_ = value;
}

}  // namespace Aspose::Pdf
