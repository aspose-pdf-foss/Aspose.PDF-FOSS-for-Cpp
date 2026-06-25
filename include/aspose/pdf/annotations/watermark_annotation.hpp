#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::WatermarkAnnotation — watermark
// annotation (PDF /Subtype /Watermark per §12.5.6.22). Mirrors
// canonical Aspose.Pdf.Annotations.WatermarkAnnotation; extends
// Annotation directly (not MarkupAnnotation — no author/review
// metadata in canonical).
//
// Phased drops:
//   * SetText(Facades.FormattedText) — Facades cascade
//   * SetTextAndState(String[], Text.TextState) — Text.TextState
//     cascade
//   * ChangeAfterResize(Matrix) — Matrix deferred
//   * FixedPrint — Aspose.Pdf.Annotations.FixedPrint deferred
// =============================================================================

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class WatermarkAnnotation : public Annotation {
public:
    WatermarkAnnotation(Aspose::Pdf::Page& page,
                        Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    double Opacity() const noexcept;
    void Opacity(double value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    double opacity_ = 1.0;
};

}  // namespace Aspose::Pdf::Annotations
