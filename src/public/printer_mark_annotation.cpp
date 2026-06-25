#include <aspose/pdf/annotations/printer_mark_annotation.hpp>

namespace Aspose::Pdf::Annotations {

void PrinterMarkAnnotation::AddPrinterMarks(
        Aspose::Pdf::Document& /*document*/,
        PrinterMarksKind /*marksKind*/) {
    // v1 emits no printer-mark dictionaries — wiring through the
    // Document/Page edit-flow is deferred. Call is a no-op so
    // callers can integrate ahead of the real impl.
}

void PrinterMarkAnnotation::AddPrinterMarks(
        Aspose::Pdf::Page& /*page*/,
        PrinterMarksKind /*marksKind*/) {
    // See ctor-Document overload — same v1 deferral.
}

}  // namespace Aspose::Pdf::Annotations
