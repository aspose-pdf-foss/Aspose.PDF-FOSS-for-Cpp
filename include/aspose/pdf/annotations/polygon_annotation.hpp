#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PolygonAnnotation — closed-vertex
// shape (PDF /Subtype /Polygon per §12.5.6.9). Mirrors canonical
// Aspose.Pdf.Annotations.PolygonAnnotation; extends PolyAnnotation.
// =============================================================================

#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/poly_annotation.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class PolygonAnnotation : public PolyAnnotation {
public:
    PolygonAnnotation(Aspose::Pdf::Document& document,
                      std::vector<Aspose::Pdf::Point> vertices);
    PolygonAnnotation(Aspose::Pdf::Page& page,
                      Aspose::Pdf::Rectangle rect,
                      std::vector<Aspose::Pdf::Point> vertices);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
