#include <aspose/pdf/annotations/polygon_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

PolygonAnnotation::PolygonAnnotation(
        Aspose::Pdf::Document& document,
        std::vector<Aspose::Pdf::Point> vertices) {
    (void)document;
    Vertices(std::move(vertices));
}

PolygonAnnotation::PolygonAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect,
        std::vector<Aspose::Pdf::Point> vertices)
    : page_(&page) {
    Rect(std::move(rect));
    Vertices(std::move(vertices));
}

void PolygonAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

AnnotationType PolygonAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Polygon;
}

}  // namespace Aspose::Pdf::Annotations
