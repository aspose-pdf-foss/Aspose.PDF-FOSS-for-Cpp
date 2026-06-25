#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::TextAnnotation — sticky-note
// annotation. Mirrors canonical Aspose.Pdf.Annotations.
// TextAnnotation (Aspose.PDF 26.4.0); extends MarkupAnnotation.
//
// Phased drops:
//   * ChangeAfterResize(Matrix) — Matrix deferred
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/text_icon.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class TextAnnotation : public MarkupAnnotation {
public:
    explicit TextAnnotation(Aspose::Pdf::Document& document);
    TextAnnotation(Aspose::Pdf::Page& page,
                   Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    bool Open() const noexcept;
    void Open(bool value) noexcept;

    TextIcon Icon() const noexcept;
    void Icon(TextIcon value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    bool open_ = false;
    TextIcon icon_ = TextIcon::Note;
};

}  // namespace Aspose::Pdf::Annotations
