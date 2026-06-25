#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::RegistrationMarkAnnotation — registration
// printer mark at a page side (PDF §14.11.3.3). Mirrors canonical
// Aspose.Pdf.Annotations.RegistrationMarkAnnotation; extends
// PrinterMarkAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/printer_mark_annotation.hpp>
#include <aspose/pdf/annotations/printer_mark_side_position.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class RegistrationMarkAnnotation : public PrinterMarkAnnotation {
public:
    RegistrationMarkAnnotation(Aspose::Pdf::Page& page,
                               PrinterMarkSidePosition position);

    void Accept(AnnotationSelector& visitor) override;

    PrinterMarkSidePosition Position() const noexcept;
    void Position(PrinterMarkSidePosition value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    PrinterMarkSidePosition position_ = PrinterMarkSidePosition::Top;
};

}  // namespace Aspose::Pdf::Annotations
