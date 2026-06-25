#pragma once

// =============================================================================
// Aspose::Pdf::Forms::SignatureCustomAppearance — visual
// appearance overrides for a signature widget (PDF §12.8.4 — sig
// appearance). Mirrors canonical
// Aspose.Pdf.Forms.SignatureCustomAppearance.
//
// 19 read-write properties — colours, fonts, label strings, the
// signed-on date format, and the rotation. All defaults match
// canonical Aspose.PDF 26.4.0.
//
// Phased drops on this beat:
//   * Culture (System.Globalization.CultureInfo) — CultureInfo
//     cascade
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/forms/subject_name_elements.hpp>
#include <aspose/pdf/rotation.hpp>

namespace Aspose::Pdf::Forms {

class SignatureCustomAppearance {
public:
    SignatureCustomAppearance() noexcept = default;

    bool IsForegroundImage() const noexcept;
    void IsForegroundImage(bool value) noexcept;

    const std::string& FontFamilyName() const noexcept;
    void FontFamilyName(std::string value);

    double FontSize() const noexcept;
    void FontSize(double value) noexcept;

    const Aspose::Pdf::Color& ForegroundColor() const noexcept;
    void ForegroundColor(Aspose::Pdf::Color value) noexcept;

    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(Aspose::Pdf::Color value) noexcept;

    bool ShowContactInfo() const noexcept;
    void ShowContactInfo(bool value) noexcept;

    bool ShowReason() const noexcept;
    void ShowReason(bool value) noexcept;

    bool ShowLocation() const noexcept;
    void ShowLocation(bool value) noexcept;

    const std::string& ContactInfoLabel() const noexcept;
    void ContactInfoLabel(std::string value);

    const std::string& ReasonLabel() const noexcept;
    void ReasonLabel(std::string value);

    const std::string& LocationLabel() const noexcept;
    void LocationLabel(std::string value);

    const std::string& DigitalSignedLabel() const noexcept;
    void DigitalSignedLabel(std::string value);

    bool UseDigitalSubjectFormat() const noexcept;
    void UseDigitalSubjectFormat(bool value) noexcept;

    const std::vector<SubjectNameElements>&
        DigitalSubjectFormat() const noexcept;
    void DigitalSubjectFormat(std::vector<SubjectNameElements> value);

    const std::string& DateSignedAtLabel() const noexcept;
    void DateSignedAtLabel(std::string value);

    const std::string& DateTimeLocalFormat() const noexcept;
    void DateTimeLocalFormat(std::string value);

    const std::string& DateTimeFormat() const noexcept;
    void DateTimeFormat(std::string value);

    Aspose::Pdf::Rotation Rotation() const noexcept;
    void Rotation(Aspose::Pdf::Rotation value) noexcept;

private:
    bool is_foreground_image_ = false;
    std::string font_family_name_;
    double font_size_ = 10.0;
    Aspose::Pdf::Color foreground_color_ = Aspose::Pdf::Color::Black();
    Aspose::Pdf::Color background_color_ = Aspose::Pdf::Color::White();
    bool show_contact_info_ = false;
    bool show_reason_ = true;
    bool show_location_ = true;
    std::string contact_info_label_ = "Contact:";
    std::string reason_label_ = "Reason:";
    std::string location_label_ = "Location:";
    std::string digital_signed_label_ = "Digitally signed by";
    bool use_digital_subject_format_ = false;
    std::vector<SubjectNameElements> digital_subject_format_;
    std::string date_signed_at_label_ = "Date:";
    std::string date_time_local_format_;
    std::string date_time_format_;
    Aspose::Pdf::Rotation rotation_ = Aspose::Pdf::Rotation::None;
};

}  // namespace Aspose::Pdf::Forms
