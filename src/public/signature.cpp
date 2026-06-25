#include <aspose/pdf/forms/signature.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

Signature::Signature(const std::string& pfx,
                      const std::string& password)
    : pfx_path_(pfx),
      pfx_password_(password) {}

bool Signature::Verify() {
    // v1 stub — actual PKCS verification lands with F8
    // AcroForm save-through + a real crypto path.
    return true;
}

SignatureCustomAppearance&
Signature::CustomAppearance() noexcept { return custom_appearance_; }

const SignatureCustomAppearance&
Signature::CustomAppearance() const noexcept { return custom_appearance_; }

void Signature::CustomAppearance(SignatureCustomAppearance v) {
    custom_appearance_ = std::move(v);
}

const std::string& Signature::Authority() const noexcept {
    return authority_;
}
void Signature::Authority(std::string v) { authority_ = std::move(v); }

const std::string& Signature::Location() const noexcept {
    return location_;
}
void Signature::Location(std::string v) { location_ = std::move(v); }

const std::string& Signature::Reason() const noexcept { return reason_; }
void Signature::Reason(std::string v) { reason_ = std::move(v); }

const std::string& Signature::ContactInfo() const noexcept {
    return contact_info_;
}
void Signature::ContactInfo(std::string v) { contact_info_ = std::move(v); }

const std::vector<int>& Signature::ByteRange() const noexcept {
    return byte_range_;
}

bool Signature::UseLtv() const noexcept { return use_ltv_; }
void Signature::UseLtv(bool v) noexcept { use_ltv_ = v; }

bool Signature::AvoidEstimatingSignatureLength() const noexcept {
    return avoid_estimating_signature_length_;
}
void Signature::AvoidEstimatingSignatureLength(bool v) noexcept {
    avoid_estimating_signature_length_ = v;
}

int Signature::DefaultSignatureLength() const noexcept {
    return default_signature_length_;
}
void Signature::DefaultSignatureLength(int v) noexcept {
    default_signature_length_ = v;
}

bool Signature::ShowProperties() const noexcept { return show_properties_; }
void Signature::ShowProperties(bool v) noexcept { show_properties_ = v; }

}  // namespace Aspose::Pdf::Forms
