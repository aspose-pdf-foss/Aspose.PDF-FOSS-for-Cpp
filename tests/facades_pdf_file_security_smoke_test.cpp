// =============================================================================
// facades_pdf_file_security_smoke_test — beat Fa5 of the Facades
// cluster. PdfFileSecurity encrypts / decrypts / changes passwords /
// sets permissions on a bound PDF. v1 wires these through the REAL
// foundation crypto (Document::Encrypt / Decrypt), input-file ->
// output-file. SetPrivilege(privilege) alone is a documented stub.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/facades/algorithm.hpp>
#include <aspose/pdf/facades/key_size.hpp>
#include <aspose/pdf/facades/pdf_file_security.hpp>

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

std::string TempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_filesec_" + name + ".pdf")).string();
}

}  // namespace

TEST(FacadesPdfFileSecuritySmoke, EncryptFileThenReopenWithPassword) {
    const std::string out = TempPath("enc_aes");
    PdfFileSecurity sec{HelloWorldPdf(), out};
    EXPECT_TRUE(sec.EncryptFile("user", "owner",
                                DocumentPrivilege::AllowAll(),
                                KeySize::x128, Algorithm::AES));
    ASSERT_TRUE(std::filesystem::exists(out));

    Document reUser{out, "user"};
    EXPECT_TRUE(reUser.IsEncrypted());
    Document reOwner{out, "owner"};
    EXPECT_TRUE(reOwner.IsEncrypted());
    EXPECT_THROW(Document(out, "wrong"), std::runtime_error);
}

TEST(FacadesPdfFileSecuritySmoke, EncryptFileDefaultCipher) {
    const std::string out = TempPath("enc_default");
    PdfFileSecurity sec;
    sec.InputFile(HelloWorldPdf());
    sec.OutputFile(out);
    EXPECT_TRUE(sec.EncryptFile("u", "o", DocumentPrivilege::Print(),
                                KeySize::x128));
    Document re{out, "u"};
    EXPECT_TRUE(re.IsEncrypted());
}

TEST(FacadesPdfFileSecuritySmoke, DecryptFileRoundTrip) {
    const std::string enc = TempPath("dec_src");
    const std::string dec = TempPath("dec_out");

    PdfFileSecurity enc_sec{HelloWorldPdf(), enc};
    ASSERT_TRUE(enc_sec.EncryptFile("user", "owner",
                                    DocumentPrivilege::AllowAll(),
                                    KeySize::x128, Algorithm::AES));

    PdfFileSecurity dec_sec{enc, dec};
    EXPECT_TRUE(dec_sec.DecryptFile("owner"));
    ASSERT_TRUE(std::filesystem::exists(dec));

    Document re{dec};  // no password — decrypted
    EXPECT_FALSE(re.IsEncrypted());
}

TEST(FacadesPdfFileSecuritySmoke, ChangePasswordRoundTrip) {
    const std::string enc = TempPath("chg_src");
    const std::string out = TempPath("chg_out");

    PdfFileSecurity enc_sec{HelloWorldPdf(), enc};
    ASSERT_TRUE(enc_sec.EncryptFile("user", "owner",
                                    DocumentPrivilege::AllowAll(),
                                    KeySize::x128, Algorithm::AES));

    PdfFileSecurity chg{enc, out};
    EXPECT_TRUE(chg.ChangePassword("owner", "newuser", "newowner"));

    Document re{out, "newuser"};
    EXPECT_TRUE(re.IsEncrypted());
    EXPECT_THROW(Document(out, "user"), std::runtime_error);
}

TEST(FacadesPdfFileSecuritySmoke, SetPrivilegeWithPasswords) {
    const std::string out = TempPath("setpriv");
    PdfFileSecurity sec{HelloWorldPdf(), out};
    EXPECT_TRUE(sec.SetPrivilege("user", "owner",
                                 DocumentPrivilege::Print()));
    Document re{out, "user"};
    EXPECT_TRUE(re.IsEncrypted());
}

TEST(FacadesPdfFileSecuritySmoke, SetPrivilegeNoPasswordIsStub) {
    PdfFileSecurity sec{HelloWorldPdf(), TempPath("setpriv_stub")};
    EXPECT_FALSE(sec.SetPrivilege(DocumentPrivilege::ForbidAll()));
}

TEST(FacadesPdfFileSecuritySmoke, ExceptionPolicy) {
    // No input bound: non-Try returns false by default (AllowExceptions
    // off), Try always returns false, and non-Try rethrows once
    // AllowExceptions is on.
    PdfFileSecurity sec;
    sec.OutputFile(TempPath("nope"));
    EXPECT_FALSE(sec.EncryptFile("u", "o", DocumentPrivilege::AllowAll(),
                                 KeySize::x128));
    EXPECT_FALSE(sec.TryEncryptFile("u", "o", DocumentPrivilege::AllowAll(),
                                    KeySize::x128));

    sec.AllowExceptions(true);
    EXPECT_THROW(sec.EncryptFile("u", "o", DocumentPrivilege::AllowAll(),
                                 KeySize::x128),
                 std::runtime_error);
    // Try* still swallows even with AllowExceptions on.
    EXPECT_FALSE(sec.TryEncryptFile("u", "o", DocumentPrivilege::AllowAll(),
                                    KeySize::x128));
}

TEST(FacadesPdfFileSecuritySmoke, BindPdfSetsInput) {
    const std::string out = TempPath("bind");
    PdfFileSecurity sec;
    sec.BindPdf(HelloWorldPdf());
    sec.OutputFile(out);
    EXPECT_TRUE(sec.EncryptFile("user", "owner",
                                DocumentPrivilege::AllowAll(),
                                KeySize::x256, Algorithm::AES));
    Document re{out, "user"};
    EXPECT_TRUE(re.IsEncrypted());
}
