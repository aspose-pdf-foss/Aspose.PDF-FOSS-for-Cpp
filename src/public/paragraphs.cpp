// Body for Aspose::Pdf::Paragraphs — page paragraph collection.

#include <aspose/pdf/paragraphs.hpp>

#include <stdexcept>

#include <aspose/pdf/base_paragraph.hpp>

namespace Aspose::Pdf {

Paragraphs::Paragraphs() noexcept = default;

void Paragraphs::Add(BaseParagraph& paragraph) {
    items_.push_back(&paragraph);
}

void Paragraphs::Insert(int index, BaseParagraph& paragraph) {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size() + 1)
        throw std::out_of_range("Paragraphs::Insert: index");
    items_.insert(items_.begin() + (index - 1), &paragraph);
}

void Paragraphs::Remove(BaseParagraph& paragraph) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (*it == &paragraph) {
            items_.erase(it);
            return;
        }
    }
}

void Paragraphs::RemoveRange(int index, int count) {
    if (index < 1 || count < 0 ||
        static_cast<std::size_t>(index - 1 + count) > items_.size())
        throw std::out_of_range("Paragraphs::RemoveRange");
    items_.erase(items_.begin() + (index - 1),
                 items_.begin() + (index - 1 + count));
}

void Paragraphs::Clear() noexcept { items_.clear(); }

int Paragraphs::Count() const noexcept {
    return static_cast<int>(items_.size());
}

BaseParagraph& Paragraphs::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size())
        throw std::out_of_range("Paragraphs::operator[]: index");
    return *items_[static_cast<std::size_t>(index) - 1];
}

}  // namespace Aspose::Pdf
