#include <aspose/pdf/text_fragment_collection.hpp>

#include <stdexcept>

namespace Aspose::Pdf::Text {

int TextFragmentCollection::Count() const noexcept {
    return static_cast<int>(items_.size());
}

TextFragment& TextFragmentCollection::operator[](int index) {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size()) {
        throw std::out_of_range("TextFragmentCollection: index out of range");
    }
    return items_[static_cast<std::size_t>(index - 1)];
}

const TextFragment& TextFragmentCollection::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size()) {
        throw std::out_of_range("TextFragmentCollection: index out of range");
    }
    return items_[static_cast<std::size_t>(index - 1)];
}

}  // namespace Aspose::Pdf::Text
