#pragma once

// =============================================================================
// Aspose::Pdf::Page — minimum viable v1 surface.
//
// Single page in a PDF document. v1 exposes the 1-based page index +
// the per-page Annotations collection (A14 wire-through). Annotation
// instances live on the call-site; the collection holds non-owning
// references and round-trips through `page.Annotations().Add(...)`.
// Every other canonical accessor (Rect / CropBox / Rotate / Resources
// / vector graphics / image insertion / text extraction) is out of
// scope until the foundation primitives that back them are stable —
// see capabilities/page.yaml.
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

namespace Aspose::Pdf {
namespace Text { class TextAbsorber; }
namespace Text { class TextFragmentAbsorber; }
namespace Text { class TextBuilder; }
namespace Devices {
class ImageDevice;
class PngDevice;
class JpegDevice;
class BmpDevice;
class TextDevice;
class TiffDevice;
}
}

namespace Aspose::Pdf {
namespace Annotations { class AnnotationCollection; }
namespace Annotations { class RedactionAnnotation; }
class ArtifactCollection;
}

namespace Aspose::Pdf {

class Document;
class Paragraphs;
class Resources;

class Page {
public:
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;
    Page(Page&&) noexcept = default;
    Page& operator=(Page&&) noexcept = default;
    ~Page() = default;

    // 1-based index of this page within the owning document. The first
    // page is 1, not 0. Always positive.
    std::int64_t Number() const noexcept;

    // ---- Geometry (PDF §7.7.3.3) ----
    // Rect / MediaBox give the page's media rectangle; CropBox the
    // visible region (defaults to MediaBox). Rotate is the page
    // rotation. Getters resolve from the parsed page tree (MediaBox
    // origin assumed (0,0) for unedited pages); setters stage the
    // change, persisted by the owning Document's next Save().
    Rectangle Rect() const;
    void Rect(const Rectangle& value);
    Rectangle MediaBox() const;
    void MediaBox(const Rectangle& value);
    Rectangle CropBox() const;
    void CropBox(const Rectangle& value);
    // Canonical TrimBox / BleedBox / ArtBox (PDF §14.11.2). Each defaults to
    // the CropBox until set; the setter stages a write persisted at Save.
    Rectangle TrimBox() const;
    void TrimBox(const Rectangle& value);
    Rectangle BleedBox() const;
    void BleedBox(const Rectangle& value);
    Rectangle ArtBox() const;
    void ArtBox(const Rectangle& value);
    Rotation Rotate() const;
    void Rotate(Rotation value);

    // Canonical GetPageRect(isMediaBox): true → MediaBox, false →
    // CropBox.
    Rectangle GetPageRect(bool isMediaBox) const;

    // Resize the page's media box to width × height (origin (0,0)).
    void SetPageSize(double width, double height);

    // Embed an image file (JPEG/PNG) and draw it in `rect`. Returns
    // true on success (persisted by the owning Document's next Save).
    bool AddImage(const std::string& imageFile, const Rectangle& rect);

    // Canonical Page.AddImage(Stream, Rectangle, …): embed an in-memory
    // image (PNG / JPEG bytes) and draw it in `imageRect`. When
    // `autoAdjustRectangle` is true the placement is shrunk to preserve the
    // image's aspect ratio and centred within `imageRect`; when `bbox` is
    // non-empty the draw is clipped to it. Persisted by the owning
    // Document's next Save. Throws std::runtime_error on unsupported /
    // unreadable image bytes.
    void AddImage(const std::vector<std::byte>& imageStream,
                  const Rectangle& imageRect,
                  const Rectangle& bbox = Rectangle::Empty(),
                  bool autoAdjustRectangle = true);

    // Per-page annotation collection. Storage lives on the
    // owning Document (see Document::page_annotations_) — Pages
    // are value-returned by PageCollection so they can't own
    // per-page state directly. The collection is lazily created
    // on first access; subsequent calls return the same instance
    // for the same (Document, leaf_index) pair.
    Annotations::AnnotationCollection& Annotations() const;

    // Canonical Page.Paragraphs — the page's generated paragraphs (e.g.
    // Tables), rendered into the content stream at Save.
    Aspose::Pdf::Paragraphs& Paragraphs() const;

    // Canonical Page.Resources — the page's resource dictionary (its images).
    Aspose::Pdf::Resources& Resources() const;

    // Canonical Page.Artifacts — the page's artifacts (e.g. watermarks),
    // rendered into the content stream at Save. Storage lives on the owning
    // Document (Pages are value-returned). v1 is write-focused: artifacts
    // added here render on Save; pre-existing artifacts are not enumerated.
    Aspose::Pdf::ArtifactCollection& Artifacts() const;

private:
    friend class PageCollection;
    friend class Aspose::Pdf::Annotations::RedactionAnnotation;
    friend class Aspose::Pdf::Text::TextAbsorber;
    friend class Aspose::Pdf::Text::TextFragmentAbsorber;
    friend class Aspose::Pdf::Text::TextBuilder;
    friend class Aspose::Pdf::Devices::ImageDevice;
    friend class Aspose::Pdf::Devices::PngDevice;
    friend class Aspose::Pdf::Devices::JpegDevice;
    friend class Aspose::Pdf::Devices::BmpDevice;
    friend class Aspose::Pdf::Devices::TextDevice;
    friend class Aspose::Pdf::Devices::TiffDevice;

    Page(Document& doc, std::size_t leaf_index) noexcept;

    Document* doc_ = nullptr;
    std::size_t leaf_index_ = 0;
};

}  // namespace Aspose::Pdf
