#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfFileSignature — digital-signature facade
// (sign / certify / verify / enumerate / remove signatures). Mirrors
// canonical Aspose.Pdf.Facades.PdfFileSignature; extends SaveableFacade.
//
// Sign / Certify perform real two-phase signing: a detached PKCS#7
// (`adbe.pkcs7.detached`) signature with a byte-exact /ByteRange and a
// SHA-256 message digest, built by foundation::pkcs7 (from-scratch RSA +
// ASN.1 DER + CMS over a bundled RSA-2048 signer). The enumerate / read /
// verify surface (GetSignNames, ContainsSignature, GetSignerName / GetReason
// / GetLocation / GetContactInfo, GetTotalRevision, CoversWholeDocument,
// VerifySignature) reads back the AcroForm /Sig fields.
//
// v1 follow-ons: PKCS#12 key extraction via SetCertificate (the bundled
// signer is used for now), the /DocMDP transform dict for Certify, and
// RemoveSignature* (no-op — needs a revision rewrite).
//
// Phased drops:
//   * Stream ctors / BindPdf(Stream) / Save(Stream) /
//     SignatureAppearanceStream / ExtractImage / ExtractCertificate
//     (Stream returns) — Stream cascade.
//   * Sign / Certify overloads taking System.Drawing.Rectangle —
//     Drawing.Rectangle cascade.
//   * GetDateTime (System.DateTime) — DateTime cascade.
//   * VerifySignature(…, ValidationOptions, ValidationResult) /
//     (…, X509Certificate2) and TryExtractCertificate(X509) /
//     GetSignaturesInfo (SignatureAlgorithmInfo) — crypto-type cascades.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/facades/signature_name.hpp>
#include <aspose/pdf/forms/doc_mdp_access_permissions.hpp>
#include <aspose/pdf/forms/doc_mdp_signature.hpp>
#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfFileSignature : public SaveableFacade {
public:
    PdfFileSignature() noexcept = default;
    explicit PdfFileSignature(const std::string& inputFile);
    PdfFileSignature(const std::string& inputFile,
                     const std::string& outputFile);
    explicit PdfFileSignature(Aspose::Pdf::Document& document);
    PdfFileSignature(Aspose::Pdf::Document& document,
                     const std::string& outputFile);

    using SaveableFacade::Save;
    void Save();

    // ---- Sign / certify ----

    void Sign(const std::string& sigName, const std::string& reason,
              const std::string& contact, const std::string& location,
              const Aspose::Pdf::Forms::Signature& signature);
    void Sign(const std::string& sigName,
              const Aspose::Pdf::Forms::Signature& signature);
    void Certify(const std::string& sigName,
                 const Aspose::Pdf::Forms::DocMDPSignature& signature);
    Aspose::Pdf::Forms::DocMDPAccessPermissions GetAccessPermissions();

    // ---- Enumerate ----

    std::vector<std::string> GetSignNames(bool onlyEmpty);
    std::vector<SignatureName> GetSignatureNames(bool onlyEmpty);
    std::vector<std::string> GetBlankSignNames();
    std::vector<SignatureName> GetBlankSignatureNames();

    bool IsContainSignature();
    bool ContainsSignature();
    bool ContainsUsageRights();

    bool IsCoversWholeDocument(const std::string& sigName);
    bool CoversWholeDocument(const std::string& sigName);
    bool CoversWholeDocument(const SignatureName& sigName);

    int GetRevision(const std::string& sigName);
    int GetRevision(const SignatureName& sigName);
    int GetTotalRevision();

    // ---- Remove (v1: no-op — needs revision rewrite) ----

    void RemoveUsageRights();
    void RemoveSignature(const std::string& sigName);
    void RemoveSignature(const SignatureName& sigName);
    void RemoveSignature(const std::string& sigName, bool keepFieldEmpty);
    void RemoveSignature(const SignatureName& sigName, bool keepFieldEmpty);
    void RemoveSignatures();

    // ---- Verify / read ----

    bool VerifySigned(const std::string& sigName);
    bool VerifySignature(const std::string& sigName);
    bool VerifySignature(const SignatureName& sigName);
    std::string GetSignerName(const std::string& sigName);
    std::string GetSignerName(const SignatureName& sigName);
    std::string GetReason(const std::string& sigName);
    std::string GetReason(const SignatureName& sigName);
    std::string GetLocation(const std::string& sigName);
    std::string GetLocation(const SignatureName& sigName);
    std::string GetContactInfo(const std::string& sigName);
    std::string GetContactInfo(const SignatureName& sigName);

    void SetCertificate(const std::string& pfxFile,
                        const std::string& password);

    // ---- Properties ----

    const std::string& SignatureAppearance() const;
    void SignatureAppearance(std::string value);
    bool IsLtvEnabled() const noexcept;
    bool IsCertified() const noexcept;

private:
    std::string output_file_;
    std::string signature_appearance_;
    std::string certificate_file_;
    std::string certificate_password_;
    bool is_certified_ = false;
};

}  // namespace Aspose::Pdf::Facades
