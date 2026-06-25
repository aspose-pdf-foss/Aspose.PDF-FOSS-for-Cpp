// =============================================================================
// Go-parity: encrypt_test.go / decrypt_test.go / permissions_test.go /
// encryption_algorithm_test.go → canonical C++ Document::Encrypt / Decrypt /
// Permissions + Permissions / CryptoAlgorithm enums.
//
// Ported (canonical): encrypt + reopen with user/owner password, wrong-password
// rejection, page-count preservation, AES-128 / RC4-128 / AES-256 round-trip,
// Permissions() round-trip, Permissions bit values + composition,
// CryptoAlgorithm constants.
// Skipped: pypdf cross-tool byte assertions and open-plain-PDF-with-password
// (Go-specific / tool-coupled); content-obfuscation byte scan (fragile).
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/permissions.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path PdfDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__)
        .parent_path()
        .parent_path()
        .parent_path() /
        "pdfs";
}

std::string TwoPages() { return (PdfDir() / "two_pages.pdf").string(); }

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_enc_" + tag + ".pdf"))
        .string();
}

std::string MakeEncrypted(const std::string& tag, CryptoAlgorithm algo) {
    const std::string out = TmpOut(tag);
    Document doc{TwoPages()};
    doc.Encrypt("user", "owner",
                Permissions::PrintDocument | Permissions::FillForm, algo);
    doc.Save(out);
    return out;
}

}  // namespace

// TestEncryptSetPassword / TestOpenWithPasswordRoundTrip
TEST(GoEncryption, EncryptThenOpenWithUserPassword) {
    const std::string out = MakeEncrypted("user", CryptoAlgorithm::AESx128);
    Document doc{out, "user"};
    EXPECT_TRUE(doc.IsEncrypted());
    std::filesystem::remove(out);
}

// TestOpenWithOwnerPassword / TestSetEncryptionAES128_OwnerPasswordRecovery
TEST(GoEncryption, OpenWithOwnerPassword) {
    const std::string out = MakeEncrypted("owner", CryptoAlgorithm::AESx128);
    Document doc{out, "owner"};
    EXPECT_TRUE(doc.IsEncrypted());
    std::filesystem::remove(out);
}

// TestOpenWithWrongPassword / TestSetEncryptionAES128_WrongPassword
TEST(GoEncryption, WrongPasswordThrows) {
    const std::string out = MakeEncrypted("wrong", CryptoAlgorithm::AESx128);
    EXPECT_THROW(Document(out, "nope"), std::runtime_error);
    std::filesystem::remove(out);
}

// TestEncryptPreservesPageCount
TEST(GoEncryption, PreservesPageCount) {
    const std::string out = MakeEncrypted("count", CryptoAlgorithm::AESx128);
    Document doc{out, "user"};
    EXPECT_EQ(doc.Pages().Count(), 2);
    std::filesystem::remove(out);
}

// TestSetEncryptionAES128_RoundTrip / RC4 / AES256 — every algorithm opens back.
TEST(GoEncryption, AllAlgorithmsRoundTrip) {
    for (const auto algo : {CryptoAlgorithm::RC4x128, CryptoAlgorithm::AESx128,
                            CryptoAlgorithm::AESx256}) {
        const std::string out = MakeEncrypted("algo", algo);
        Document doc{out, "user"};
        EXPECT_TRUE(doc.IsEncrypted());
        std::filesystem::remove(out);
    }
}

// TestSetEncryptionAES128_PermissionsRoundTrip / TestSetPermissionsPropagates
TEST(GoEncryption, PermissionsRoundTrip) {
    const std::string out = MakeEncrypted("perms", CryptoAlgorithm::AESx128);
    Document doc{out, "user"};
    const int p = doc.Permissions();
    EXPECT_NE(p & static_cast<int>(Permissions::PrintDocument), 0);
    EXPECT_NE(p & static_cast<int>(Permissions::FillForm), 0);
    std::filesystem::remove(out);
}

// TestPermissionsBitPacking — canonical bit values + bitwise composition.
TEST(GoEncryption, PermissionsBitValues) {
    EXPECT_EQ(static_cast<int>(Permissions::PrintDocument), 4);
    EXPECT_EQ(static_cast<int>(Permissions::ModifyContent), 8);
    EXPECT_EQ(static_cast<int>(Permissions::ExtractContent), 16);
    const Permissions combo =
        Permissions::PrintDocument | Permissions::ExtractContent;
    EXPECT_NE(static_cast<int>(combo & Permissions::PrintDocument), 0);
    EXPECT_NE(static_cast<int>(combo & Permissions::ExtractContent), 0);
    EXPECT_EQ(static_cast<int>(combo & Permissions::ModifyContent), 0);
}

// TestEncryptionAlgorithmConstants / TestEncryptionAlgAES256Constant
TEST(GoEncryption, CryptoAlgorithmConstants) {
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::RC4x40), 0);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::RC4x128), 1);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::AESx128), 2);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::AESx256), 3);
}
