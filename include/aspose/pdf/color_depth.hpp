#pragma once

// =============================================================================
// Aspose::Pdf::Devices::ColorDepth — output colour depth selector for
// TIFF rendering (consumed by TiffSettings).
//
// DLL surface (Aspose.PDF 26.4.0):
//   Default     = 0
//   Format24bpp = 1
//   Format8bpp  = 2
//   Format4bpp  = 3
//   Format1bpp  = 4
// =============================================================================

namespace Aspose::Pdf::Devices {

enum class ColorDepth : int {
    Default = 0,
    Format24bpp = 1,
    Format8bpp = 2,
    Format4bpp = 3,
    Format1bpp = 4,
};

}
