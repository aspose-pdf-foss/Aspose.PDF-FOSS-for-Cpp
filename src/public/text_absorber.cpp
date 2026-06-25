#include <aspose/pdf/text_absorber.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>

#include "text_extractor.hpp"

#include <span>
#include <string>
#include <utility>

namespace Aspose::Pdf::Text {

struct TextAbsorber::Impl {
    // Accumulated text from prior Visit() calls. Successive Visit()
    // calls append to this buffer (mirroring canonical's behaviour
    // where Text aggregates across visits until the next ctor).
    std::string text;
};

TextAbsorber::TextAbsorber() : impl_(std::make_unique<Impl>()) {}

TextAbsorber::TextAbsorber(TextAbsorber&&) noexcept = default;
TextAbsorber& TextAbsorber::operator=(TextAbsorber&&) noexcept = default;
TextAbsorber::~TextAbsorber() = default;

void TextAbsorber::Visit(const Aspose::Pdf::Document& pdf) {
    const auto& bytes = pdf.bytes_;
    auto pages = foundation::text_extractor::Parse(
        std::span<const std::byte>(bytes.data(), bytes.size()));
    for (auto& page_text : pages) {
        impl_->text += std::move(page_text);
    }
}

void TextAbsorber::Visit(const Aspose::Pdf::Page& page) {
    const auto& bytes = page.doc_->bytes_;
    auto pages = foundation::text_extractor::Parse(
        std::span<const std::byte>(bytes.data(), bytes.size()));
    if (page.leaf_index_ < pages.size()) {
        impl_->text += pages[page.leaf_index_];
    }
}

std::string TextAbsorber::Text() const {
    return impl_->text;
}

bool TextAbsorber::HasErrors() const noexcept {
    return false;
}

}  // namespace Aspose::Pdf::Text
