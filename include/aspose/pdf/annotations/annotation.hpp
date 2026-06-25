#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::Annotation — abstract base of the
// annotation hierarchy. Mirrors canonical
// Aspose.Pdf.Annotations.Annotation (Aspose.PDF 26.4.0); extends
// Aspose::Pdf::BaseParagraph.
//
// Members dropped from this beat (drops.yaml cpp track):
//   * Accept(AnnotationSelector) — lands at beat A5
//   * ChangeAfterResize(Matrix) — Aspose.Pdf.Matrix deferred
//   * States + Appearance — AppearanceDictionary deferred
//   * Actions — PdfActionCollection deferred (PdfAction cascade)
//   * Modified (DateTime) — auto-drops via translations cascade
//
// Concrete annotation subclasses (TextAnnotation, LinkAnnotation,
// FreeTextAnnotation, etc.) land in cluster beats A6..A13.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/border.hpp>
#include <aspose/pdf/annotations/characteristics.hpp>
#include <aspose/pdf/annotations/text_alignment.hpp>
#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf::Annotations {

class AnnotationSelector;

class Annotation : public Aspose::Pdf::BaseParagraph {
public:
    // ---- Methods (3 shipped; Accept/ChangeAfterResize dropped) ----

    // Returns the annotation's Rect, optionally accounting for
    // page rotation.
    Aspose::Pdf::Rectangle GetRectangle(
            bool considerRotation) const noexcept;

    // Flatten this annotation into the page content stream
    // (the annotation is replaced by static graphics).
    void Flatten();

    // Visitor double-dispatch hook. Default impl is a no-op so
    // smoke-test stubs + the abstract base don't need to wire
    // through. Concrete subclasses (A6..A13) override this to
    // call visitor.Visit(*this) — the canonical visitor pattern.
    virtual void Accept(AnnotationSelector& visitor);

    // ---- Properties ----

    bool UpdateAppearanceOnConvert() const noexcept;
    void UpdateAppearanceOnConvert(bool value) noexcept;

    bool UseFontSubset() const noexcept;
    void UseFontSubset(bool value) noexcept;

    AnnotationFlags Flags() const noexcept;
    void Flags(AnnotationFlags value) noexcept;

    // Read-only — subclasses report their kind via this property
    // by overriding the private virtual TypeImpl() hook.
    Annotations::AnnotationType AnnotationType() const noexcept;

    double Width() const noexcept;
    void Width(double value) noexcept;

    double Height() const noexcept;
    void Height(double value) noexcept;

    const Aspose::Pdf::Rectangle& Rect() const noexcept;
    void Rect(Aspose::Pdf::Rectangle value) noexcept;

    const std::string& Contents() const noexcept;
    void Contents(std::string value) noexcept;

    const std::string& Name() const noexcept;
    void Name(std::string value) noexcept;

    Aspose::Pdf::Color Color() const noexcept;
    void Color(Aspose::Pdf::Color value) noexcept;

    Annotations::Border& Border() noexcept;
    const Annotations::Border& Border() const noexcept;
    void Border(Annotations::Border value) noexcept;

    const std::string& ActiveState() const noexcept;
    void ActiveState(std::string value) noexcept;

    const Annotations::Characteristics& Characteristics() const noexcept;

    TextAlignment Alignment() const noexcept;
    void Alignment(TextAlignment value) noexcept;

    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment v) noexcept;

    Aspose::Pdf::HorizontalAlignment TextHorizontalAlignment() const noexcept;
    void TextHorizontalAlignment(Aspose::Pdf::HorizontalAlignment v) noexcept;

    const std::string& FullName() const noexcept;
    int PageIndex() const noexcept;

    // Annotation is abstract via TypeImpl(); ctors are public
    // for derived-class brace-init convenience.
    Annotation() noexcept;
    explicit Annotation(Aspose::Pdf::Rectangle rect) noexcept;

protected:
    // Concrete annotation subclasses report their AnnotationType
    // by overriding this hook.
    virtual Annotations::AnnotationType TypeImpl() const noexcept = 0;

    // Subclasses can update FullName when they're attached to a
    // Page (so it reads "<PageIndex>_<Name>" or similar). v1
    // doesn't surface this binding yet; left protected for future
    // beats.
    void SetFullName(std::string value) noexcept;

    // Subclasses can set PageIndex when attached. v1 default is 0.
    void SetPageIndex(int value) noexcept;

private:
    AnnotationFlags flags_ = AnnotationFlags::Default;
    bool update_appearance_on_convert_ = false;
    bool use_font_subset_ = false;
    Aspose::Pdf::Rectangle rect_ = Aspose::Pdf::Rectangle::Trivial();
    std::string contents_;
    std::string name_;
    Aspose::Pdf::Color color_;
    Annotations::Border border_;
    Annotations::Characteristics characteristics_;
    std::string active_state_;
    TextAlignment alignment_ = TextAlignment::Left;
    Aspose::Pdf::HorizontalAlignment text_horizontal_alignment_
        = Aspose::Pdf::HorizontalAlignment::None;
    std::string full_name_;
    int page_index_ = 0;
};

}  // namespace Aspose::Pdf::Annotations
