#include <aspose/pdf/annotations/annotation_selector.hpp>

#include <aspose/pdf/annotations/annotation.hpp>

namespace Aspose::Pdf::Annotations {

AnnotationSelector::AnnotationSelector(Annotation& annotation) noexcept
    : seed_annotation_(&annotation) {}

// All Visit overloads default to no-ops — derived user selectors
// override the subset they care about. The concrete annotation
// classes (A6..A13) call `visitor.Visit(*this)` from their
// Accept(visitor) override to land here at runtime.

void AnnotationSelector::Visit(LinkAnnotation&)             { /* no-op */ }
void AnnotationSelector::Visit(FileAttachmentAnnotation&)   { /* no-op */ }
void AnnotationSelector::Visit(TextAnnotation&)             { /* no-op */ }
void AnnotationSelector::Visit(RedactionAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(FreeTextAnnotation&)         { /* no-op */ }
void AnnotationSelector::Visit(HighlightAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(UnderlineAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(StrikeOutAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(SquigglyAnnotation&)         { /* no-op */ }
void AnnotationSelector::Visit(PopupAnnotation&)            { /* no-op */ }
void AnnotationSelector::Visit(LineAnnotation&)             { /* no-op */ }
void AnnotationSelector::Visit(CircleAnnotation&)           { /* no-op */ }
void AnnotationSelector::Visit(SquareAnnotation&)           { /* no-op */ }
void AnnotationSelector::Visit(InkAnnotation&)              { /* no-op */ }
void AnnotationSelector::Visit(PolylineAnnotation&)         { /* no-op */ }
void AnnotationSelector::Visit(PolygonAnnotation&)          { /* no-op */ }
void AnnotationSelector::Visit(CaretAnnotation&)            { /* no-op */ }
void AnnotationSelector::Visit(StampAnnotation&)            { /* no-op */ }
void AnnotationSelector::Visit(WidgetAnnotation&)           { /* no-op */ }
void AnnotationSelector::Visit(WatermarkAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(MovieAnnotation&)            { /* no-op */ }
void AnnotationSelector::Visit(RichMediaAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(ScreenAnnotation&)           { /* no-op */ }
void AnnotationSelector::Visit(PDF3DAnnotation&)            { /* no-op */ }
void AnnotationSelector::Visit(ColorBarAnnotation&)         { /* no-op */ }
void AnnotationSelector::Visit(TrimMarkAnnotation&)         { /* no-op */ }
void AnnotationSelector::Visit(BleedMarkAnnotation&)        { /* no-op */ }
void AnnotationSelector::Visit(RegistrationMarkAnnotation&) { /* no-op */ }
void AnnotationSelector::Visit(PageInformationAnnotation&)  { /* no-op */ }

}  // namespace Aspose::Pdf::Annotations
