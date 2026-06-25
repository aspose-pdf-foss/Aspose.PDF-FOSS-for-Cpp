// Body for Aspose::Pdf::Cells — a table row's cell collection.

#include <aspose/pdf/cells.hpp>

#include <memory>
#include <stdexcept>
#include <string>

namespace Aspose::Pdf {

Cells::Cells() = default;
Cells::~Cells() = default;

Cells::Cells(const Cells& other) {
    cells_.reserve(other.cells_.size());
    for (const auto& c : other.cells_)
        cells_.push_back(std::make_unique<Cell>(*c));
}

Cells& Cells::operator=(const Cells& other) {
    if (this != &other) {
        cells_.clear();
        cells_.reserve(other.cells_.size());
        for (const auto& c : other.cells_)
            cells_.push_back(std::make_unique<Cell>(*c));
    }
    return *this;
}

Cells::Cells(Cells&&) noexcept = default;
Cells& Cells::operator=(Cells&&) noexcept = default;

Cell& Cells::Add() {
    cells_.push_back(std::make_unique<Cell>());
    return *cells_.back();
}

Cell& Cells::Add(const std::string& text) {
    Cell& c = Add();
    c.text_ = text;
    return c;
}

Cell& Cells::Add(const std::string& text,
                 const Aspose::Pdf::Text::TextState& ts) {
    Cell& c = Add();
    c.text_ = text;
    c.default_cell_text_state_ = ts;
    return c;
}

Cell& Cells::Add(Aspose::Pdf::Text::TextFragment& textFragment) {
    Cell& c = Add();
    c.text_ = textFragment.Text();
    c.default_cell_text_state_ = textFragment.TextState();
    return c;
}

void Cells::Add(const Cell& cell) {
    cells_.push_back(std::make_unique<Cell>(cell));
}

void Cells::Insert(int index, const Cell& cell) {
    if (index < 1 || static_cast<std::size_t>(index) > cells_.size() + 1)
        throw std::out_of_range("Cells::Insert: index");
    cells_.insert(cells_.begin() + (index - 1),
                  std::make_unique<Cell>(cell));
}

void Cells::Remove(const Cell& cell) {
    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        if (it->get() == &cell) {
            cells_.erase(it);
            return;
        }
    }
}

void Cells::RemoveRange(int index, int count) {
    if (index < 1 || count < 0 ||
        static_cast<std::size_t>(index - 1 + count) > cells_.size())
        throw std::out_of_range("Cells::RemoveRange");
    cells_.erase(cells_.begin() + (index - 1),
                 cells_.begin() + (index - 1 + count));
}

int Cells::Count() const noexcept { return static_cast<int>(cells_.size()); }

Cell& Cells::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > cells_.size())
        throw std::out_of_range("Cells::operator[]: index");
    return *cells_[static_cast<std::size_t>(index) - 1];
}

}  // namespace Aspose::Pdf
