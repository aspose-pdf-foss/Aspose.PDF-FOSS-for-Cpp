#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PolyAnnotation — abstract intermediate
// base for vertex-list shape annotations (Polygon, Polyline).
// Mirrors canonical Aspose.Pdf.Annotations.PolyAnnotation; extends
// MarkupAnnotation.
//
// Phased drops (drops.yaml):
//   * ChangeAfterResize(Matrix) — Matrix deferred
//   * Measure — Aspose.Pdf.Annotations.Measure deferred
// =============================================================================

#include <vector>

#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/poly_intent.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/point.hpp>

namespace Aspose::Pdf::Annotations {

class PolyAnnotation : public MarkupAnnotation {
public:
    PolyAnnotation() noexcept = default;

    const std::vector<Aspose::Pdf::Point>& Vertices() const noexcept;
    void Vertices(std::vector<Aspose::Pdf::Point> value) noexcept;

    Aspose::Pdf::Color InteriorColor() const noexcept;
    void InteriorColor(Aspose::Pdf::Color value) noexcept;

    LineEnding StartingStyle() const noexcept;
    void StartingStyle(LineEnding value) noexcept;

    LineEnding EndingStyle() const noexcept;
    void EndingStyle(LineEnding value) noexcept;

    PolyIntent Intent() const noexcept;
    void Intent(PolyIntent value) noexcept;

private:
    std::vector<Aspose::Pdf::Point> vertices_;
    Aspose::Pdf::Color interior_color_;
    LineEnding starting_style_ = LineEnding::None;
    LineEnding ending_style_ = LineEnding::None;
    PolyIntent intent_ = PolyIntent::Undefined;
};

}  // namespace Aspose::Pdf::Annotations
