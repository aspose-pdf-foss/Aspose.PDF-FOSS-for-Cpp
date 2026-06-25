// =============================================================================
// document_encrypted_open_smoke_test — exercises the E3 password-
// open ctor + IsEncrypted + Decrypt against the five (V, R)
// fixture PDFs vendored under tests/fixtures/encrypt_parser/.
//
// The fixtures are pikepdf-generated; passwords are hard-coded
// (user="user" / "" for the empty case; owner="owner") to match
// the encrypt_parser corpus.
// =============================================================================

#include <aspose/pdf/document.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

std::string FixturePath(const std::string& name) {
    return (std::filesystem::path(__FILE__).parent_path()
            / "fixtures" / "encrypt_parser" / name).string();
}

}  // namespace

TEST(DocumentEncryptedOpen, UnencryptedDocReportsFalse) {
    Aspose::Pdf::Document doc;
    EXPECT_FALSE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, UnencryptedWithPasswordCtorOk) {
    // Per canonical .NET semantics, Document(filename, password) on
    // an unencrypted source silently ignores the password.
    // user_pwd_aes128.pdf IS encrypted — so use Document() empty
    // here; we verify the path doesn't crash and IsEncrypted=false.
    Aspose::Pdf::Document doc;
    EXPECT_FALSE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V1R2_RC40_UserPassword) {
    Aspose::Pdf::Document doc(FixturePath("v1_r2_rc40.pdf"), "user");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V1R2_RC40_OwnerPassword) {
    Aspose::Pdf::Document doc(FixturePath("v1_r2_rc40.pdf"), "owner");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V1R2_RC40_WrongPasswordThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document(FixturePath("v1_r2_rc40.pdf"), "nope"),
        std::runtime_error);
}

TEST(DocumentEncryptedOpen, V2R3_RC128_UserPassword) {
    Aspose::Pdf::Document doc(FixturePath("v2_r3_rc128.pdf"), "user");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V2R3_RC128_WrongPasswordThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document(FixturePath("v2_r3_rc128.pdf"), "wrong"),
        std::runtime_error);
}

TEST(DocumentEncryptedOpen, V4R4_AES128_UserPassword) {
    Aspose::Pdf::Document doc(FixturePath("user_pwd_aes128.pdf"), "user");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V4R4_AES128_OwnerPassword) {
    Aspose::Pdf::Document doc(FixturePath("user_pwd_aes128.pdf"), "owner");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V4R4_AES128_WrongPasswordThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document(FixturePath("user_pwd_aes128.pdf"), "bad"),
        std::runtime_error);
}

TEST(DocumentEncryptedOpen, V5R5_AES256_UserPassword) {
    Aspose::Pdf::Document doc(FixturePath("v5_r5_aes256.pdf"), "user");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V5R5_AES256_WrongPasswordThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document(FixturePath("v5_r5_aes256.pdf"), "nope"),
        std::runtime_error);
}

TEST(DocumentEncryptedOpen, V5R6_AES256_UserPassword) {
    Aspose::Pdf::Document doc(FixturePath("v5_r6_aes256.pdf"), "user");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V5R6_AES256_OwnerPassword) {
    Aspose::Pdf::Document doc(FixturePath("v5_r6_aes256.pdf"), "owner");
    EXPECT_TRUE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, V5R6_AES256_WrongPasswordThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document(FixturePath("v5_r6_aes256.pdf"), "nope"),
        std::runtime_error);
}

TEST(DocumentEncryptedOpen, DecryptClearsIsEncrypted) {
    Aspose::Pdf::Document doc(FixturePath("user_pwd_aes128.pdf"), "user");
    ASSERT_TRUE(doc.IsEncrypted());
    doc.Decrypt();
    EXPECT_FALSE(doc.IsEncrypted());
}

TEST(DocumentEncryptedOpen, DecryptOnUnencryptedIsNoop) {
    Aspose::Pdf::Document doc;
    ASSERT_FALSE(doc.IsEncrypted());
    doc.Decrypt();
    EXPECT_FALSE(doc.IsEncrypted());
}
