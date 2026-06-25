// =============================================================================
// x_image_embed_smoke_test — beat F-2 of the images cluster. Image embedding
// (XImageCollection::Add) and deletion persistence: images staged with Add are
// written as new /Resources /XObject entries at Save, and images removed from a
// loaded collection are dropped from the saved PDF (XObject + `Do` operator).
// =============================================================================

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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

// XImageCollection::Add stages a JPEG; Save embeds it as a new image XObject
// that survives a round-trip.
TEST(XImageEmbedSmoke, AddPersistsThroughSave) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_ximage_add.pdf")
            .string();
    const std::vector<std::byte> jpeg = ReadAllBytes(JpegFixture());
    ASSERT_GE(jpeg.size(), 2u);

    std::string added_name;
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        XImageCollection& images = doc.Pages()[1].Resources().Images();
        ASSERT_EQ(images.Count(), 0);
        added_name = images.Add(jpeg);
        EXPECT_FALSE(added_name.empty());
        // The add is visible immediately on the live collection.
        EXPECT_EQ(images.Count(), 1);
        EXPECT_EQ(images[added_name].Name(), added_name);
        doc.Save(out);
    }

    Document doc{out};
    XImageCollection& images = doc.Pages()[1].Resources().Images();
    ASSERT_EQ(images.Count(), 1);
    XImage& img = images[added_name];
    EXPECT_EQ(img.Name(), added_name);
    EXPECT_GT(img.Width(), 0);
    EXPECT_GT(img.Height(), 0);
    // JPEG passes through as DCTDecode → raw bytes keep the SOI marker.
    const std::vector<std::byte> raw = img.GetRawImageData();
    ASSERT_GE(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0xFF);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0xD8);

    std::filesystem::remove(out);
}

// Deleting a loaded image removes it from the saved PDF, not just the
// in-memory collection.
TEST(XImageEmbedSmoke, DeletePersistsThroughSave) {
    const std::string with_image =
        (std::filesystem::temp_directory_path() / "aspose_ximage_seed.pdf")
            .string();
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_ximage_del.pdf")
            .string();

    // Seed a document that actually contains one embedded image.
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(
            JpegFixture(), Rectangle{100.0, 100.0, 300.0, 300.0, true}));
        doc.Save(with_image);
    }

    // Delete the loaded image and save.
    {
        Document doc{with_image};
        XImageCollection& images = doc.Pages()[1].Resources().Images();
        ASSERT_EQ(images.Count(), 1);
        images.Delete(1);
        EXPECT_EQ(images.Count(), 0);
        doc.Save(out);
    }

    // The deletion persisted: the reloaded page has no images.
    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Resources().Images().Count(), 0);

    std::filesystem::remove(with_image);
    std::filesystem::remove(out);
}
