// Body for Aspose::Pdf::Cell — a table cell.

#include <aspose/pdf/cell.hpp>

#include <string>
#include <utility>

namespace Aspose::Pdf {

Cell::Cell() = default;

Cell::Cell(const Rectangle& rect) {
    width_ = rect.URX() - rect.LLX();
}

bool Cell::IsNoBorder() const noexcept { return is_no_border_; }
void Cell::IsNoBorder(bool value) { is_no_border_ = value; }

const MarginInfo& Cell::Margin() const noexcept { return margin_; }
void Cell::Margin(const MarginInfo& value) { margin_ = value; }

const BorderInfo& Cell::Border() const noexcept { return border_; }
void Cell::Border(const BorderInfo& value) { border_ = value; }

const Aspose::Pdf::Color& Cell::BackgroundColor() const noexcept {
    return background_color_;
}
void Cell::BackgroundColor(const Aspose::Pdf::Color& value) {
    background_color_ = value;
}

const std::string& Cell::BackgroundImageFile() const noexcept {
    return background_image_file_;
}
void Cell::BackgroundImageFile(std::string value) {
    background_image_file_ = std::move(value);
}

const std::vector<std::byte>& Cell::BackgroundImage() const noexcept {
    return background_image_;
}
void Cell::BackgroundImage(std::vector<std::byte> value) {
    background_image_ = std::move(value);
}

HorizontalAlignment Cell::Alignment() const noexcept { return alignment_; }
void Cell::Alignment(HorizontalAlignment value) { alignment_ = value; }

const Aspose::Pdf::Text::TextState& Cell::DefaultCellTextState()
        const noexcept {
    return default_cell_text_state_;
}
void Cell::DefaultCellTextState(const Aspose::Pdf::Text::TextState& value) {
    default_cell_text_state_ = value;
}

bool Cell::IsOverrideByFragment() const noexcept {
    return is_override_by_fragment_;
}
void Cell::IsOverrideByFragment(bool value) {
    is_override_by_fragment_ = value;
}

bool Cell::IsWordWrapped() const noexcept { return is_word_wrapped_; }
void Cell::IsWordWrapped(bool value) { is_word_wrapped_ = value; }

Aspose::Pdf::VerticalAlignment Cell::VerticalAlignment() const noexcept {
    return vertical_alignment_;
}
void Cell::VerticalAlignment(Aspose::Pdf::VerticalAlignment value) {
    vertical_alignment_ = value;
}

int Cell::ColSpan() const noexcept { return col_span_; }
void Cell::ColSpan(int value) { col_span_ = value; }
int Cell::RowSpan() const noexcept { return row_span_; }
void Cell::RowSpan(int value) { row_span_ = value; }

double Cell::Width() const noexcept { return width_; }

}  // namespace Aspose::Pdf
