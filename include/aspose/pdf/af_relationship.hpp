#pragma once

// =============================================================================
// Aspose::Pdf::AFRelationship — relationship classification for
// an associated file (PDF 2.0 §14.13 /AFRelationship key). Mirrors
// canonical Aspose.Pdf.AFRelationship.
// =============================================================================

namespace Aspose::Pdf {

enum class AFRelationship : int {
    Source = 0,
    Data = 1,
    Alternative = 2,
    Supplement = 3,
    Unspecified = 4,
    EncryptedPayload = 5,
    None = 6,
};

}  // namespace Aspose::Pdf
