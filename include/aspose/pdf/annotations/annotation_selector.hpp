#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::AnnotationSelector — visitor base
// for the annotation hierarchy. Mirrors canonical
// Aspose.Pdf.Annotations.AnnotationSelector (Aspose.PDF 26.4.0).
//
// Defines 30 Visit(T) overloads — one per concrete annotation
// subtype. Derived user selectors override the Visit methods
// they care about; the defaults are no-ops.
//
// Concrete annotation classes call `visitor.Visit(*this)` from
// their Accept(visitor) override (the double-dispatch idiom).
// Beats A6..A13 ship the concrete subclasses; the visitor stays
// stable across that work.
//
// Phased drops on this beat (drops.yaml):
//   * Selected (IList<Annotation>) — abstract-element idiom
//     mismatch; defer until concrete-annotation cluster lands.
// =============================================================================

namespace Aspose::Pdf::Annotations {

class Annotation;

// Forward declarations for the 29 concrete annotation classes
// the visitor dispatches over. The classes themselves land in
// cluster beats A6..A13; ANY include of this header gets the
// visitor surface ready immediately so Accept(visitor)
// implementations on the concrete classes can compile against
// it without further plumbing.
class LinkAnnotation;
class FileAttachmentAnnotation;
class TextAnnotation;
class RedactionAnnotation;
class FreeTextAnnotation;
class HighlightAnnotation;
class UnderlineAnnotation;
class StrikeOutAnnotation;
class SquigglyAnnotation;
class PopupAnnotation;
class LineAnnotation;
class CircleAnnotation;
class SquareAnnotation;
class InkAnnotation;
class PolylineAnnotation;
class PolygonAnnotation;
class CaretAnnotation;
class StampAnnotation;
class WidgetAnnotation;
class WatermarkAnnotation;
class MovieAnnotation;
class RichMediaAnnotation;
class ScreenAnnotation;
class PDF3DAnnotation;
class ColorBarAnnotation;
class TrimMarkAnnotation;
class BleedMarkAnnotation;
class RegistrationMarkAnnotation;
class PageInformationAnnotation;

class AnnotationSelector {
public:
    AnnotationSelector() noexcept = default;

    // 1-arg ctor — canonical takes a single Annotation seed (the
    // visitor's Selected list starts with this annotation
    // included). Stored as a non-owning pointer; the Annotation
    // must outlive the selector.
    explicit AnnotationSelector(Annotation& annotation) noexcept;

    virtual ~AnnotationSelector() = default;

    AnnotationSelector(const AnnotationSelector&) = default;
    AnnotationSelector& operator=(const AnnotationSelector&) = default;
    AnnotationSelector(AnnotationSelector&&) noexcept = default;
    AnnotationSelector& operator=(AnnotationSelector&&) noexcept = default;

    // 30 Visit overloads — one per concrete annotation subtype.
    // Defaults are no-ops; derived user selectors override the
    // ones they care about.

    virtual void Visit(LinkAnnotation& link);
    virtual void Visit(FileAttachmentAnnotation& attachment);
    virtual void Visit(TextAnnotation& text);
    virtual void Visit(RedactionAnnotation& redact);
    virtual void Visit(FreeTextAnnotation& freetext);
    virtual void Visit(HighlightAnnotation& highlight);
    virtual void Visit(UnderlineAnnotation& underline);
    virtual void Visit(StrikeOutAnnotation& strikeOut);
    virtual void Visit(SquigglyAnnotation& squiggly);
    virtual void Visit(PopupAnnotation& popup);
    virtual void Visit(LineAnnotation& line);
    virtual void Visit(CircleAnnotation& circle);
    virtual void Visit(SquareAnnotation& square);
    virtual void Visit(InkAnnotation& ink);
    virtual void Visit(PolylineAnnotation& polyline);
    virtual void Visit(PolygonAnnotation& polygon);
    virtual void Visit(CaretAnnotation& caret);
    virtual void Visit(StampAnnotation& stamp);
    virtual void Visit(WidgetAnnotation& widget);
    virtual void Visit(WatermarkAnnotation& watermark);
    virtual void Visit(MovieAnnotation& movie);
    virtual void Visit(RichMediaAnnotation& richMedia);
    virtual void Visit(ScreenAnnotation& screen);
    virtual void Visit(PDF3DAnnotation& pdf3D);
    virtual void Visit(ColorBarAnnotation& colorBar);
    virtual void Visit(TrimMarkAnnotation& trimMark);
    virtual void Visit(BleedMarkAnnotation& bleedMark);
    virtual void Visit(RegistrationMarkAnnotation& registrationMark);
    virtual void Visit(PageInformationAnnotation& pageInformation);

protected:
    // Held by raw pointer so callers control lifetime. nullptr
    // when default-constructed. Derived selectors can introspect
    // this in their Visit overrides.
    Annotation* seed_annotation_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
