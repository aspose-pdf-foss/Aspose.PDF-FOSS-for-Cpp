// =============================================================================
// facades_pdf_file_signature_smoke_test — PdfFileSignature + SignatureName.
//
// The unsigned-document cases below assert the empty / default read-back on
// a document with no signatures. The *RealSigning* cases drive the wired
// two-phase signing pipeline: a detached PKCS#7 (`adbe.pkcs7.detached`)
// signature with a byte-exact /ByteRange, then read it back and (when the
// openssl CLI is available) cryptographically verify the embedded blob.
// =============================================================================

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_signature.hpp>
#include <aspose/pdf/facades/signature_name.hpp>
#include <aspose/pdf/forms/doc_mdp_access_permissions.hpp>
#include <aspose/pdf/forms/pkcs7.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

}  // namespace

TEST(FacadesSignatureNameSmoke, ValueTypeRoundtrip) {
    SignatureName sn;
    EXPECT_TRUE(sn.Name.empty());
    EXPECT_TRUE(sn.FullName.empty());
    EXPECT_FALSE(sn.HasSignature());

    sn.Name = "Signature1";
    sn.FullName = "form.Signature1";
    EXPECT_EQ(sn.ToString(), "form.Signature1");
    EXPECT_EQ(sn.GetHashCode(),
              static_cast<int>(std::hash<std::string>{}("form.Signature1")));

    SignatureName empty;
    EXPECT_EQ(empty.ToString(), "");
}

TEST(FacadesPdfFileSignatureSmoke, EnumerationsEmptyOnUnsignedDoc) {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc};

    EXPECT_FALSE(sig.ContainsSignature());
    EXPECT_FALSE(sig.IsContainSignature());
    EXPECT_FALSE(sig.ContainsUsageRights());
    EXPECT_EQ(sig.GetTotalRevision(), 0);
    EXPECT_TRUE(sig.GetSignNames(false).empty());
    EXPECT_TRUE(sig.GetSignatureNames(false).empty());
    EXPECT_TRUE(sig.GetBlankSignNames().empty());
    EXPECT_TRUE(sig.GetBlankSignatureNames().empty());
}

TEST(FacadesPdfFileSignatureSmoke, ReadAccessorsReturnDefaults) {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc};

    EXPECT_FALSE(sig.VerifySignature("Sig1"));
    EXPECT_FALSE(sig.VerifySigned("Sig1"));
    EXPECT_EQ(sig.GetSignerName("Sig1"), "");
    EXPECT_EQ(sig.GetReason("Sig1"), "");
    EXPECT_EQ(sig.GetLocation("Sig1"), "");
    EXPECT_EQ(sig.GetContactInfo("Sig1"), "");
    EXPECT_EQ(sig.GetRevision("Sig1"), 0);
    EXPECT_FALSE(sig.CoversWholeDocument("Sig1"));
    EXPECT_EQ(sig.GetAccessPermissions(),
              Aspose::Pdf::Forms::DocMDPAccessPermissions::NoChanges);
    EXPECT_FALSE(sig.IsLtvEnabled());
    EXPECT_FALSE(sig.IsCertified());
}

TEST(FacadesPdfFileSignatureSmoke, MutationStubsDoNotThrow) {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc};

    sig.RemoveSignature("Sig1");
    sig.RemoveSignature("Sig1", true);
    sig.RemoveSignatures();
    sig.RemoveUsageRights();
    sig.SetCertificate("cert.pfx", "pw");
    sig.SignatureAppearance("appearance.png");
    EXPECT_EQ(sig.SignatureAppearance(), "appearance.png");
    SUCCEED();
}

TEST(FacadesPdfFileSignatureSmoke, SignatureNameOverloads) {
    Document doc{HelloWorldPdf()};
    PdfFileSignature sig{doc};
    SignatureName sn;
    sn.FullName = "Sig1";

    EXPECT_FALSE(sig.VerifySignature(sn));
    EXPECT_EQ(sig.GetSignerName(sn), "");
    EXPECT_EQ(sig.GetRevision(sn), 0);
    EXPECT_FALSE(sig.CoversWholeDocument(sn));
    sig.RemoveSignature(sn);  // no-throw
    SUCCEED();
}

// ---- Real signing pipeline -------------------------------------------------

namespace {

std::vector<unsigned char> ReadAll(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in),
                                      std::istreambuf_iterator<char>());
}

// Extract the four /ByteRange integers from a signed PDF.
std::vector<long long> ParseByteRange(const std::vector<unsigned char>& b) {
    const std::string key = "/ByteRange [";
    const std::string s(b.begin(), b.end());
    auto pos = s.find(key);
    if (pos == std::string::npos) return {};
    pos += key.size();
    std::vector<long long> out;
    while (out.size() < 4 && pos < s.size()) {
        while (pos < s.size() && (s[pos] == ' ')) ++pos;
        long long v = 0;
        bool any = false;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
            v = v * 10 + (s[pos] - '0');
            ++pos;
            any = true;
        }
        if (any) out.push_back(v);
    }
    return out;
}

// Extract the /Contents hex digits (between '<' and '>').
std::string ParseContentsHex(const std::vector<unsigned char>& b) {
    const std::string needle = "/Contents <";
    const std::string s(b.begin(), b.end());
    auto pos = s.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();  // first hex digit
    auto end = s.find('>', pos);
    return s.substr(pos, end - pos);
}

std::vector<unsigned char> HexToBytes(const std::string& hex) {
    std::vector<unsigned char> out;
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        int hi = nib(hex[i]), lo = nib(hex[i + 1]);
        if (hi < 0 || lo < 0) break;
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return out;
}

std::filesystem::path TmpDir() {
    return std::filesystem::temp_directory_path();
}

}  // namespace

TEST(FacadesPdfFileSignatureRealSigning, SignAddsReadableSignature) {
    const auto out = (TmpDir() / "asp_signed_readable.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, out};
        sig.SetCertificate("test.pfx", "pw");
        Aspose::Pdf::Forms::PKCS7 cert;
        sig.Sign("Signature1", "I approve this document", "signer@example.com",
                 "Prague", cert);
        sig.Save();
    }

    // Reload and read back through a fresh facade.
    Document signedDoc{out};
    PdfFileSignature reader{signedDoc};

    EXPECT_TRUE(reader.ContainsSignature());
    EXPECT_EQ(reader.GetTotalRevision(), 1);

    auto names = reader.GetSignNames(false);
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "Signature1");
    EXPECT_TRUE(reader.GetBlankSignNames().empty());

    EXPECT_EQ(reader.GetSignerName("Signature1"),
              "Aspose.PDF FOSS Test Signer");
    EXPECT_EQ(reader.GetReason("Signature1"), "I approve this document");
    EXPECT_EQ(reader.GetContactInfo("Signature1"), "signer@example.com");
    EXPECT_EQ(reader.GetLocation("Signature1"), "Prague");
    EXPECT_TRUE(reader.CoversWholeDocument("Signature1"));
    EXPECT_TRUE(reader.VerifySignature("Signature1"));

    auto sn = reader.GetSignatureNames(false);
    ASSERT_EQ(sn.size(), 1u);
    EXPECT_EQ(sn[0].Name, "Signature1");
    EXPECT_TRUE(sn[0].HasSignature());
}

TEST(FacadesPdfFileSignatureRealSigning, ByteRangeAndContentsAreWellFormed) {
    const auto out = (TmpDir() / "asp_signed_byterange.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, out};
        Aspose::Pdf::Forms::PKCS7 cert;
        sig.Sign("Signature1", cert);
        sig.Save();
    }

    const auto bytes = ReadAll(out);
    ASSERT_FALSE(bytes.empty());

    auto br = ParseByteRange(bytes);
    ASSERT_EQ(br.size(), 4u);
    EXPECT_EQ(br[0], 0);
    // The gap [br1, br2) must wrap the /Contents hex string in '<' '>'.
    EXPECT_EQ(static_cast<char>(bytes[br[1]]), '<');
    EXPECT_EQ(static_cast<char>(bytes[br[2] - 1]), '>');
    // The byte ranges must cover the whole file.
    EXPECT_EQ(br[2] + br[3], static_cast<long long>(bytes.size()));

    const auto hex = ParseContentsHex(bytes);
    ASSERT_FALSE(hex.empty());
    const auto der = HexToBytes(hex);
    // DER SignedData ContentInfo starts with SEQUENCE (0x30).
    ASSERT_FALSE(der.empty());
    EXPECT_EQ(der[0], 0x30u);
}

// Cryptographically verify the embedded PKCS#7 with the openssl CLI when it
// is available on PATH. The from-scratch RSA signature + messageDigest must
// validate against the signed byte-range content.
TEST(FacadesPdfFileSignatureRealSigning, OpensslVerifiesEmbeddedPkcs7) {
    if (std::system("command -v openssl >/dev/null 2>&1") != 0) {
        GTEST_SKIP() << "openssl CLI not available";
    }
    const auto out = (TmpDir() / "asp_signed_openssl.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        PdfFileSignature sig{doc, out};
        Aspose::Pdf::Forms::PKCS7 cert;
        sig.Sign("Signature1", "openssl-verify", "x@y.z", "Earth", cert);
        sig.Save();
    }

    const auto bytes = ReadAll(out);
    auto br = ParseByteRange(bytes);
    ASSERT_EQ(br.size(), 4u);

    // Reconstruct the signed content (file minus the /Contents gap) + the DER.
    std::vector<unsigned char> content;
    content.insert(content.end(), bytes.begin(), bytes.begin() + br[0] + br[1]);
    content.insert(content.end(), bytes.begin() + br[2],
                   bytes.begin() + br[2] + br[3]);
    const auto der = HexToBytes(ParseContentsHex(bytes));

    const auto cpath = (TmpDir() / "asp_sig_content.bin").string();
    const auto dpath = (TmpDir() / "asp_sig.der").string();
    {
        std::ofstream(cpath, std::ios::binary)
            .write(reinterpret_cast<const char*>(content.data()),
                   static_cast<std::streamsize>(content.size()));
        std::ofstream(dpath, std::ios::binary)
            .write(reinterpret_cast<const char*>(der.data()),
                   static_cast<std::streamsize>(der.size()));
    }
    const std::string cmd = "openssl smime -verify -inform DER -in '" + dpath +
                            "' -content '" + cpath +
                            "' -noverify -out /dev/null 2>/dev/null";
    EXPECT_EQ(std::system(cmd.c_str()), 0)
        << "openssl failed to verify the embedded PKCS#7 signature";
}
