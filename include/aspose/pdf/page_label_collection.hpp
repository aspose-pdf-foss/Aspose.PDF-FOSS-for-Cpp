#pragma once

// =============================================================================
// Aspose::Pdf::PageLabelCollection — the document's page-label ranges (PDF
// §12.4.2 — the Catalog's /PageLabels number tree). Mirrors canonical
// Aspose.Pdf.PageLabelCollection (Aspose.PDF 26.4.0): a map from a 0-based
// range-start page index to the PageLabel that numbers pages from there until
// the next range begins.
//
// Persistence: load-on-open reads the /PageLabels number tree; Save() writes
// it back (incremental update).
// =============================================================================

#include <cstddef>
#include <map>
#include <vector>

#include <aspose/pdf/page_label.hpp>

namespace Aspose::Pdf {

class PageLabelCollection {
public:
    PageLabelCollection() noexcept;

    // Canonical GetLabel — the PageLabel active at `pageIndex` (the range with
    // the largest start <= pageIndex). Returns a default PageLabel when no
    // range covers the index.
    PageLabel GetLabel(int pageIndex) const;

    // Canonical UpdateLabel — set the range starting at `pageIndex` to
    // `pageLabel` (replacing any existing range with that start).
    void UpdateLabel(int pageIndex, const PageLabel& pageLabel);

    // Canonical RemoveLabel — remove the range starting at `pageIndex`.
    // Returns true iff a range with that start existed.
    bool RemoveLabel(int pageIndex);

    // Canonical GetPages — the range-start page indices, ascending.
    std::vector<int> GetPages() const;

private:
    std::map<int, PageLabel> ranges_;  // range-start index → label

    // Count of ranges populated by load-on-open (lets Save() tell "nothing to
    // persist" from "loaded ranges were all removed → clear the tree").
    std::size_t loaded_count_ = 0;

    friend class Document;
};

}  // namespace Aspose::Pdf
