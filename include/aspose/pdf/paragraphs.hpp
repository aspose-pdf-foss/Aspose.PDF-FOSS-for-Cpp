#pragma once

// =============================================================================
// Aspose::Pdf::Paragraphs — an ordered collection of page paragraphs (PDF
// generated content). Mirrors canonical Aspose.Pdf.Paragraphs (Aspose.PDF
// 26.4.0). Holds non-owning BaseParagraph references (the caller owns the
// paragraph objects, which must outlive the Save); the renderer draws them at
// Save time. operator[] is 1-based.
// =============================================================================

#include <cstddef>
#include <vector>

namespace Aspose::Pdf {

class BaseParagraph;

class Paragraphs {
public:
    Paragraphs() noexcept;

    // Canonical Paragraphs.Add — append a paragraph (non-owning).
    void Add(BaseParagraph& paragraph);
    // Canonical Paragraphs.Insert — insert at `index` (1-based).
    void Insert(int index, BaseParagraph& paragraph);
    // Canonical Paragraphs.Remove — remove the matching paragraph.
    void Remove(BaseParagraph& paragraph);
    // Canonical Paragraphs.RemoveRange — remove `count` from `index` (1-based).
    void RemoveRange(int index, int count);
    // Canonical Paragraphs.Clear.
    void Clear() noexcept;

    // Canonical Paragraphs.Count.
    int Count() const noexcept;
    // Canonical indexer — the paragraph at `index` (1-based).
    BaseParagraph& operator[](int index) const;

private:
    std::vector<BaseParagraph*> items_;

    friend class Document;
};

}  // namespace Aspose::Pdf
