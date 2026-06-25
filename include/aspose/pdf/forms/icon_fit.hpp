#pragma once

// =============================================================================
// Aspose::Pdf::Forms::IconFit — icon-fitting parameters for a
// push-button widget annotation. PDF §12.5.6.19 /MK dict /IF
// entry. Mirrors canonical Aspose.Pdf.Forms.IconFit.
// =============================================================================

#include <string>

#include <aspose/pdf/forms/scaling_mode.hpp>
#include <aspose/pdf/forms/scaling_reason.hpp>

namespace Aspose::Pdf::Forms {

class IconFit {
public:
    IconFit() noexcept = default;

    // String <-> enum helpers (canonical /MK dict /IF /SW / /S
    // entries are PDF names; these helpers do the conversion).
    static Aspose::Pdf::Forms::ScalingReason NameToScalingReason(
        const std::string& reason);
    static std::string ScalingReasonToName(
        Aspose::Pdf::Forms::ScalingReason reason);
    static Aspose::Pdf::Forms::ScalingMode NameToScalingMode(
        const std::string& mode);
    static std::string ScalingModeToName(
        Aspose::Pdf::Forms::ScalingMode mode);

    // ---- Properties ----

    Aspose::Pdf::Forms::ScalingReason ScalingReason() const noexcept;
    void ScalingReason(Aspose::Pdf::Forms::ScalingReason value) noexcept;

    Aspose::Pdf::Forms::ScalingMode ScalingMode() const noexcept;
    void ScalingMode(Aspose::Pdf::Forms::ScalingMode value) noexcept;

    double LeftoverLeft() const noexcept;
    void LeftoverLeft(double value) noexcept;

    double LeftoverBottom() const noexcept;
    void LeftoverBottom(double value) noexcept;

    bool SpreadOnBorder() const noexcept;
    void SpreadOnBorder(bool value) noexcept;

private:
    Aspose::Pdf::Forms::ScalingReason scaling_reason_ =
        Aspose::Pdf::Forms::ScalingReason::Always;
    Aspose::Pdf::Forms::ScalingMode scaling_mode_ =
        Aspose::Pdf::Forms::ScalingMode::Proportional;
    double leftover_left_ = 0.5;
    double leftover_bottom_ = 0.5;
    bool spread_on_border_ = false;
};

}  // namespace Aspose::Pdf::Forms
