#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PrinterMarkAnnotation — abstract base
// for printer-mark annotations (PDF /Subtype /PrinterMark per
// §14.11.3). Mirrors canonical
// Aspose.Pdf.Annotations.PrinterMarkAnnotation; extends Annotation.
//
// Canonical surface is a pair of static AddPrinterMarks helpers —
// the class itself has no public ctor (subclasses do).
// =============================================================================

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/printer_marks_kind.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class PrinterMarkAnnotation : public Annotation {
public:
    static void AddPrinterMarks(Aspose::Pdf::Document& document,
                                PrinterMarksKind marksKind);
    static void AddPrinterMarks(Aspose::Pdf::Page& page,
                                PrinterMarksKind marksKind);

protected:
    PrinterMarkAnnotation() noexcept = default;
};

}  // namespace Aspose::Pdf::Annotations
