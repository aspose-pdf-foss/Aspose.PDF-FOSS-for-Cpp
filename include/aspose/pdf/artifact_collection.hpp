#pragma once

// =============================================================================
// Aspose::Pdf::ArtifactCollection — the artifacts of a page (PDF §14.8.2.2).
// Mirrors canonical Aspose.Pdf.ArtifactCollection (Aspose.PDF 26.4.0): add /
// remove / replace artifacts and enumerate them by 1-based index. Obtained via
// Page::Artifacts(). Added artifacts are non-owning references (the caller
// keeps them alive) and are rendered into the page on the owning Document's
// next Save().
//
// v1 subset: the collection is write-focused — artifacts added here are drawn
// on Save; pre-existing page artifacts are not enumerated back.
// =============================================================================

#include <cstddef>
#include <vector>

namespace Aspose::Pdf {

class Artifact;

class ArtifactCollection {
public:
    ArtifactCollection();
    ~ArtifactCollection();
    ArtifactCollection(const ArtifactCollection&) = delete;
    ArtifactCollection& operator=(const ArtifactCollection&) = delete;

    // Canonical Add — append `artifact` (a non-owning reference; the caller
    // owns its lifetime until the next Save()).
    void Add(Artifact& artifact);

    // Canonical Delete — remove the artifact at `index` (1-based) or the one
    // matching `artifact` by reference identity.
    void Delete(int index);
    void Delete(const Artifact& artifact);

    // Canonical Update — replace the artifact at the position of an existing
    // one matched by reference identity (no-op if not present). Modelled as a
    // membership refresh: re-stages `artifact` so its current state renders.
    void Update(Artifact& artifact);

    // Canonical Count.
    int Count() const noexcept;

    // Canonical indexer — the artifact at `index` (1-based).
    Artifact& operator[](int index) const;

private:
    std::vector<Artifact*> items_;

    friend class Document;
};

}  // namespace Aspose::Pdf
