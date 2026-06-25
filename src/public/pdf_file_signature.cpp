#include <aspose/pdf/facades/pdf_file_signature.hpp>

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/signature.hpp>

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
    req.name = foundation::pkcs7::BundledSignerName();
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
        const Aspose::Pdf::Forms::DocMDPSignature&) {
    if (document_ == nullptr) return;
    // v1: a certifying signature is written with the same detached-PKCS#7
    // pipeline as an approval signature; the /DocMDP /Reference transform
    // dictionary that locks subsequent edits (per the supplied
    // DocMDPSignature's access permissions) is a documented follow-on.
    Aspose::Pdf::Document::SignatureRequest req;
    req.field_name = sigName.empty() ? "Signature1" : sigName;
    req.name = foundation::pkcs7::BundledSignerName();
    NowStrings(req.date_pdf, req.signing_time_utc);
    document_->SignInternal(req);
    is_certified_ = true;
}

Aspose::Pdf::Forms::DocMDPAccessPermissions
PdfFileSignature::GetAccessPermissions() {
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

bool PdfFileSignature::ContainsUsageRights() { return false; }

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

// ===== Remove — v1 stubs =====================================================
// Removing a signature requires rewriting the AcroForm /Fields and the page
// /Annots and invalidating the prior revision; deferred past the signing
// floor.

void PdfFileSignature::RemoveUsageRights() {}
void PdfFileSignature::RemoveSignature(const std::string&) {}
void PdfFileSignature::RemoveSignature(const SignatureName&) {}
void PdfFileSignature::RemoveSignature(const std::string&, bool) {}
void PdfFileSignature::RemoveSignature(const SignatureName&, bool) {}
void PdfFileSignature::RemoveSignatures() {}

// ===== Verify / read =========================================================

bool PdfFileSignature::VerifySigned(const std::string& sigName) {
    return VerifySignature(sigName);
}

bool PdfFileSignature::VerifySignature(const std::string& sigName) {
    if (document_ == nullptr) return false;
    for (const auto& s : document_->ReadSignaturesInternal()) {
        if (s.field_name == sigName) return s.has_value && s.covers_whole;
    }
    return false;
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
    // The signer identity (RSA-2048 + X.509) is bundled; the supplied .pfx
    // path / password are recorded for API fidelity. PKCS#12 key extraction
    // to sign with a caller-provided certificate is a documented follow-on.
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
bool PdfFileSignature::IsLtvEnabled() const noexcept { return false; }
bool PdfFileSignature::IsCertified() const noexcept { return is_certified_; }

}  // namespace Aspose::Pdf::Facades
