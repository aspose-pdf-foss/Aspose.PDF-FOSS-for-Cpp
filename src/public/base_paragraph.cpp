#include <aspose/pdf/base_paragraph.hpp>

#include <memory>
#include <utility>

namespace Aspose::Pdf {

VerticalAlignment BaseParagraph::VerticalAlignment() const noexcept {
    return vertical_alignment_;
}
void BaseParagraph::VerticalAlignment(
        Aspose::Pdf::VerticalAlignment v) noexcept {
    vertical_alignment_ = v;
}

HorizontalAlignment BaseParagraph::HorizontalAlignment() const noexcept {
    return horizontal_alignment_;
}
void BaseParagraph::HorizontalAlignment(
        Aspose::Pdf::HorizontalAlignment v) noexcept {
    horizontal_alignment_ = v;
}

const MarginInfo& BaseParagraph::Margin() const noexcept {
    return margin_;
}
void BaseParagraph::Margin(MarginInfo v) noexcept {
    margin_ = std::move(v);
}

bool BaseParagraph::IsFirstParagraphInColumn() const noexcept {
    return is_first_paragraph_in_column_;
}
void BaseParagraph::IsFirstParagraphInColumn(bool v) noexcept {
    is_first_paragraph_in_column_ = v;
}

bool BaseParagraph::IsKeptWithNext() const noexcept {
    return is_kept_with_next_;
}
void BaseParagraph::IsKeptWithNext(bool v) noexcept {
    is_kept_with_next_ = v;
}

bool BaseParagraph::IsInNewPage() const noexcept {
    return is_in_new_page_;
}
void BaseParagraph::IsInNewPage(bool v) noexcept {
    is_in_new_page_ = v;
}

bool BaseParagraph::IsInLineParagraph() const noexcept {
    return is_in_line_paragraph_;
}
void BaseParagraph::IsInLineParagraph(bool v) noexcept {
    is_in_line_paragraph_ = v;
}

const std::shared_ptr<Hyperlink>&
        BaseParagraph::Hyperlink() const noexcept {
    return hyperlink_;
}
void BaseParagraph::Hyperlink(
        std::shared_ptr<Aspose::Pdf::Hyperlink> v) noexcept {
    hyperlink_ = std::move(v);
}

int BaseParagraph::ZIndex() const noexcept { return z_index_; }
void BaseParagraph::ZIndex(int v) noexcept { z_index_ = v; }

}  // namespace Aspose::Pdf
