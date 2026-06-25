#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::PolyIntent — intent classifier for
// PolygonAnnotation / PolylineAnnotation (PDF /IT entry per
// §12.5.6.9). Mirrors canonical Aspose.Pdf.Annotations.PolyIntent.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class PolyIntent : int {
    Undefined = 0,
    PolygonCloud = 1,
    PolyLineDimension = 2,
    PolygonDimension = 3,
};

}  // namespace Aspose::Pdf::Annotations
