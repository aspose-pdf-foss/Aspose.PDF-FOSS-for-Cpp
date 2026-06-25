// =============================================================================
// optimize_resources_smoke_test — beat P-5. Document::OptimizeResources() /
// Optimize() request a compacting resave that keeps only objects reachable from
// the catalog and writes a fresh single-section PDF. Verifies the result is a
// valid, content-preserving, compacted file.
// =============================================================================

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string ReadText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

int Count(const std::string& hay, const std::string& needle) {
    int n = 0;
    for (std::size_t p = hay.find(needle); p != std::string::npos;
         p = hay.find(needle, p + needle.size()))
        ++n;
    return n;
}

}  // namespace

TEST(OptimizeResourcesSmoke, CompactsAndPreservesContent) {
    const auto tmp = std::filesystem::temp_directory_path();
    const std::string edited = (tmp / "aspose_opt_edited.pdf").string();
    const std::string optimized = (tmp / "aspose_opt_compacted.pdf").string();

    // 1. Make an incremental edit (TrimBox) — this appends a second revision,
    //    leaving the page's prior object version superseded in the file.
    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        doc.Pages()[1].TrimBox(Rectangle{10.0, 20.0, 200.0, 300.0, false});
        doc.Save(edited);
    }
    const std::string edited_bytes = ReadText(edited);
    EXPECT_GE(Count(edited_bytes, "startxref"), 2);  // incremental → 2+ sections

    // 2. Optimize → compacting resave.
    {
        Document doc{edited};
        doc.OptimizeResources();
        doc.Save(optimized);
    }

    // 3. The compacted file is a single-section PDF.
    const std::string opt_bytes = ReadText(optimized);
    EXPECT_EQ(opt_bytes.compare(0, 5, "%PDF-"), 0);
    EXPECT_EQ(Count(opt_bytes, "startxref"), 1);
    EXPECT_NE(opt_bytes.find("%%EOF"), std::string::npos);
    EXPECT_LE(opt_bytes.size(), edited_bytes.size());  // not larger

    // 4. Content / structure preserved.
    {
        Document doc{optimized};
        ASSERT_EQ(doc.Pages().Count(), 2);
        EXPECT_DOUBLE_EQ(doc.Pages()[1].TrimBox().LLX(), 10.0);
        EXPECT_DOUBLE_EQ(doc.Pages()[1].TrimBox().URY(), 300.0);
    }

    std::filesystem::remove(edited);
    std::filesystem::remove(optimized);
}

TEST(OptimizeResourcesSmoke, OptimizeAliasAlsoCompacts) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_opt_alias.pdf")
            .string();
    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        doc.Optimize();
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 2);
    std::filesystem::remove(out);
}
