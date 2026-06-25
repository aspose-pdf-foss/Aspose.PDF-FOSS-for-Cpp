#pragma once

// =============================================================================
// Aspose::Pdf::Cells — the cell collection of a table row (PDF table model).
// Mirrors canonical Aspose.Pdf.Cells (Aspose.PDF 26.4.0). The Add overloads
// create and append a Cell (optionally pre-filled with text); the collection
// owns its cells. operator[] is 1-based.
// =============================================================================

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_state.hpp>

namespace Aspose::Pdf {

class Cells {
public:
    Cells();
    ~Cells();
    Cells(const Cells& other);             // deep copy (clones each cell)
    Cells& operator=(const Cells& other);
    Cells(Cells&&) noexcept;
    Cells& operator=(Cells&&) noexcept;

    // Canonical Cells.Add overloads — create, append and return a cell.
    Cell& Add();
    Cell& Add(const std::string& text);
    Cell& Add(const std::string& text,
              const Aspose::Pdf::Text::TextState& ts);
    Cell& Add(Aspose::Pdf::Text::TextFragment& textFragment);
    // Append a copy of an existing cell.
    void Add(const Cell& cell);

    // Canonical Cells.Insert — insert a copy at `index` (1-based).
    void Insert(int index, const Cell& cell);

    // Canonical Cells.Remove — remove the matching cell (by index identity).
    void Remove(const Cell& cell);
    // Canonical Cells.RemoveRange — remove `count` cells from `index` (1-based).
    void RemoveRange(int index, int count);

    // Canonical Cells.Count.
    int Count() const noexcept;

    // Canonical indexer — the cell at `index` (1-based).
    Cell& operator[](int index) const;

private:
    std::vector<std::unique_ptr<Cell>> cells_;

    friend class Row;
    friend class Table;
};

}  // namespace Aspose::Pdf
