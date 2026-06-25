// Body for Aspose::Pdf::Rows — a table's row collection.

#include <aspose/pdf/rows.hpp>

#include <memory>
#include <stdexcept>

namespace Aspose::Pdf {

Rows::Rows() = default;
Rows::~Rows() = default;

Row& Rows::Add() {
    rows_.push_back(std::make_unique<Row>());
    return *rows_.back();
}

void Rows::Add(const Row& row) {
    rows_.push_back(std::make_unique<Row>(row));
}

int Rows::IndexOf(const Row& row) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].get() == &row) return static_cast<int>(i) + 1;
    }
    return 0;
}

void Rows::Remove(const Row& row) {
    for (auto it = rows_.begin(); it != rows_.end(); ++it) {
        if (it->get() == &row) {
            rows_.erase(it);
            return;
        }
    }
}

void Rows::RemoveAt(int index) {
    if (index < 1 || static_cast<std::size_t>(index) > rows_.size())
        throw std::out_of_range("Rows::RemoveAt");
    rows_.erase(rows_.begin() + (index - 1));
}

void Rows::RemoveRange(int index, int count) {
    if (index < 1 || count < 0 ||
        static_cast<std::size_t>(index - 1 + count) > rows_.size())
        throw std::out_of_range("Rows::RemoveRange");
    rows_.erase(rows_.begin() + (index - 1),
                rows_.begin() + (index - 1 + count));
}

int Rows::Count() const noexcept { return static_cast<int>(rows_.size()); }

Row& Rows::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > rows_.size())
        throw std::out_of_range("Rows::operator[]: index");
    return *rows_[static_cast<std::size_t>(index) - 1];
}

}  // namespace Aspose::Pdf
