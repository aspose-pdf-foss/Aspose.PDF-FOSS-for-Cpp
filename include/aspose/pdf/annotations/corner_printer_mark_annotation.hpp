#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::CornerPrinterMarkAnnotation — abstract
// intermediate base for corner-positioned printer marks (TrimMark,
// BleedMark). Mirrors canonical
// Aspose.Pdf.Annotations.CornerPrinterMarkAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/printer_mark_annotation.hpp>
#include <aspose/pdf/annotations/printer_mark_corner_position.hpp>

namespace Aspose::Pdf::Annotations {

class CornerPrinterMarkAnnotation : public PrinterMarkAnnotation {
public:
    PrinterMarkCornerPosition Position() const noexcept;
    void Position(PrinterMarkCornerPosition value) noexcept;

protected:
    CornerPrinterMarkAnnotation() noexcept = default;

private:
    PrinterMarkCornerPosition position_ =
        PrinterMarkCornerPosition::TopLeft;
};

}  // namespace Aspose::Pdf::Annotations
