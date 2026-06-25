#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PropertyFlag — facade-level field-flag
// selector (FormEditor.SetFieldAttribute). Mirrors canonical
// Aspose.Pdf.Facades.PropertyFlag.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class PropertyFlag : int {
    ReadOnly = 0,
    Required = 1,
    NoExport = 2,
    InvalidFlag = 3,
};

}  // namespace Aspose::Pdf::Facades
