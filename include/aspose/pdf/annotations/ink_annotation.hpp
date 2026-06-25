#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::InkAnnotation — freehand annotation
// (PDF /Subtype /Ink per §12.5.6.12). Mirrors canonical
// Aspose.Pdf.Annotations.InkAnnotation; extends MarkupAnnotation.
//
// InkList is a list of strokes; each stroke is a list of Points.
// Canonical IList<Point[]> translates to std::vector<std::vector<Point>>
// via the translations.yaml IList + array_template mappings.
//
// Phased drops:
//   * ChangeAfterResize(Matrix) — Matrix deferred
//   * .ctor(Page&, Rect, System.Collections.IList) — non-generic
//     IList overload auto-drops via translations cascade
// =============================================================================

#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/cap_style.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class InkAnnotation : public MarkupAnnotation {
public:
    using Stroke = std::vector<Aspose::Pdf::Point>;
    using StrokeList = std::vector<Stroke>;

    InkAnnotation(Aspose::Pdf::Document& document,
                  StrokeList inkList);
    InkAnnotation(Aspose::Pdf::Page& page,
                  Aspose::Pdf::Rectangle rect,
                  StrokeList inkList);

    void Accept(AnnotationSelector& visitor) override;

    Annotations::CapStyle CapStyle() const noexcept;
    void CapStyle(Annotations::CapStyle value) noexcept;

    const StrokeList& InkList() const noexcept;
    void InkList(StrokeList value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    StrokeList ink_list_;
    Annotations::CapStyle cap_style_ = Annotations::CapStyle::Butt;
};

}  // namespace Aspose::Pdf::Annotations
