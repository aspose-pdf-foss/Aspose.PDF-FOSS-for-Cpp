#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::CommonFigureAnnotation — abstract
// intermediate base for shape-like annotations (Square, Circle).
// Mirrors canonical Aspose.Pdf.Annotations.CommonFigureAnnotation;
// extends MarkupAnnotation.
//
// Adds InteriorColor (fill) and Frame (the visible rectangle —
// distinct from the bounding Rect) on top of MarkupAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Annotations {

class CommonFigureAnnotation : public MarkupAnnotation {
public:
    CommonFigureAnnotation() noexcept = default;
    explicit CommonFigureAnnotation(Aspose::Pdf::Document& document);

    Aspose::Pdf::Color InteriorColor() const noexcept;
    void InteriorColor(Aspose::Pdf::Color value) noexcept;

    const Aspose::Pdf::Rectangle& Frame() const noexcept;
    void Frame(Aspose::Pdf::Rectangle value) noexcept;

private:
    Aspose::Pdf::Color interior_color_;
    Aspose::Pdf::Rectangle frame_ = Aspose::Pdf::Rectangle::Trivial();
};

}  // namespace Aspose::Pdf::Annotations
