#pragma once

// =============================================================================
// Aspose::Pdf::Rotation — 90-degree-quantized rotation selector
// consumed by Page rotation + Rectangle::Rotate.
//
// Canonical Aspose.Pdf.Rotation (Aspose.PDF 26.4.0): enum with
// `None` and three `on<deg>` members at 90-degree increments.
// =============================================================================

namespace Aspose::Pdf {

enum class Rotation : int {
    None = 0,
    on90 = 1,
    on180 = 2,
    on270 = 3,
};

}  // namespace Aspose::Pdf
