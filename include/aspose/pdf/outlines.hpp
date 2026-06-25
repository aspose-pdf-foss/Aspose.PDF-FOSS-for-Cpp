#pragma once

// =============================================================================
// Aspose::Pdf::Outlines — abstract base of the document-outline (bookmark)
// tree (PDF §12.3.3). Mirrors canonical Aspose.Pdf.Outlines (Aspose.PDF
// 26.4.0): a collection of OutlineItemCollection children that owns its
// nodes. Both the document-level OutlineCollection and each
// OutlineItemCollection node are Outlines.
//
// Ownership: the collection owns its child nodes (deep copy on Add). Accessors
// (First / Last / operator[]) return non-owning pointers/references into the
// owned tree. operator[] is 1-based, matching the other Aspose.Pdf
// collections (PageCollection, …).
// =============================================================================

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Aspose::Pdf {

class OutlineItemCollection;

class Outlines {
public:
    virtual ~Outlines();

    // Canonical Outlines.Count — number of direct children.
    int Count() const noexcept;

    // Canonical Outlines.IsReadOnly — always false (the tree is mutable).
    bool IsReadOnly() const noexcept;

    // Canonical Outlines.VisibleCount — children plus, for every open child,
    // its visible descendants (the count the viewer would show expanded).
    int VisibleCount() const noexcept;

    // Canonical Outlines.Add — append `item` as the last child (deep copy).
    void Add(OutlineItemCollection& item);

    // Canonical Outlines.Clear — remove all children.
    void Clear() noexcept;

    // Canonical Outlines.Contains — true iff `item` matches a direct child by
    // title + structure.
    bool Contains(const OutlineItemCollection& item) const noexcept;

    // Canonical Outlines.Remove — remove the matching direct child. Returns
    // true iff one was removed.
    bool Remove(OutlineItemCollection& item);

    // Canonical Remove(int) / Delete() — remove the child at `index` (1-based)
    // / remove all children.
    void Remove(int index);
    void Delete() noexcept;

    // Canonical Delete(string) — remove the first direct child whose Title
    // matches `name`. No-op when absent.
    void Delete(const std::string& name);

    // Canonical First / Last — the first / last child (nullptr when empty).
    OutlineItemCollection* First() const noexcept;
    OutlineItemCollection* Last() const noexcept;

    // Canonical indexer — the child at `index` (1-based). Throws
    // std::out_of_range on a bad index.
    OutlineItemCollection& operator[](int index) const;

protected:
    Outlines();

    // Attach `child` (already owned in children_) — set its parent link.
    void Adopt(OutlineItemCollection& child);

    std::vector<std::unique_ptr<OutlineItemCollection>> children_;

    friend class OutlineItemCollection;
    friend class Document;
};

}  // namespace Aspose::Pdf
