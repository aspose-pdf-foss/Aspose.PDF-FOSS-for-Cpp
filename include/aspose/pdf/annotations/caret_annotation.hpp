#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::CaretAnnotation — text-insertion
// caret annotation (PDF /Subtype /Caret per §12.5.6.11). Mirrors
// canonical Aspose.Pdf.Annotations.CaretAnnotation; extends
// MarkupAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/caret_symbol.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class CaretAnnotation : public MarkupAnnotation {
public:
    explicit CaretAnnotation(Aspose::Pdf::Document& document);
    CaretAnnotation(Aspose::Pdf::Page& page,
                    Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    const Aspose::Pdf::Rectangle& Frame() const noexcept;
    void Frame(Aspose::Pdf::Rectangle value) noexcept;

    CaretSymbol Symbol() const noexcept;
    void Symbol(CaretSymbol value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    Aspose::Pdf::Rectangle frame_ = Aspose::Pdf::Rectangle::Trivial();
    CaretSymbol symbol_ = CaretSymbol::None;
};

}  // namespace Aspose::Pdf::Annotations
