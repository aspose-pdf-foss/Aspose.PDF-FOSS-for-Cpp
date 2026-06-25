// =============================================================================
// document_encrypt_save_smoke_test — round-trip E4 contract:
//   * Default-construct an empty Document
//   * doc.Encrypt(user, owner, perms, algo)
//   * doc.Save(tmp_path)
//   * Re-open tmp_path with the user / owner password — IsEncrypted=true
//   * Re-open tmp_path with the wrong password — std::runtime_error
//
// Covers all four canonical CryptoAlgorithm values across the
// Permissions overload and the DocumentPrivilege overload.
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/permissions.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

using Aspose::Pdf::CryptoAlgorithm;
using Aspose::Pdf::Document;
using Aspose::Pdf::Permissions;
using Aspose::Pdf::Facades::DocumentPrivilege;

std::string TempPath(const std::string& suffix) {
    auto p = std::filesystem::temp_directory_path()
        / ("aspose_pdf_foss_encrypt_save_" + suffix + ".pdf");
    return p.string();
}

void RoundTripPermissions(CryptoAlgorithm algo,
                          bool usePdf20,
                          const std::string& tag) {
    const auto out_path = TempPath(tag);

    {
        Document doc;
        doc.Encrypt("user", "owner",
                    Permissions::PrintDocument | Permissions::FillForm,
                    algo, usePdf20);
        doc.Save(out_path);
    }

    {
        Document reopened(out_path, "user");
        EXPECT_TRUE(reopened.IsEncrypted()) << "tag=" << tag;
    }
    {
        Document reopened(out_path, "owner");
        EXPECT_TRUE(reopened.IsEncrypted()) << "tag=" << tag;
    }
    EXPECT_THROW(Document(out_path, "wrong"), std::runtime_error)
        << "tag=" << tag;

    std::filesystem::remove(out_path);
}

}  // namespace

TEST(DocumentEncryptSave, RoundTrip_RC4x40) {
    RoundTripPermissions(CryptoAlgorithm::RC4x40, false, "rc4x40");
}

TEST(DocumentEncryptSave, RoundTrip_RC4x128) {
    RoundTripPermissions(CryptoAlgorithm::RC4x128, false, "rc4x128");
}

TEST(DocumentEncryptSave, RoundTrip_AESx128) {
    RoundTripPermissions(CryptoAlgorithm::AESx128, false, "aesx128");
}

TEST(DocumentEncryptSave, RoundTrip_AESx256_R5) {
    // usePdf20=false selects R=5 (Adobe Supplement single-SHA-256)
    RoundTripPermissions(CryptoAlgorithm::AESx256, false, "aesx256_r5");
}

TEST(DocumentEncryptSave, RoundTrip_AESx256_R6) {
    // usePdf20=true selects R=6 (PDF 2.0 Algorithm 2.B)
    RoundTripPermissions(CryptoAlgorithm::AESx256, true, "aesx256_r6");
}

TEST(DocumentEncryptSave, RoundTrip_DocumentPrivilege) {
    const auto out_path = TempPath("doc_privilege");
    {
        Document doc;
        doc.Encrypt("user", "owner",
                    DocumentPrivilege::Print(),
                    CryptoAlgorithm::AESx128);
        doc.Save(out_path);
    }
    {
        Document reopened(out_path, "user");
        EXPECT_TRUE(reopened.IsEncrypted());
    }
    EXPECT_THROW(Document(out_path, "nope"), std::runtime_error);
    std::filesystem::remove(out_path);
}

TEST(DocumentEncryptSave, EmptyOwnerPasswordOk) {
    // owner password "" — encrypt_writer's Algorithm 3 falls back
    // to the user password when owner is empty.
    const auto out_path = TempPath("empty_owner");
    {
        Document doc;
        doc.Encrypt("user", "",
                    Permissions::PrintDocument,
                    CryptoAlgorithm::AESx128);
        doc.Save(out_path);
    }
    {
        Document reopened(out_path, "user");
        EXPECT_TRUE(reopened.IsEncrypted());
    }
    std::filesystem::remove(out_path);
}
