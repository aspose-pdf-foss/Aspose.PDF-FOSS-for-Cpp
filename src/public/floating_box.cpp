// Body for Aspose::Pdf::FloatingBox. The box frame is rendered at Save by
// Document::AppendParagraphsUpdate (the shared Page.Paragraphs renderer).

#include <aspose/pdf/floating_box.hpp>

#include <utility>

namespace Aspose::Pdf {

FloatingBox::FloatingBox() = default;

FloatingBox::FloatingBox(float width, float height)
    : width_(static_cast<double>(width)),
      height_(static_cast<double>(height)) {}

double FloatingBox::Width() const noexcept { return width_; }
void FloatingBox::Width(double value) noexcept { width_ = value; }

double FloatingBox::Height() const noexcept { return height_; }
void FloatingBox::Height(double value) noexcept { height_ = value; }

bool FloatingBox::IsNeedRepeating() const noexcept {
    return is_need_repeating_;
}
void FloatingBox::IsNeedRepeating(bool value) noexcept {
    is_need_repeating_ = value;
}

Aspose::Pdf::Paragraphs& FloatingBox::Paragraphs() noexcept {
    return paragraphs_;
}
void FloatingBox::Paragraphs(Aspose::Pdf::Paragraphs value) {
    paragraphs_ = std::move(value);
}

const Aspose::Pdf::BorderInfo& FloatingBox::Border() const noexcept {
    return border_;
}
void FloatingBox::Border(Aspose::Pdf::BorderInfo value) {
    border_ = std::move(value);
}

const Aspose::Pdf::Color& FloatingBox::BackgroundColor() const noexcept {
    return background_color_;
}
void FloatingBox::BackgroundColor(Aspose::Pdf::Color value) {
    background_color_ = std::move(value);
}

}  // namespace Aspose::Pdf
