#include <aspose/pdf/forms/signature_custom_appearance.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

bool SignatureCustomAppearance::IsForegroundImage() const noexcept {
    return is_foreground_image_;
}
void SignatureCustomAppearance::IsForegroundImage(bool v) noexcept {
    is_foreground_image_ = v;
}

const std::string&
SignatureCustomAppearance::FontFamilyName() const noexcept {
    return font_family_name_;
}
void SignatureCustomAppearance::FontFamilyName(std::string v) {
    font_family_name_ = std::move(v);
}

double SignatureCustomAppearance::FontSize() const noexcept {
    return font_size_;
}
void SignatureCustomAppearance::FontSize(double v) noexcept {
    font_size_ = v;
}

const Aspose::Pdf::Color&
SignatureCustomAppearance::ForegroundColor() const noexcept {
    return foreground_color_;
}
void SignatureCustomAppearance::ForegroundColor(
        Aspose::Pdf::Color v) noexcept {
    foreground_color_ = std::move(v);
}

const Aspose::Pdf::Color&
SignatureCustomAppearance::BackgroundColor() const noexcept {
    return background_color_;
}
void SignatureCustomAppearance::BackgroundColor(
        Aspose::Pdf::Color v) noexcept {
    background_color_ = std::move(v);
}

bool SignatureCustomAppearance::ShowContactInfo() const noexcept {
    return show_contact_info_;
}
void SignatureCustomAppearance::ShowContactInfo(bool v) noexcept {
    show_contact_info_ = v;
}

bool SignatureCustomAppearance::ShowReason() const noexcept {
    return show_reason_;
}
void SignatureCustomAppearance::ShowReason(bool v) noexcept {
    show_reason_ = v;
}

bool SignatureCustomAppearance::ShowLocation() const noexcept {
    return show_location_;
}
void SignatureCustomAppearance::ShowLocation(bool v) noexcept {
    show_location_ = v;
}

const std::string&
SignatureCustomAppearance::ContactInfoLabel() const noexcept {
    return contact_info_label_;
}
void SignatureCustomAppearance::ContactInfoLabel(std::string v) {
    contact_info_label_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::ReasonLabel() const noexcept {
    return reason_label_;
}
void SignatureCustomAppearance::ReasonLabel(std::string v) {
    reason_label_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::LocationLabel() const noexcept {
    return location_label_;
}
void SignatureCustomAppearance::LocationLabel(std::string v) {
    location_label_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::DigitalSignedLabel() const noexcept {
    return digital_signed_label_;
}
void SignatureCustomAppearance::DigitalSignedLabel(std::string v) {
    digital_signed_label_ = std::move(v);
}

bool SignatureCustomAppearance::UseDigitalSubjectFormat() const noexcept {
    return use_digital_subject_format_;
}
void SignatureCustomAppearance::UseDigitalSubjectFormat(bool v) noexcept {
    use_digital_subject_format_ = v;
}

const std::vector<SubjectNameElements>&
SignatureCustomAppearance::DigitalSubjectFormat() const noexcept {
    return digital_subject_format_;
}
void SignatureCustomAppearance::DigitalSubjectFormat(
        std::vector<SubjectNameElements> v) {
    digital_subject_format_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::DateSignedAtLabel() const noexcept {
    return date_signed_at_label_;
}
void SignatureCustomAppearance::DateSignedAtLabel(std::string v) {
    date_signed_at_label_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::DateTimeLocalFormat() const noexcept {
    return date_time_local_format_;
}
void SignatureCustomAppearance::DateTimeLocalFormat(std::string v) {
    date_time_local_format_ = std::move(v);
}

const std::string&
SignatureCustomAppearance::DateTimeFormat() const noexcept {
    return date_time_format_;
}
void SignatureCustomAppearance::DateTimeFormat(std::string v) {
    date_time_format_ = std::move(v);
}

Aspose::Pdf::Rotation
SignatureCustomAppearance::Rotation() const noexcept { return rotation_; }
void SignatureCustomAppearance::Rotation(
        Aspose::Pdf::Rotation v) noexcept {
    rotation_ = v;
}

}  // namespace Aspose::Pdf::Forms
