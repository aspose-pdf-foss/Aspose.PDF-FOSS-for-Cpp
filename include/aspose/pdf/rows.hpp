#pragma once

// =============================================================================
// Aspose::Pdf::Rows — the row collection of a table (PDF table model). Mirrors
// canonical Aspose.Pdf.Rows (Aspose.PDF 26.4.0). Add() creates and appends a
// row; the collection owns its rows. operator[] is 1-based.
// =============================================================================

#include <cstddef>
#include <memory>
#include <vector>

#include <aspose/pdf/row.hpp>

namespace Aspose::Pdf {

class Rows {
public:
    Rows();
    ~Rows();
    Rows(const Rows&) = delete;
    Rows& operator=(const Rows&) = delete;

    // Canonical Rows.Add — create and append a row, or append a copy.
    Row& Add();
    void Add(const Row& row);

    // Canonical IndexOf — 1-based index of `row`, or 0 when absent.
    int IndexOf(const Row& row) const;
    void Remove(const Row& row);
    void RemoveAt(int index);
    void RemoveRange(int index, int count);

    int Count() const noexcept;
    Row& operator[](int index) const;

private:
    std::vector<std::unique_ptr<Row>> rows_;

    friend class Table;
};

}  // namespace Aspose::Pdf
