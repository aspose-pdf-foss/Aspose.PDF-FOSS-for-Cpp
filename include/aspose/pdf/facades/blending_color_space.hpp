#pragma once

// =============================================================================
// Aspose::Pdf::Facades::BlendingColorSpace — color space used by
// transparency-group rendering (PDF §11.6.5 /CS). Mirrors
// canonical Aspose.Pdf.Facades.BlendingColorSpace.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class BlendingColorSpace : int {
    DontChange = 0,
    Auto = 1,
    DeviceRGB = 2,
    DeviceCMYK = 3,
};

}  // namespace Aspose::Pdf::Facades
