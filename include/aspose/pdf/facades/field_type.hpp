#pragma once

// =============================================================================
// Aspose::Pdf::Facades::FieldType — facade-level field-type
// selector (parallels the Forms::Field subclass hierarchy with
// a flat enum view). Mirrors canonical Aspose.Pdf.Facades.FieldType.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class FieldType : int {
    Text = 0,
    ComboBox = 1,
    ListBox = 2,
    Radio = 3,
    CheckBox = 4,
    PushButton = 5,
    MultiLineText = 6,
    Barcode = 7,
    InvalidNameOrType = 8,
    Signature = 9,
    Image = 10,
    Numeric = 11,
    DateTime = 12,
};

}  // namespace Aspose::Pdf::Facades
