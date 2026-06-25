// =============================================================================
// page_collection_indexer_smoke_test — exercise the canonical 1-based
// indexer on PageCollection. Returns Page objects whose Number()
// matches the 1-based index. Out-of-range indices throw std::out_of_range.
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>

namespace {
std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "page_renderer";
}
}

TEST(PageCollectionIndexer, SinglePage_Index1IsPage1) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto& pages = doc.Pages();
    ASSERT_EQ(pages.Count(), 1u);
    auto p1 = pages[1];
    EXPECT_EQ(p1.Number(), 1);
}

TEST(PageCollectionIndexer, MultiPage_IndexMatchesNumber) {
    const auto pdf = FixtureRoot() / "two_pages.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto& pages = doc.Pages();
    ASSERT_EQ(pages.Count(), 2u);
    EXPECT_EQ(pages[1].Number(), 1);
    EXPECT_EQ(pages[2].Number(), 2);
}

TEST(PageCollectionIndexer, ZeroOrNegative_Throws) {
    const auto pdf = FixtureRoot() / "two_pages.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto& pages = doc.Pages();
    EXPECT_THROW((void)pages[0],  std::out_of_range);
    EXPECT_THROW((void)pages[-1], std::out_of_range);
}

TEST(PageCollectionIndexer, BeyondCount_Throws) {
    const auto pdf = FixtureRoot() / "two_pages.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto& pages = doc.Pages();
    EXPECT_THROW((void)pages[3], std::out_of_range);
    EXPECT_THROW((void)pages[100], std::out_of_range);
}
