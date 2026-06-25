#include <aspose/pdf/annotations/default_appearance.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

double DefaultAppearance::FontSize() const noexcept {
    return font_size_;
}
void DefaultAppearance::FontSize(double v) noexcept {
    font_size_ = v;
    text_cache_.clear();
}

const std::string& DefaultAppearance::FontName() const noexcept {
    return font_name_;
}
void DefaultAppearance::FontName(std::string v) noexcept {
    font_name_ = std::move(v);
    text_cache_.clear();
}

const std::string& DefaultAppearance::FontResourceName() const noexcept {
    return font_resource_name_;
}
void DefaultAppearance::FontResourceName(std::string v) noexcept {
    font_resource_name_ = std::move(v);
    text_cache_.clear();
}

const std::string& DefaultAppearance::Text() const noexcept {
    if (text_cache_.empty() && !font_name_.empty()) {
        text_cache_ = font_name_ + " "
            + std::to_string(font_size_);
    }
    return text_cache_;
}

}  // namespace Aspose::Pdf::Annotations
