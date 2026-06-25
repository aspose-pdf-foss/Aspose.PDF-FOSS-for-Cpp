// =============================================================================
// document_permissions_getter_smoke_test — beat P-2. Document::Permissions()
// returns the current /P permission bitfield: all-permissions for a plain
// document, and the staged restriction once Encrypt is requested.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/permissions.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

// /P bit positions (ISO 32000-1 Table 22) — bit 3 = print, bit 4 = modify.
constexpr int kPrintBit = 4;
constexpr int kModifyBit = 8;

}  // namespace

TEST(DocumentPermissionsGetterSmoke, PlainDocumentAllowsEverything) {
    Document doc{(FixtureRoot() / "two_pages.pdf").string()};
    const int p = doc.Permissions();
    EXPECT_NE(p & kPrintBit, 0);
    EXPECT_NE(p & kModifyBit, 0);
}

TEST(DocumentPermissionsGetterSmoke, ReflectsStagedEncryption) {
    Document doc{(FixtureRoot() / "two_pages.pdf").string()};
    // Allow printing only.
    doc.Encrypt("user", "owner", Permissions::PrintDocument,
                CryptoAlgorithm::RC4x128);
    const int p = doc.Permissions();
    EXPECT_NE(p & kPrintBit, 0);    // print permitted
    EXPECT_EQ(p & kModifyBit, 0);   // content modification denied
}
