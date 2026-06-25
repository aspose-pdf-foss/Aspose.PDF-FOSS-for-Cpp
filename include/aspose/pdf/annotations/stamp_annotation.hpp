#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::StampAnnotation — rubber-stamp
// annotation (PDF /Subtype /Stamp per §12.5.6.13). Mirrors
// canonical Aspose.Pdf.Annotations.StampAnnotation; extends
// MarkupAnnotation.
//
// Image carries the stamp's custom image content as raw bytes (the canonical
// System.IO.Stream property; ships on the cpp track via a per-member
// translation override). v1 stores the bytes; rendering them into the stamp's
// /AP appearance stream is a follow-on.
// =============================================================================

#include <cstddef>
#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/stamp_icon.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class StampAnnotation : public MarkupAnnotation {
public:
    explicit StampAnnotation(Aspose::Pdf::Document& document);
    StampAnnotation(Aspose::Pdf::Page& page,
                    Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    StampIcon Icon() const noexcept;
    void Icon(StampIcon value) noexcept;

    // Canonical StampAnnotation.Image — the stamp's custom image content as
    // raw bytes (get/set). Empty when the stamp uses a predefined Icon.
    const std::vector<std::byte>& Image() const noexcept;
    void Image(std::vector<std::byte> value);

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    StampIcon icon_ = StampIcon::Draft;
    std::vector<std::byte> image_;
};

}  // namespace Aspose::Pdf::Annotations
