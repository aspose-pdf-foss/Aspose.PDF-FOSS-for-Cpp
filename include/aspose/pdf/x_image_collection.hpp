#pragma once

// =============================================================================
// Aspose::Pdf::XImageCollection — the images of a page's resources (PDF
// §8.9 — /Resources /XObject image entries). Mirrors canonical
// Aspose.Pdf.XImageCollection (Aspose.PDF 26.4.0). Enumerate / query the
// images, fetch them by 1-based index or by name, and remove them. The
// collection owns its XImage entries.
// =============================================================================

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/x_image.hpp>

namespace Aspose::Pdf {

class XImageCollection {
public:
    XImageCollection();
    ~XImageCollection();
    XImageCollection(const XImageCollection&) = delete;
    XImageCollection& operator=(const XImageCollection&) = delete;

    // Canonical XImageCollection.Add — stage a new image (raw PNG / JPEG
    // file bytes) for embedding into the page's /Resources /XObject. Returns
    // the assigned image resource name; the XObject is written on
    // Document::Save. (The Stream overload of canonical Add; the bytes carry
    // the source image rather than a System.IO.Stream.)
    std::string Add(const std::vector<std::byte>& image);

    // Canonical XImageCollection.Replace — replace the image at `index`
    // (1-based) with a new one (raw PNG / JPEG file bytes), keeping its
    // resource name so existing references render the new image. Persisted on
    // the owning Document's next Save.
    void Replace(int index, const std::vector<std::byte>& image);

    // Canonical XImageCollection.Count.
    int Count() const noexcept;

    // Canonical XImageCollection.Names — the image resource names.
    std::vector<std::string> Names() const;

    // Canonical indexers — the image at `index` (1-based) or by name.
    XImage& operator[](int index) const;
    XImage& operator[](const std::string& name) const;

    // Canonical Delete — remove the image at `index` (1-based), by name, or all.
    void Delete(int index);
    void Delete(const std::string& name);
    void Delete();
    // Canonical Clear — remove all images (alias Delete()).
    void Clear() noexcept;

    // Canonical Contains / Remove — by reference identity.
    bool Contains(const XImage& image) const noexcept;
    bool Remove(const XImage& image);

    // Canonical GetImageName — the resource name of `image`.
    std::string GetImageName(const XImage& image) const;

private:
    std::vector<std::unique_ptr<XImage>> images_;
    // Resource names of the images present when the page was loaded (set by
    // Document::LoadPageImages). Document::Save diffs this against the
    // surviving loaded images to persist deletions through the saved PDF.
    std::vector<std::string> loaded_names_;

    friend class Document;
    friend class Resources;
};

}  // namespace Aspose::Pdf
