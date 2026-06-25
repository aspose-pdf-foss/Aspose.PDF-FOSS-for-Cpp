#pragma once

// =============================================================================
// Aspose::Pdf::PasswordType — open/edit password role classification
// when a PDF was opened with a password (PDF §7.6.4 — user vs
// owner password). Mirrors canonical Aspose.Pdf.PasswordType.
// =============================================================================

namespace Aspose::Pdf {

enum class PasswordType : int {
    None = 0,
    User = 1,
    Owner = 2,
    Inaccessible = 3,
};

}  // namespace Aspose::Pdf
