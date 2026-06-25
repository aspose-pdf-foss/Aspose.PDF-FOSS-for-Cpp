// Bodies for the SVG-import load-options surface: LoadOptions, SvgLoadOptions.

#include <aspose/pdf/load_options.hpp>
#include <aspose/pdf/svg_load_options.hpp>

namespace Aspose::Pdf {

// ---- LoadOptions -----------------------------------------------------------

bool LoadOptions::DisableFontLicenseVerifications() const noexcept {
    return disable_font_license_verifications_;
}
void LoadOptions::DisableFontLicenseVerifications(bool value) noexcept {
    disable_font_license_verifications_ = value;
}

// ---- SvgLoadOptions --------------------------------------------------------

SvgLoadOptions::SvgLoadOptions() = default;

bool SvgLoadOptions::AdjustPageSize() const noexcept {
    return adjust_page_size_;
}
void SvgLoadOptions::AdjustPageSize(bool value) noexcept {
    adjust_page_size_ = value;
}

}  // namespace Aspose::Pdf
