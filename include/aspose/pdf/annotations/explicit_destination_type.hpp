#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::ExplicitDestinationType — the fit mode of an
// explicit (in-document) navigation destination (PDF /D array verb per
// §12.3.2.2). Mirrors canonical Aspose.Pdf.Annotations.ExplicitDestinationType
// (Aspose.PDF 26.4.0).
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class ExplicitDestinationType : int {
    XYZ = 0,
    Fit = 1,
    FitH = 2,
    FitV = 3,
    FitR = 4,
    FitB = 5,
    FitBH = 6,
    FitBV = 7,
};

}  // namespace Aspose::Pdf::Annotations
