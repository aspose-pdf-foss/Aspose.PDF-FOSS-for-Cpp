// =============================================================================
// page_geometry_smoke_test — Page geometry (Rect / MediaBox / CropBox /
// Rotate / GetPageRect / SetPageSize). Reads resolve from the parsed
// page tree; writes stage and persist through Document::Save via an
// incremental /MediaBox / /Rotate rewrite. Closes the pdflib
// Page::resize / size / rotation parity gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
           "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_pagegeom_" + name)).string();
}

}  // namespace

TEST(PageGeometrySmoke, ReadDefaults) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Width()), 612.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Height()), 792.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(page.MediaBox().Width()), 612.0f);
    EXPECT_EQ(page.Rotate(), Rotation::None);
    EXPECT_FLOAT_EQ(static_cast<float>(page.GetPageRect(true).Height()),
                    792.0f);
    // CropBox defaults to MediaBox.
    EXPECT_FLOAT_EQ(static_cast<float>(page.CropBox().Width()), 612.0f);
}

TEST(PageGeometrySmoke, SetGetInSession) {
    Document doc{HelloWorldPdf()};
    doc.Pages()[1].SetPageSize(300.0, 400.0);
    auto page = doc.Pages()[1];
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Width()), 300.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Height()), 400.0f);

    doc.Pages()[1].Rect(Rectangle(10.0, 20.0, 210.0, 320.0, false));
    auto p2 = doc.Pages()[1];
    EXPECT_FLOAT_EQ(static_cast<float>(p2.Rect().LLX()), 10.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(p2.Rect().Width()), 200.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(p2.Rect().Height()), 300.0f);
}

TEST(PageGeometrySmoke, SetPageSizeSaveReload) {
    const std::string out = TempPath("resize.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.Pages()[1].SetPageSize(360.0, 480.0);
        doc.Save(out);
    }
    ASSERT_TRUE(std::filesystem::exists(out));
    Document re{out};
    auto page = re.Pages()[1];
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Width()), 360.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(page.Rect().Height()), 480.0f);
    std::filesystem::remove(out);
}

TEST(PageGeometrySmoke, RotateSaveReload) {
    const std::string out = TempPath("rotate.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.Pages()[1].Rotate(Rotation::on90);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Pages()[1].Rotate(), Rotation::on90);
    std::filesystem::remove(out);
}

TEST(PageGeometrySmoke, NoEditSavesVerbatim) {
    // No geometry edit → Save short-circuits to a verbatim copy.
    const std::string out = TempPath("verbatim.pdf");
    Document doc{HelloWorldPdf()};
    doc.Save(out);
    EXPECT_TRUE(std::filesystem::exists(out));
    std::filesystem::remove(out);
}
