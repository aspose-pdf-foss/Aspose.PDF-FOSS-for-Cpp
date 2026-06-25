#pragma once

// =============================================================================
// Aspose::Pdf::PageCollection — minimum viable v1 surface.
//
// View onto a Document's pages. Exposes the page count, the canonical
// 1-based page indexer (`pages[1]` returns the first page, matching
// Aspose.Pdf semantics; Page::Number is also 1-based), and page-tree
// mutation (Add / Insert / Delete). Mutations rewrite the document via
// an incremental /Kids + /Count update and re-parse the page tree, so
// Count() reflects the change immediately. v1 mutation requires a flat
// /Pages tree.
// =============================================================================

#include <cstddef>
#include <vector>

#include "page.hpp"

namespace Aspose::Pdf {

class Document;

class PageCollection {
public:
    PageCollection(const PageCollection&) = delete;
    PageCollection& operator=(const PageCollection&) = delete;
    PageCollection(PageCollection&&) noexcept = default;
    PageCollection& operator=(PageCollection&&) noexcept = default;
    ~PageCollection() = default;

    // Number of pages currently in the document. Zero for a freshly
    // created empty document.
    std::size_t Count() const noexcept;

    // 1-based page accessor. The first page is `pages[1]`, matching
    // canonical Aspose.Pdf and the Page::Number invariant. Throws
    // std::out_of_range when index < 1 or index > Count().
    Page operator[](int index) const;

    // Append a new blank page (US-Letter, 612 x 792). Returns the new
    // page.
    Page Add();
    // Append a copy of `page` (shallow — shares the source page's
    // content/resources object refs). Same-document only in v1.
    Page Add(const Page& page);
    // Insert a new blank page before 1-based `pageNumber` (append when
    // out of range). Returns the new page.
    Page Insert(int pageNumber);
    // Insert a copy of `page` before 1-based `pageNumber`.
    Page Insert(int pageNumber, const Page& page);
    // Remove all pages.
    void Delete();
    // Remove the 1-based page.
    void Delete(int pageNumber);
    // Remove the given 1-based pages.
    void Delete(const std::vector<int>& pageNumbers);

private:
    friend class Document;
    explicit PageCollection(Document& doc) noexcept;

    Document* doc_ = nullptr;
};

}  // namespace Aspose::Pdf
