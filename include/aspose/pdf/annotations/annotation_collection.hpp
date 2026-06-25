#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::AnnotationCollection — per-page
// annotation collection. Mirrors canonical Aspose.Pdf.Annotations.
// AnnotationCollection (Aspose.PDF 26.4.0).
//
// Storage model: the collection holds non-owning Annotation
// pointers. Concrete annotation instances must outlive the
// collection (typical case: they live on the Document /
// Page that owns the collection). This matches canonical .NET
// semantics where Annotation is a reference type and the
// collection holds references, not copies.
//
// Phased drops on this beat (drops.yaml):
//   * Accept(AnnotationSelector) — A5
//   * CopyTo(Annotation[], int) — abstract-array semantic
//   * IsSynchronized + SyncRoot — System.Collections legacy
//   * FindByName(string) — abstract return-type idiom mismatch
//   * GetEnumerator — auto-drops via System.Collections.Generic
//     .IEnumerator translations cascade
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Annotations {

class Annotation;
class AnnotationSelector;

class AnnotationCollection {
public:
    // Both special members are out-of-line: owned_ holds
    // unique_ptr<Annotation>, an incomplete type in this header, so the
    // member (con/de)structions are emitted in the .cpp where Annotation
    // is complete.
    AnnotationCollection() noexcept;
    ~AnnotationCollection();

    // ---- Mutation ----

    // Add an annotation. `considerRotation` (canonical 2-arg
    // overload) hints that the writer should account for the
    // owning page's rotation when persisting the /Rect entry;
    // it's stored as a flag on the collection-side entry but
    // does not transform the annotation's own Rect.
    void Add(Annotation& annotation, bool considerRotation);
    void Add(Annotation& annotation);

    // Delete the annotation at `index`. Throws std::out_of_range
    // on bad index.
    void Delete(int index);

    // Delete all annotations (alias for Clear()).
    void Delete();

    // Delete the annotation matching `annotation` by reference
    // identity. No-op when the annotation isn't in the
    // collection.
    void Delete(const Annotation& annotation);

    // Remove all entries.
    void Clear();

    // Remove the annotation matching by reference identity.
    // Returns true iff the annotation was found and removed.
    bool Remove(const Annotation& annotation);

    // ---- Query ----

    bool Contains(const Annotation& annotation) const noexcept;

    int Count() const noexcept;

    // Always false in v1 — the collection is mutable.
    bool IsReadOnly() const noexcept;

    // Index-based accessor — throws std::out_of_range on bad
    // index.
    Annotation& operator[](int index) const;

    // ---- Visitor ----

    // Iterate every annotation in this collection and dispatch
    // each through the visitor's Accept(visitor) — typical
    // collection-traversal idiom for the visitor pattern.
    void Accept(AnnotationSelector& visitor);

private:
    // Document populates these on load-on-open (Page::Annotations()):
    // annotations read back from a page's /Annots are owned here, raw
    // pointers registered in items_, and the source object id kept in
    // loaded_ids_ so a re-save reuses the original object (lossless —
    // loaded annotations are not rewritten unless removed).
    friend class Aspose::Pdf::Document;
    std::vector<std::unique_ptr<Annotation>> owned_;
    std::map<const Annotation*, std::uint32_t> loaded_ids_;
    std::size_t loaded_count_ = 0;

    std::vector<Annotation*> items_;
    std::vector<bool> consider_rotation_;
};

}  // namespace Aspose::Pdf::Annotations
