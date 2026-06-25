#pragma once

// =============================================================================
// Aspose::Pdf::Hyperlink — abstract base for the canonical
// Aspose.Pdf hyperlink hierarchy (WebHyperlink + LocalHyperlink
// are the concrete subclasses, deferred to a follow-up beat).
//
// Canonical Aspose.Pdf.Hyperlink (Aspose.PDF 26.4.0) exposes no
// public members — it's purely a base-type marker that
// BaseParagraph.Hyperlink can hold. Polymorphic destruction
// requires a virtual destructor in C++.
// =============================================================================

namespace Aspose::Pdf {

class Hyperlink {
public:
    Hyperlink() noexcept = default;
    virtual ~Hyperlink() = default;

    Hyperlink(const Hyperlink&) = default;
    Hyperlink& operator=(const Hyperlink&) = default;
    Hyperlink(Hyperlink&&) noexcept = default;
    Hyperlink& operator=(Hyperlink&&) noexcept = default;
};

}  // namespace Aspose::Pdf
