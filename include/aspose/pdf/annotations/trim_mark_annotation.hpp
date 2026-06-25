#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::TrimMarkAnnotation — trim-mark printer
// mark at a page corner (PDF §14.11.3.3). Mirrors canonical
// Aspose.Pdf.Annotations.TrimMarkAnnotation; extends
// CornerPrinterMarkAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/corner_printer_mark_annotation.hpp>
#include <aspose/pdf/annotations/printer_mark_corner_position.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class TrimMarkAnnotation : public CornerPrinterMarkAnnotation {
public:
    TrimMarkAnnotation(Aspose::Pdf::Page& page,
                       PrinterMarkCornerPosition position);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
