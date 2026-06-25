#include <aspose/pdf/text_fragment.hpp>

#include <utility>

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Text {

TextFragment::TextFragment(std::string text) : text_(std::move(text)) {}

const std::string& TextFragment::Text() const noexcept { return text_; }

void TextFragment::Text(std::string value) {
    text_ = std::move(value);
    if (doc_ == nullptr) return;  // unlinked fragment: store only

    // Rebuild the show literal with the matched run replaced, and register
    // the edit for the next Save().
    std::string new_literal = original_literal_;
    new_literal.replace(match_offset_, match_len_, text_);
    Aspose::Pdf::Document::TextEdit edit;
    edit.stream_id = stream_id_;
    edit.occurrence = occurrence_;
    edit.old_literal = original_literal_;
    edit.new_literal = std::move(new_literal);
    doc_->RegisterTextEditInternal(edit);

    // Subsequent edits to this fragment chain from the new literal.
    original_literal_ = edit.new_literal;
    match_len_ = text_.size();
}

Aspose::Pdf::Rectangle TextFragment::Rectangle() const {
    return Aspose::Pdf::Rectangle(rx_, ry_, rx_ + rw_, ry_ + rh_, false);
}

Aspose::Pdf::Text::TextFragmentState& TextFragment::TextState() noexcept {
    return state_;
}

Aspose::Pdf::Text::Position TextFragment::Position() const noexcept {
    return position_;
}

void TextFragment::Position(Aspose::Pdf::Text::Position value) noexcept {
    position_ = value;
}

}  // namespace Aspose::Pdf::Text
