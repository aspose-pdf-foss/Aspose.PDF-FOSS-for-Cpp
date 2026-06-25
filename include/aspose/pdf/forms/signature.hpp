#pragma once

// =============================================================================
// Aspose::Pdf::Forms::Signature — abstract base for the digital-
// signature class hierarchy (PDF §12.8 — digital signatures).
// Mirrors canonical Aspose.Pdf.Forms.Signature; extends
// System.Object.
//
// Concrete subclasses (F6 this beat): PKCS1, PKCS7, PKCS7Detached,
// ExternalSignature.
//
// Phased drops (this beat):
//   * .ctor(System.IO.Stream pfx, string password) — Stream
//   * GetSignatureAlgorithmInfo — SignatureAlgorithmInfo cascade
//   * Verify (2 of 3 overloads — ValidationOptions /
//     ValidationResult / X509Certificate2 cascades)
//   * Date (System.DateTime) — DateTime cascade
//   * TimestampSettings + OcspSettings — class cascades
//   * CustomSignHash — SignHash delegate cascade
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/forms/signature_custom_appearance.hpp>

namespace Aspose::Pdf::Forms {

class Signature {
public:
    Signature() noexcept = default;
    Signature(const std::string& pfx, const std::string& password);
    virtual ~Signature() = default;

    // Verify(no-arg) ships per canonical; the (ValidationOptions,
    // ValidationResult) overloads drop via translations cascade.
    bool Verify();

    SignatureCustomAppearance& CustomAppearance() noexcept;
    const SignatureCustomAppearance& CustomAppearance() const noexcept;
    void CustomAppearance(SignatureCustomAppearance value);

    const std::string& Authority() const noexcept;
    void Authority(std::string value);

    const std::string& Location() const noexcept;
    void Location(std::string value);

    const std::string& Reason() const noexcept;
    void Reason(std::string value);

    const std::string& ContactInfo() const noexcept;
    void ContactInfo(std::string value);

    const std::vector<int>& ByteRange() const noexcept;

    bool UseLtv() const noexcept;
    void UseLtv(bool value) noexcept;

    bool AvoidEstimatingSignatureLength() const noexcept;
    void AvoidEstimatingSignatureLength(bool value) noexcept;

    int DefaultSignatureLength() const noexcept;
    void DefaultSignatureLength(int value) noexcept;

    bool ShowProperties() const noexcept;
    void ShowProperties(bool value) noexcept;

protected:
    std::string pfx_path_;
    std::string pfx_password_;

private:
    SignatureCustomAppearance custom_appearance_;
    std::string authority_;
    std::string location_;
    std::string reason_;
    std::string contact_info_;
    std::vector<int> byte_range_;
    bool use_ltv_ = false;
    bool avoid_estimating_signature_length_ = false;
    int default_signature_length_ = 8192;
    bool show_properties_ = false;
};

}  // namespace Aspose::Pdf::Forms
