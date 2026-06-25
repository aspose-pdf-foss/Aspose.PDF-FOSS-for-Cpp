#pragma once

// =============================================================================
// Aspose::Pdf::Devices::CompressionType — TIFF compression scheme
// selector (consumed by TiffSettings).
//
// DLL surface (Aspose.PDF 26.4.0) — note value ordering: None is 4,
// not 0. The default is LZW (value 0).
//   LZW    = 0
//   CCITT4 = 1
//   CCITT3 = 2
//   RLE    = 3
//   None   = 4
// =============================================================================

namespace Aspose::Pdf::Devices {

enum class CompressionType : int {
    LZW = 0,
    CCITT4 = 1,
    CCITT3 = 2,
    RLE = 3,
    None = 4,
};

}
