#pragma once

// =============================================================================
// Aspose::Pdf::PageCoordinateType — selects which page-level box
// (MediaBox vs CropBox) the renderer uses when sizing the output
// canvas.
//
// DLL surface (Aspose.PDF 26.4.0):
//   MediaBox = 0
//   CropBox  = 1
// =============================================================================

namespace Aspose::Pdf {

enum class PageCoordinateType : int {
    MediaBox = 0,
    CropBox = 1,
};

}
