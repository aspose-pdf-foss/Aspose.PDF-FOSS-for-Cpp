#include <aspose/pdf/forms/icon_fit.hpp>

#include <stdexcept>
#include <string>

namespace Aspose::Pdf::Forms {

Aspose::Pdf::Forms::ScalingReason
IconFit::NameToScalingReason(const std::string& reason) {
    if (reason == "A") return Aspose::Pdf::Forms::ScalingReason::Always;
    if (reason == "B") return Aspose::Pdf::Forms::ScalingReason::IconIsBigger;
    if (reason == "S") return Aspose::Pdf::Forms::ScalingReason::IconIsSmaller;
    if (reason == "N") return Aspose::Pdf::Forms::ScalingReason::Never;
    throw std::out_of_range(
        "Aspose::Pdf::Forms::IconFit::NameToScalingReason: '" +
        reason + "' is not a /SW name (expected A / B / S / N)");
}

std::string IconFit::ScalingReasonToName(
        Aspose::Pdf::Forms::ScalingReason reason) {
    switch (reason) {
        case Aspose::Pdf::Forms::ScalingReason::Always:        return "A";
        case Aspose::Pdf::Forms::ScalingReason::IconIsBigger:  return "B";
        case Aspose::Pdf::Forms::ScalingReason::IconIsSmaller: return "S";
        case Aspose::Pdf::Forms::ScalingReason::Never:         return "N";
    }
    return "A";
}

Aspose::Pdf::Forms::ScalingMode
IconFit::NameToScalingMode(const std::string& mode) {
    if (mode == "P") return Aspose::Pdf::Forms::ScalingMode::Proportional;
    if (mode == "A") return Aspose::Pdf::Forms::ScalingMode::Anamorphic;
    throw std::out_of_range(
        "Aspose::Pdf::Forms::IconFit::NameToScalingMode: '" +
        mode + "' is not a /S name (expected P / A)");
}

std::string IconFit::ScalingModeToName(
        Aspose::Pdf::Forms::ScalingMode mode) {
    switch (mode) {
        case Aspose::Pdf::Forms::ScalingMode::Proportional: return "P";
        case Aspose::Pdf::Forms::ScalingMode::Anamorphic:   return "A";
    }
    return "P";
}

Aspose::Pdf::Forms::ScalingReason
IconFit::ScalingReason() const noexcept { return scaling_reason_; }
void IconFit::ScalingReason(
        Aspose::Pdf::Forms::ScalingReason v) noexcept {
    scaling_reason_ = v;
}

Aspose::Pdf::Forms::ScalingMode
IconFit::ScalingMode() const noexcept { return scaling_mode_; }
void IconFit::ScalingMode(
        Aspose::Pdf::Forms::ScalingMode v) noexcept {
    scaling_mode_ = v;
}

double IconFit::LeftoverLeft() const noexcept { return leftover_left_; }
void IconFit::LeftoverLeft(double v) noexcept { leftover_left_ = v; }

double IconFit::LeftoverBottom() const noexcept {
    return leftover_bottom_;
}
void IconFit::LeftoverBottom(double v) noexcept {
    leftover_bottom_ = v;
}

bool IconFit::SpreadOnBorder() const noexcept {
    return spread_on_border_;
}
void IconFit::SpreadOnBorder(bool v) noexcept { spread_on_border_ = v; }

}  // namespace Aspose::Pdf::Forms
