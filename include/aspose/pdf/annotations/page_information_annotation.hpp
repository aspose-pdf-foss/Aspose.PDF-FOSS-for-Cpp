#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PageInformationAnnotation —
// page-information printer mark (PDF §14.11.3.3). Mirrors canonical
// Aspose.Pdf.Annotations.PageInformationAnnotation; extends
// PrinterMarkAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/printer_mark_annotation.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class PageInformationAnnotation : public PrinterMarkAnnotation {
public:
    PageInformationAnnotation(Aspose::Pdf::Page& page,
                              Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Annotations
