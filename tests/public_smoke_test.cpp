// =============================================================================
// public_smoke_test — end-to-end exercise of the Document/PageCollection
// public API. Proves LibForge produced a working library: open a real
// PDF, read page count, save back byte-identical.
//
// Driven by gtest. Fixture path passed via SMOKE_FIXTURE_DIR env var.
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path FixtureDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    // Fallback to the repo-relative default; the workspace lays Fixtures
    // at <workspace>/the test fixtures
    return std::filesystem::path(__FILE__)
        .parent_path()             // tests/
        / "fixtures" / "pages_tree";
}

std::vector<std::byte> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) {
        return {};
    }
    const auto size = f.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

}  // namespace

// ---------------------------------------------------------------------------
// Single-page PDF — Pages.Count == 1, Save roundtrip is byte-identical.
// ---------------------------------------------------------------------------

TEST(PublicSmoke, SinglePage_OpenCountSaveRoundtrip) {
    const auto src = FixtureDir() / "single.pdf";
    ASSERT_TRUE(std::filesystem::exists(src))
        << "fixture not found: " << src;

    Aspose::Pdf::Document doc(src.string());
    EXPECT_EQ(doc.Pages().Count(), 1u);

    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_smoke_single_out.pdf";
    doc.Save(out.string());

    EXPECT_EQ(ReadFile(src), ReadFile(out))
        << "Save() should produce a byte-identical roundtrip for a "
           "doc loaded read-only";
    std::filesystem::remove(out);
}

// ---------------------------------------------------------------------------
// Three-page PDF with non-uniform geometry — Pages.Count == 3.
// ---------------------------------------------------------------------------

TEST(PublicSmoke, ThreePages_CountIsThree) {
    const auto src = FixtureDir() / "three_sizes.pdf";
    ASSERT_TRUE(std::filesystem::exists(src))
        << "fixture not found: " << src;

    Aspose::Pdf::Document doc(src.string());
    EXPECT_EQ(doc.Pages().Count(), 3u);
}

// ---------------------------------------------------------------------------
// Pages.Count survives multiple successive calls without re-parsing.
// ---------------------------------------------------------------------------

TEST(PublicSmoke, PagesCountStableAcrossCalls) {
    Aspose::Pdf::Document doc(
        (FixtureDir() / "three_sizes.pdf").string());
    EXPECT_EQ(doc.Pages().Count(), doc.Pages().Count());
}

// ---------------------------------------------------------------------------
// Missing file errors out via std::system_error.
// ---------------------------------------------------------------------------

TEST(PublicSmoke, MissingFileThrows) {
    EXPECT_THROW(
        Aspose::Pdf::Document("/nonexistent/path/never_exists.pdf"),
        std::system_error);
}
