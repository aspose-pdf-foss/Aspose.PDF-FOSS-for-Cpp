#pragma once

// =============================================================================
// Aspose::Pdf::Facades::SignatureName — value type identifying a
// signature field by its partial Name + fully-qualified FullName, with a
// HasSignature flag (signed vs blank). Mirrors canonical
// Aspose.Pdf.Facades.SignatureName.
//
// Phased drops:
//   * Equals(System.Object) — Object comparator non-idiomatic in C++.
// =============================================================================

#include <string>

namespace Aspose::Pdf::Facades {

class SignatureName {
public:
    SignatureName() = default;

    // Canonical public fields.
    std::string Name;
    std::string FullName;

    std::string ToString() const;
    int GetHashCode() const;

    // True iff this name refers to a signed field (vs a blank
    // signature field). Populated by the signature enumeration on
    // PdfFileSignature.
    bool HasSignature() const noexcept;

private:
    friend class PdfFileSignature;
    bool has_signature_ = false;
};

}  // namespace Aspose::Pdf::Facades
