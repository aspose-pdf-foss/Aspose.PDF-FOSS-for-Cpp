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

    // The page text under the QuadPoints (the Rect when there are none):
    // characters whose centre lies in a quadrilateral's box, lines
    // separated by '\n', words by a space. Character extents use the
    // text extractor's 0.5 em estimate. Throws std::logic_error when the
    // annotation is not on a page.
    std::string GetMarkedText() const;

    const std::vector<Aspose::Pdf::Point>& QuadPoints() const noexcept;
    void QuadPoints(std::vector<Aspose::Pdf::Point> value) noexcept;

private:
    std::vector<Aspose::Pdf::Point> quad_points_;
    friend class Aspose::Pdf::Document;
};

}  // namespace Aspose::Pdf::Annotations
