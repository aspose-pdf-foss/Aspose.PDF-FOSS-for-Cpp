#include <aspose/pdf/facades/pdf_file_signature.hpp>

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/signature.hpp>

#include "_pending.hpp"
#include "pkcs7_sign.hpp"

namespace Aspose::Pdf::Facades {

namespace {

// Build the two date strings the signing pipeline needs from the current
// UTC time: a PDF date ("D:YYYYMMDDHHmmSS+00'00'") for the /M entry and a
// 13-char UTCTime ("YYMMDDHHMMSSZ") for the PKCS#7 signingTime attribute.
void NowStrings(std::string& pdf_date, std::string& utc_time) {
    std::time_t t = std::time(nullptr);
    std::tm gm{};
#if defined(_WIN32)
    gmtime_s(&gm, &t);
#else
    gmtime_r(&t, &gm);
#endif
    char pdf[32];
    std::snprintf(pdf, sizeof(pdf), "D:%04d%02d%02d%02d%02d%02d+00'00'",
                  gm.tm_year + 1900, gm.tm_mon + 1, gm.tm_mday, gm.tm_hour,
                  gm.tm_min, gm.tm_sec);
    pdf_date = pdf;
    char utc[16];
    std::snprintf(utc, sizeof(utc), "%02d%02d%02d%02d%02d%02dZ",
                  (gm.tm_year + 1900) % 100, gm.tm_mon + 1, gm.tm_mday,
                  gm.tm_hour, gm.tm_min, gm.tm_sec);
    utc_time = utc;
}

}  // namespace

PdfFileSignature::PdfFileSignature(const std::string& inputFile) {
    BindPdf(inputFile);
}

PdfFileSignature::PdfFileSignature(const std::string& inputFile,
                                   const std::string& outputFile)
    : output_file_(outputFile) {
    BindPdf(inputFile);
}

PdfFileSignature::PdfFileSignature(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

PdfFileSignature::PdfFileSignature(Aspose::Pdf::Document& document,
                                   const std::string& outputFile)
    : output_file_(outputFile) {
    BindPdf(document);
}

void PdfFileSignature::Save() {
    if (document_ != nullptr && !output_file_.empty()) {
        document_->Save(output_file_);
    }
}

// ===== Sign / certify ========================================================

void PdfFileSignature::Sign(const std::string& sigName,
                            const std::string& reason,
                            const std::string& contact,
                            const std::string& location,
                            const Aspose::Pdf::Forms::Signature& signature) {
    if (document_ == nullptr) return;
    Aspose::Pdf::Document::SignatureRequest req;
    req.field_name = sigName.empty() ? "Signature1" : sigName;
    // The signer: the Signature's own .pfx, else SetCertificate's, else the
    // bundled test key. /Name is the certificate's common name.
    req.pfx_path = signature.pfx_path_.empty() ? certificate_file_ : signature.pfx_path_;
    req.pfx_password =
        signature.pfx_path_.empty() ? certificate_password_ : signature.pfx_password_;
    req.reason = reason;
    req.contact = contact;
    req.location = location;
    req.contents_capacity =
        static_cast<std::size_t>(signature.DefaultSignatureLength() > 0
                                     ? signature.DefaultSignatureLength()
                                     : 8192);
    NowStrings(req.date_pdf, req.signing_time_utc);
    document_->SignInternal(req);
}

void PdfFileSignature::Sign(const std::string& sigName,
                            const Aspose::Pdf::Forms::Signature& signature) {
    Sign(sigName, signature.Reason(), signature.ContactInfo(),
         signature.Location(), signature);
}

void PdfFileSignature::Certify(
        const std::string& sigName,
        const Aspose::Pdf::Forms::DocMDPSignature& signature) {
    if (document_ == nullptr) return;
    // A certifying signature: the same detached CMS, plus a DocMDP
    // /Reference transform and the catalog /Perms /DocMDP entry.
    Aspose::Pdf::Document::SignatureRequest req;
    req.field_name = sigName.empty() ? "Signature1" : sigName;
    const Aspose::Pdf::Forms::Signature* s = signature.signature_;
    if (s != nullptr && !s->pfx_path_.empty()) {
        req.pfx_path = s->pfx_path_;
        req.pfx_password = s->pfx_password_;
    } else {
        req.pfx_path = certificate_file_;
        req.pfx_password = certificate_password_;
    }
    if (s != nullptr) {
        req.reason = s->Reason();
        req.contact = s->ContactInfo();
        req.location = s->Location();
        if (s->DefaultSignatureLength() > 0)
            req.contents_capacity = static_cast<std::size_t>(s->DefaultSignatureLength());
    }
    req.docmdp_permissions = static_cast<int>(signature.AccessPermissions());
    NowStrings(req.date_pdf, req.signing_time_utc);
    document_->SignInternal(req);
    is_certified_ = true;
}

Aspose::Pdf::Forms::DocMDPAccessPermissions
PdfFileSignature::GetAccessPermissions() {
    if (document_ != nullptr) {
        const int p = document_->ReadCatalogSignatureStateInternal().docmdp_permissions;
        if (p >= 1 && p <= 3)
            return static_cast<Aspose::Pdf::Forms::DocMDPAccessPermissions>(p);
    }
    return Aspose::Pdf::Forms::DocMDPAccessPermissions::NoChanges;
}

// ===== Enumerate =============================================================

std::vector<std::string> PdfFileSignature::GetSignNames(bool onlyEmpty) {
    std::vector<std::string> out;
    if (document_ == nullptr) return out;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (onlyEmpty && s.has_value) continue;
        out.push_back(s.field_name);
    }
    return out;
}

std::vector<SignatureName> PdfFileSignature::GetSignatureNames(bool onlyEmpty) {
    std::vector<SignatureName> out;
    if (document_ == nullptr) return out;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (onlyEmpty && s.has_value) continue;
        SignatureName sn;
        sn.Name = s.field_name;
        sn.FullName = s.field_name;
        sn.has_signature_ = s.has_value;
        out.push_back(std::move(sn));
    }
    return out;
}

std::vector<std::string> PdfFileSignature::GetBlankSignNames() {
    return GetSignNames(true);
}
std::vector<SignatureName> PdfFileSignature::GetBlankSignatureNames() {
    return GetSignatureNames(true);
}

bool PdfFileSignature::IsContainSignature() { return ContainsSignature(); }

bool PdfFileSignature::ContainsSignature() {
    if (document_ == nullptr) return false;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.has_value) return true;
    }
    return false;
}

bool PdfFileSignature::ContainsUsageRights() {
    return document_ != nullptr &&
           document_->ReadCatalogSignatureStateInternal().usage_rights;
}

bool PdfFileSignature::IsCoversWholeDocument(const std::string& sigName) {
    return CoversWholeDocument(sigName);
}

bool PdfFileSignature::CoversWholeDocument(const std::string& sigName) {
    if (document_ == nullptr) return false;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.covers_whole;
    }
    return false;
}

bool PdfFileSignature::CoversWholeDocument(const SignatureName& sigName) {
    return CoversWholeDocument(sigName.Name);
}

int PdfFileSignature::GetRevision(const std::string& sigName) {
    if (document_ == nullptr) return 0;
    int rev = 0;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.has_value) ++rev;
        if (s.field_name == sigName) return rev;
    }
    return 0;
}
int PdfFileSignature::GetRevision(const SignatureName& sigName) {
    return GetRevision(sigName.Name);
}

int PdfFileSignature::GetTotalRevision() {
    if (document_ == nullptr) return 0;
    int rev = 0;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.has_value) ++rev;
    }
    return rev;
}

// ===== Remove ================================================================
// Incremental updates: the signature value goes; the field stays (empty)
// only with keepFieldEmpty.

void PdfFileSignature::RemoveUsageRights() {
    if (document_ != nullptr) document_->RemoveUsageRightsInternal();
}
void PdfFileSignature::RemoveSignature(const std::string& sigName) {
    RemoveSignature(sigName, false);
}
void PdfFileSignature::RemoveSignature(const SignatureName& sigName) {
    RemoveSignature(sigName.Name, false);
}
void PdfFileSignature::RemoveSignature(const std::string& sigName,
                                       bool keepFieldEmpty) {
    if (document_ == nullptr || sigName.empty()) return;
    document_->RemoveSignatureInternal(sigName, keepFieldEmpty);
}
void PdfFileSignature::RemoveSignature(const SignatureName& sigName,
                                       bool keepFieldEmpty) {
    RemoveSignature(sigName.Name, keepFieldEmpty);
}
void PdfFileSignature::RemoveSignatures() {
    if (document_ != nullptr) document_->RemoveSignatureInternal({}, false);
}

// ===== Verify / read =========================================================

bool PdfFileSignature::VerifySigned(const std::string& sigName) {
    return VerifySignature(sigName);
}

bool PdfFileSignature::VerifySignature(const std::string& sigName) {
    if (document_ == nullptr) return false;
    return document_->VerifySignatureInternal(sigName).valid;
}
bool PdfFileSignature::VerifySignature(const SignatureName& sigName) {
    return VerifySignature(sigName.Name);
}

std::string PdfFileSignature::GetSignerName(const std::string& sigName) {
    if (document_ == nullptr) return {};
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.name;
    }
    return {};
}
std::string PdfFileSignature::GetSignerName(const SignatureName& sigName) {
    return GetSignerName(sigName.Name);
}

std::string PdfFileSignature::GetReason(const std::string& sigName) {
    if (document_ == nullptr) return {};
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.reason;
    }
    return {};
}
std::string PdfFileSignature::GetReason(const SignatureName& sigName) {
    return GetReason(sigName.Name);
}

std::string PdfFileSignature::GetLocation(const std::string& sigName) {
    if (document_ == nullptr) return {};
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.location;
    }
    return {};
}
std::string PdfFileSignature::GetLocation(const SignatureName& sigName) {
    return GetLocation(sigName.Name);
}

std::string PdfFileSignature::GetContactInfo(const std::string& sigName) {
    if (document_ == nullptr) return {};
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.contact;
    }
    return {};
}
std::string PdfFileSignature::GetContactInfo(const SignatureName& sigName) {
    return GetContactInfo(sigName.Name);
}

void PdfFileSignature::SetCertificate(const std::string& pfxFile,
                                      const std::string& password) {
    // Used by Sign / Certify when their Signature carries no .pfx of its
    // own; the file is opened (and the password checked) when signing.
    certificate_file_ = pfxFile;
    certificate_password_ = password;
}

// ===== Properties ============================================================

const std::string& PdfFileSignature::SignatureAppearance() const {
    return signature_appearance_;
}
void PdfFileSignature::SignatureAppearance(std::string value) {
    signature_appearance_ = std::move(value);
}
bool PdfFileSignature::IsLtvEnabled() const noexcept {
    try {
        return document_ != nullptr &&
               document_->ReadCatalogSignatureStateInternal().dss;
    } catch (...) {
        return false;
    }
}
bool PdfFileSignature::IsCertified() const noexcept {
    if (is_certified_) return true;
    try {
        return document_ != nullptr &&
               document_->ReadCatalogSignatureStateInternal().docmdp_permissions != 0;
    } catch (...) {
        return false;
    }
}

}  // namespace Aspose::Pdf::Facades
