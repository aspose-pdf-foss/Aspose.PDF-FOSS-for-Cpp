#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::TextMarkupAnnotation — abstract
// intermediate base for text-decoration annotations (Highlight,
// Underline, StrikeOut, Squiggly — concrete subclasses at A7).
// Mirrors canonical Aspose.Pdf.Annotations.TextMarkupAnnotation
// (Aspose.PDF 26.4.0); extends MarkupAnnotation.
//
// Phased drops on this beat (drops.yaml):
//   * ChangeAfterResize(Matrix) — Aspose.Pdf.Matrix deferred
//   * GetMarkedTextFragments — TextFragmentCollection deferred
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/point.hpp>

namespace Aspose::Pdf::Annotations {

class TextMarkupAnnotation : public MarkupAnnotation {
public:
    TextMarkupAnnotation() noexcept = default;

    // Extract the plaintext spanned by the QuadPoints of this
    // markup. v1 stub returns empty until text-extraction
    // wire-through lands (separate cluster).
    std::string GetMarkedText() const;

    const std::vector<Aspose::Pdf::Point>& QuadPoints() const noexcept;
    void QuadPoints(std::vector<Aspose::Pdf::Point> value) noexcept;

private:
    std::vector<Aspose::Pdf::Point> quad_points_;
};

}  // namespace Aspose::Pdf::Annotations
