#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PolylineAnnotation — open-vertex
// path (PDF /Subtype /PolyLine per §12.5.6.9). Mirrors canonical
// Aspose.Pdf.Annotations.PolylineAnnotation; extends PolyAnnotation.
// =============================================================================

#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/poly_annotation.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class PolylineAnnotation : public PolyAnnotation {
public:
    PolylineAnnotation(Aspose::Pdf::Page& page,
                       Aspose::Pdf::Rectangle rect,
                       std::vector<Aspose::Pdf::Point> vertices);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
