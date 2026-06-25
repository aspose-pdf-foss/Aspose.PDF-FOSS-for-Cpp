#include <aspose/pdf/annotations/corner_printer_mark_annotation.hpp>

namespace Aspose::Pdf::Annotations {

PrinterMarkCornerPosition
CornerPrinterMarkAnnotation::Position() const noexcept {
    return position_;
}
void CornerPrinterMarkAnnotation::Position(
        PrinterMarkCornerPosition v) noexcept {
    position_ = v;
}

}  // namespace Aspose::Pdf::Annotations
