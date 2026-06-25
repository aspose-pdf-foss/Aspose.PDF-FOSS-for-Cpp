#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::ColorBarAnnotation — color-bar
// printer-mark annotation (PDF §14.11.3.3 ColorBar). Mirrors
// canonical Aspose.Pdf.Annotations.ColorBarAnnotation; extends
// PrinterMarkAnnotation.
//
// Phased drops:
//   * ChangeAfterResize(Matrix) — Matrix deferred
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/colors_of_cmyk.hpp>
#include <aspose/pdf/annotations/printer_mark_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class ColorBarAnnotation : public PrinterMarkAnnotation {
public:
    ColorBarAnnotation(Aspose::Pdf::Page& page,
                       Aspose::Pdf::Rectangle rect,
                       ColorsOfCMYK colorOfCMYK);

    void Accept(AnnotationSelector& visitor) override;

    ColorsOfCMYK ColorOfCMYK() const noexcept;
    void ColorOfCMYK(ColorsOfCMYK value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    ColorsOfCMYK color_of_cmyk_ = ColorsOfCMYK::Cyan;
};

}  // namespace Aspose::Pdf::Annotations
