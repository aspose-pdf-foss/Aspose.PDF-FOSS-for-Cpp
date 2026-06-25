// =============================================================================
// watermark_artifact_smoke_test — beat T-5 of the text cluster. A
// WatermarkArtifact (text or image) added through Page::Artifacts() renders
// into the page content on Save, honouring Opacity / Rotation / IsBackground.
// =============================================================================

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/artifact.hpp>
#include <aspose/pdf/artifact_collection.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/watermark_artifact.hpp>
#include <aspose/pdf/x_image_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextFragmentAbsorber;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string JpegFixture() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg")
        .string();
}

std::vector<std::byte> ReadAllBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    const std::string raw((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<std::byte> out(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i)
        out[i] = static_cast<std::byte>(static_cast<unsigned char>(raw[i]));
    return out;
}

}  // namespace

// A text WatermarkArtifact renders as page text that survives a round-trip.
TEST(WatermarkArtifactSmoke, TextWatermarkRendersOnSave) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_wm_text.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        WatermarkArtifact wm;  // must outlive Save (Add stores a reference)
        Text::TextState ts;
        ts.FontSize(36.0f);
        wm.SetTextAndState("CONFIDENTIAL", ts);
        wm.Position(Point{150.0, 400.0});
        wm.Rotation(45.0);
        wm.Opacity(0.4);
        wm.IsBackground(true);

        Page page = doc.Pages()[1];
        page.Artifacts().Add(wm);
        EXPECT_EQ(page.Artifacts().Count(), 1);
        EXPECT_EQ(&page.Artifacts()[1], &wm);  // 1-based indexer
        doc.Save(out);
    }

    // The watermark text is now present on the page.
    Document doc{out};
    TextFragmentAbsorber absorber{"CONFIDENTIAL"};
    absorber.Visit(doc);
    EXPECT_GE(absorber.TextFragments().Count(), 1);

    std::filesystem::remove(out);
}

// An image WatermarkArtifact embeds an image XObject on the page.
TEST(WatermarkArtifactSmoke, ImageWatermarkEmbedsXObject) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_wm_img.pdf").string();
    const std::vector<std::byte> jpeg = ReadAllBytes(JpegFixture());
    ASSERT_GE(jpeg.size(), 2u);
    {
        Document doc{HelloWorldPdf()};
        WatermarkArtifact wm;
        wm.SetImage(jpeg);
        wm.Position(Point{100.0, 200.0});
        wm.Opacity(0.5);

        Page page = doc.Pages()[1];
        page.Artifacts().Add(wm);
        doc.Save(out);
    }

    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Resources().Images().Count(), 1);

    std::filesystem::remove(out);
}

// Collection semantics: Add / Count / Delete.
TEST(WatermarkArtifactSmoke, CollectionAddAndDelete) {
    Document doc{HelloWorldPdf()};
    WatermarkArtifact a;
    WatermarkArtifact b;
    Page page = doc.Pages()[1];
    auto& artifacts = page.Artifacts();
    artifacts.Add(a);
    artifacts.Add(b);
    EXPECT_EQ(artifacts.Count(), 2);
    artifacts.Delete(1);
    EXPECT_EQ(artifacts.Count(), 1);
    EXPECT_EQ(&artifacts[1], &b);
    EXPECT_THROW(artifacts[2], std::out_of_range);
}

// Default Artifact property values.
TEST(WatermarkArtifactSmoke, DefaultProperties) {
    WatermarkArtifact wm;
    EXPECT_DOUBLE_EQ(wm.Opacity(), 1.0);
    EXPECT_DOUBLE_EQ(wm.Rotation(), 0.0);
    EXPECT_FALSE(wm.IsBackground());
    EXPECT_TRUE(wm.Text().empty());

    wm.IsBackground(true);
    wm.Rotation(90.0);
    EXPECT_TRUE(wm.IsBackground());
    EXPECT_DOUBLE_EQ(wm.Rotation(), 90.0);
}
