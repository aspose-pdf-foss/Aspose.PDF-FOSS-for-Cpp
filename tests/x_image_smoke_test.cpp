// =============================================================================
// x_image_smoke_test — beat F-1 of the images cluster. Page.Resources().Images()
// enumerates a page's embedded image XObjects as XImage objects (name,
// dimensions, raw bytes), supports lookup by index/name and removal.
// =============================================================================

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/x_image.hpp>
#include <aspose/pdf/x_image_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string JpegFixture() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg")
        .string();
}

}  // namespace

TEST(XImageSmoke, EnumerateAndExtract) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_ximage.pdf").string();

    // Embed a JPEG, then enumerate the page's images.
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(
            JpegFixture(), Rectangle{100.0, 100.0, 300.0, 300.0, true}));
        doc.Save(out);
    }

    Document doc{out};
    XImageCollection& images = doc.Pages()[1].Resources().Images();
    ASSERT_EQ(images.Count(), 1);

    XImage& img = images[1];
    EXPECT_GT(img.Width(), 0);
    EXPECT_GT(img.Height(), 0);
    EXPECT_FALSE(img.Name().empty());

    // GetRawImageData returns the embedded JPEG bytes (SOI marker FF D8).
    const std::vector<std::byte> raw = img.GetRawImageData();
    ASSERT_GE(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0xFF);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0xD8);

    // Lookup by name + the collection name list.
    EXPECT_EQ(&images[img.Name()], &img);
    EXPECT_EQ(images.Names().size(), 1u);

    // Remove drops it from the (in-memory) collection.
    images.Delete(1);
    EXPECT_EQ(images.Count(), 0);

    std::filesystem::remove(out);
}
