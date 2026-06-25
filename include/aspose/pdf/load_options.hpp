#pragma once

// =============================================================================
// Aspose::Pdf::LoadOptions — abstract base for format-specific load options
// passed to Document(filename, options). Mirrors canonical Aspose.Pdf.
// LoadOptions (Aspose.PDF 26.4.0) as a strict subset: the
// DisableFontLicenseVerifications flag. The LoadFormat enum and the
// WarningHandler delegate are deferred. Instantiated only through a derived
// type (e.g. SvgLoadOptions).
// =============================================================================

namespace Aspose::Pdf {

class LoadOptions {
public:
    virtual ~LoadOptions() = default;

    // Canonical LoadOptions.DisableFontLicenseVerifications.
    bool DisableFontLicenseVerifications() const noexcept;
    void DisableFontLicenseVerifications(bool value) noexcept;

protected:
    LoadOptions() = default;

private:
    bool disable_font_license_verifications_ = false;
};

}  // namespace Aspose::Pdf
