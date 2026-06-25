// =============================================================================
// page_boxes_smoke_test — beat P-1. Page.TrimBox / BleedBox / ArtBox alongside
// the existing MediaBox / CropBox. Covers the CropBox-default getter, the
// setters, and the /TrimBox //BleedBox //ArtBox save-through round-trip.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

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

}  // namespace

TEST(PageBoxesSmoke, DefaultToCropBox) {
    Document doc{(FixtureRoot() / "two_pages.pdf").string()};
    Page page = doc.Pages()[1];
    const Rectangle crop = page.CropBox();
    // Unset Trim/Bleed/Art boxes default to the CropBox.
    EXPECT_DOUBLE_EQ(page.TrimBox().URX(), crop.URX());
    EXPECT_DOUBLE_EQ(page.BleedBox().URY(), crop.URY());
    EXPECT_DOUBLE_EQ(page.ArtBox().LLX(), crop.LLX());
}

TEST(PageBoxesSmoke, SetAndSaveRoundTrip) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_page_boxes.pdf")
            .string();
    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        Page page = doc.Pages()[1];
        page.TrimBox(Rectangle{10.0, 20.0, 200.0, 300.0, false});
        page.BleedBox(Rectangle{5.0, 15.0, 210.0, 310.0, false});
        page.ArtBox(Rectangle{12.0, 22.0, 190.0, 290.0, false});
        EXPECT_DOUBLE_EQ(page.TrimBox().LLX(), 10.0);
        doc.Save(out);
    }
    {
        Document doc{out};
        Page page = doc.Pages()[1];
        EXPECT_DOUBLE_EQ(page.TrimBox().LLX(), 10.0);
        EXPECT_DOUBLE_EQ(page.TrimBox().URY(), 300.0);
        EXPECT_DOUBLE_EQ(page.BleedBox().LLY(), 15.0);
        EXPECT_DOUBLE_EQ(page.ArtBox().URX(), 190.0);
    }
    std::filesystem::remove(out);
}
