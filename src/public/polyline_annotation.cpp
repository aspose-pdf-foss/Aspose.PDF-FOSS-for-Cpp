#include <aspose/pdf/annotations/polyline_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

PolylineAnnotation::PolylineAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect,
        std::vector<Aspose::Pdf::Point> vertices)
    : page_(&page) {
    Rect(std::move(rect));
    Vertices(std::move(vertices));
}

void PolylineAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType PolylineAnnotation::TypeImpl() const noexcept {
    return AnnotationType::PolyLine;
}

}  // namespace Aspose::Pdf::Annotations
