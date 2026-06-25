// =============================================================================
// page_add_image_stream_smoke_test — beat F-3 of the images cluster. The
// canonical Page::AddImage(Stream, Rectangle, …) overload places an in-memory
// image (PNG / JPEG bytes) on the page and draws it; bbox clip and
// aspect-preserving autoAdjustRectangle are honoured.
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

// Page::AddImage(bytes, rect) embeds the image and draws it; the image
// survives a save/reload as a page resource.
TEST(PageAddImageStreamSmoke, PlacesImageFromBytes) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_addimg_stream.pdf")
            .string();
    const std::vector<std::byte> jpeg = ReadAllBytes(JpegFixture());
    ASSERT_GE(jpeg.size(), 2u);

    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        // Default args exercise the autoAdjustRectangle path.
        doc.Pages()[1].AddImage(jpeg, Rectangle{100.0, 100.0, 300.0, 300.0,
                                                true});
        doc.Save(out);
    }

    Document doc{out};
    XImageCollection& images = doc.Pages()[1].Resources().Images();
    ASSERT_EQ(images.Count(), 1);
    EXPECT_GT(images[1].Width(), 0);
    EXPECT_GT(images[1].Height(), 0);

    std::filesystem::remove(out);
}

// The bbox + autoAdjustRectangle overload arguments are accepted and the
// image still lands as a resource.
TEST(PageAddImageStreamSmoke, PlacesImageWithBboxClip) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_addimg_bbox.pdf")
            .string();
    const std::vector<std::byte> jpeg = ReadAllBytes(JpegFixture());
    ASSERT_GE(jpeg.size(), 2u);

    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        doc.Pages()[1].AddImage(jpeg, Rectangle{100.0, 100.0, 300.0, 300.0,
                                                true},
                                Rectangle{110.0, 110.0, 200.0, 200.0, true},
                                /*autoAdjustRectangle=*/false);
        doc.Save(out);
    }

    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Resources().Images().Count(), 1);

    std::filesystem::remove(out);
}

// Unsupported image bytes throw (canonical void overload signals failure by
// exception rather than a bool return).
TEST(PageAddImageStreamSmoke, ThrowsOnUnsupportedBytes) {
    Document doc{(FixtureRoot() / "hello_world.pdf").string()};
    const std::vector<std::byte> junk(32, std::byte{0x00});
    EXPECT_THROW(
        doc.Pages()[1].AddImage(junk, Rectangle{0.0, 0.0, 10.0, 10.0, true}),
        std::runtime_error);
}
