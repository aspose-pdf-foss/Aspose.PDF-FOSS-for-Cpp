// =============================================================================
// pdf_file_signature_pfx_smoke_test — PdfFileSignature with a caller's
// PKCS#12 certificate (tests/fixtures/pkcs12, password "test"): signing,
// real verification (and its failure on a modified file), certification
// (DocMDP), signature removal, usage rights / LTV detection, and
// Signature::Verify on a loaded document's signature field.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_signature.hpp>
#include <aspose/pdf/forms/doc_mdp_signature.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/pkcs7.hpp>
#include <aspose/pdf/forms/signature_field.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using flatten_test::HelloWorldPdf;
using flatten_test::ReadFile;
using flatten_test::TmpPath;
using flatten_test::WritePdf;

std::string Pfx(const std::string& name) {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" / "pkcs12" / name)
        .string();
}

// Sign hello_world.pdf into `out` with `pfx`.
void SignWith(const std::string& out, const std::string& pfx,
              const std::string& field = "Signature1") {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc, out};
    Forms::PKCS7 cert{pfx, "test"};
    sig.Sign(field, "Approved", "signer@example.com", "Prague", cert);
    sig.Save();
}

void WriteFile(const std::string& path, const std::string& bytes) {
    std::ofstream(path, std::ios::binary)
        .write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

}  // namespace

TEST(PdfFileSignaturePfx, SignWithPfxVerifies) {
    const auto out = TmpPath("sig_pfx_aes");
    SignWith(out, Pfx("signer_aes.p12"));
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_TRUE(reader.VerifySignature("Signature1"));
    EXPECT_EQ(reader.GetSignerName("Signature1"), "FOSS Test Signer");
    EXPECT_EQ(reader.GetReason("Signature1"), "Approved");
    EXPECT_TRUE(reader.CoversWholeDocument("Signature1"));
    EXPECT_FALSE(reader.IsCertified());
}

TEST(PdfFileSignaturePfx, SetCertificateWithLegacyPfx) {
    const auto out = TmpPath("sig_pfx_legacy");
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, out};
        sig.SetCertificate(Pfx("signer_legacy.p12"), "test");
        Forms::PKCS7 blank;
        sig.Sign("Signature1", blank);
        sig.Save();
    }
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_TRUE(reader.VerifySignature("Signature1"));
    EXPECT_EQ(reader.GetSignerName("Signature1"), "FOSS Test Signer");
}

TEST(PdfFileSignaturePfx, WrongPasswordOrMissingFileThrows) {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc};
    Forms::PKCS7 wrong{Pfx("signer_aes.p12"), "nope"};
    EXPECT_THROW(sig.Sign("Signature1", wrong), std::runtime_error);
    Forms::PKCS7 missing{Pfx("no_such.p12"), "test"};
    EXPECT_THROW(sig.Sign("Signature1", missing), std::runtime_error);
}

TEST(PdfFileSignaturePfx, ModifiedFileFailsVerification) {
    const auto out = TmpPath("sig_pfx_tampered");
    SignWith(out, Pfx("signer_aes.p12"));
    std::string bytes = ReadFile(out);
    const auto pos = bytes.find("Hello");
    ASSERT_NE(pos, std::string::npos);
    bytes[pos] = 'J';  // inside the signed byte range
    WriteFile(out, bytes);
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_FALSE(reader.VerifySignature("Signature1"));
}

TEST(PdfFileSignaturePfx, EarlierSignatureSurvivesLaterRevision) {
    const auto first = TmpPath("sig_pfx_rev1");
    const auto second = TmpPath("sig_pfx_rev2");
    SignWith(first, Pfx("signer_aes.p12"), "First");
    {
        Document doc{first};
        PdfFileSignature sig{doc, second};
        Forms::PKCS7 cert{Pfx("signer_legacy.p12"), "test"};
        sig.Sign("Second", cert);
        sig.Save();
    }
    Document doc{second};
    PdfFileSignature reader{doc};
    EXPECT_EQ(reader.GetTotalRevision(), 2);
    EXPECT_TRUE(reader.VerifySignature("First"));
    EXPECT_TRUE(reader.VerifySignature("Second"));
    EXPECT_FALSE(reader.CoversWholeDocument("First"));
    EXPECT_TRUE(reader.CoversWholeDocument("Second"));
}

TEST(PdfFileSignaturePfx, CertifyWritesDocMdp) {
    const auto out = TmpPath("sig_pfx_certify");
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, out};
        Forms::PKCS7 cert{Pfx("signer_aes.p12"), "test"};
        Forms::DocMDPSignature mdp{cert, Forms::DocMDPAccessPermissions::FillingInForms};
        sig.Certify("Certification", mdp);
        EXPECT_TRUE(sig.IsCertified());
        sig.Save();
    }
    const std::string bytes = ReadFile(out);
    EXPECT_NE(bytes.find("/TransformMethod /DocMDP"), std::string::npos);
    EXPECT_NE(bytes.find("/Perms"), std::string::npos);

    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_TRUE(reader.IsCertified());
    EXPECT_EQ(reader.GetAccessPermissions(), Forms::DocMDPAccessPermissions::FillingInForms);
    EXPECT_TRUE(reader.VerifySignature("Certification"));
}

TEST(PdfFileSignaturePfx, RemoveSignatureDropsFieldAndWidget) {
    const auto signed_path = TmpPath("sig_pfx_remove_src");
    const auto out = TmpPath("sig_pfx_removed");
    SignWith(signed_path, Pfx("signer_aes.p12"));
    {
        Document doc{signed_path};
        PdfFileSignature sig{doc, out};
        sig.RemoveSignature("Signature1");
        sig.Save();
    }
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_FALSE(reader.ContainsSignature());
    EXPECT_TRUE(reader.GetSignNames(false).empty());
    EXPECT_EQ(flatten_test::CountSubtype(flatten_test::ParsePage(ReadFile(out), 0), "Widget"), 0);
}

TEST(PdfFileSignaturePfx, RemoveSignatureCanKeepEmptyField) {
    const auto signed_path = TmpPath("sig_pfx_keep_src");
    const auto out = TmpPath("sig_pfx_keep");
    SignWith(signed_path, Pfx("signer_aes.p12"));
    {
        Document doc{signed_path};
        PdfFileSignature sig{doc, out};
        sig.RemoveSignature("Signature1", true);
        sig.Save();
    }
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_FALSE(reader.ContainsSignature());
    const auto blank = reader.GetBlankSignNames();
    ASSERT_EQ(blank.size(), 1u);
    EXPECT_EQ(blank[0], "Signature1");
    EXPECT_FALSE(reader.VerifySignature("Signature1"));
}

TEST(PdfFileSignaturePfx, RemoveSignaturesUncertifies) {
    const auto certified = TmpPath("sig_pfx_uncert_src");
    const auto out = TmpPath("sig_pfx_uncert");
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, certified};
        Forms::PKCS7 cert{Pfx("signer_aes.p12"), "test"};
        Forms::DocMDPSignature mdp{cert, Forms::DocMDPAccessPermissions::NoChanges};
        sig.Certify("Certification", mdp);
        sig.Save();
    }
    {
        Document doc{certified};
        PdfFileSignature sig{doc, out};
        sig.RemoveSignatures();
        sig.Save();
    }
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_FALSE(reader.ContainsSignature());
    EXPECT_FALSE(reader.IsCertified());
}

TEST(PdfFileSignaturePfx, LoadedSignatureFieldVerifies) {
    const auto out = TmpPath("sig_pfx_field");
    SignWith(out, Pfx("signer_aes.p12"));
    Document doc{out};
    Forms::SignatureField* field = nullptr;
    for (Forms::Field* f : doc.Form().Fields())
        if (auto* sf = dynamic_cast<Forms::SignatureField*>(f)) field = sf;
    ASSERT_NE(field, nullptr);
    ASSERT_NE(field->Signature(), nullptr);
    EXPECT_EQ(field->Signature()->Reason(), "Approved");
    EXPECT_EQ(field->Signature()->ByteRange().size(), 4u);
    EXPECT_TRUE(field->Signature()->Verify());
}

TEST(PdfFileSignaturePfx, UsageRightsAndLtvDetection) {
    const auto path = TmpPath("sig_ur3_dss");
    WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R /Perms << /UR3 4 0 R >> /DSS << /Certs [] >> >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] >>",
        "<< /Type /Sig /Filter /Adobe.PPKLite /SubFilter /adbe.pkcs7.detached >>",
    });
    const auto out = TmpPath("sig_ur3_removed");
    {
        Document doc{path};
        PdfFileSignature sig{doc, out};
        EXPECT_TRUE(sig.ContainsUsageRights());
        EXPECT_TRUE(sig.IsLtvEnabled());
        sig.RemoveUsageRights();
        EXPECT_FALSE(sig.ContainsUsageRights());
        sig.Save();
    }
    Document doc{out};
    PdfFileSignature reader{doc};
    EXPECT_FALSE(reader.ContainsUsageRights());
    EXPECT_TRUE(reader.IsLtvEnabled());
}

// The PKCS#12-signed PDF verifies with the openssl CLI (Linux CI / WSL).
TEST(PdfFileSignaturePfx, OpenSslVerifiesPfxSignature) {
    if (std::system("command -v openssl >/dev/null 2>&1") != 0)
        GTEST_SKIP() << "openssl CLI not available";
    const auto out = TmpPath("sig_pfx_openssl");
    SignWith(out, Pfx("signer_aes.p12"));
    const std::string bytes = ReadFile(out);
    const auto brp = bytes.rfind("/ByteRange [");
    ASSERT_NE(brp, std::string::npos);
    long long br[4];
    ASSERT_EQ(std::sscanf(bytes.c_str() + brp, "/ByteRange [%lld %lld %lld %lld",
                          &br[0], &br[1], &br[2], &br[3]), 4);
    const std::string content = bytes.substr(static_cast<std::size_t>(br[0]),
                                             static_cast<std::size_t>(br[1])) +
                                bytes.substr(static_cast<std::size_t>(br[2]),
                                             static_cast<std::size_t>(br[3]));
    const std::string hex = bytes.substr(static_cast<std::size_t>(br[1]) + 1,
                                         static_cast<std::size_t>(br[2] - br[1]) - 2);
    std::string der;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
        der.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    const auto cpath = TmpPath("sig_pfx_openssl_content");
    const auto dpath = TmpPath("sig_pfx_openssl_der");
    WriteFile(cpath, content);
    WriteFile(dpath, der);
    const std::string cmd = "openssl cms -verify -binary -inform DER -in '" + dpath +
                            "' -content '" + cpath + "' -CAfile '" + Pfx("ca.pem") +
                            "' -purpose any -out /dev/null 2>/dev/null";
    EXPECT_EQ(std::system(cmd.c_str()), 0);
}
