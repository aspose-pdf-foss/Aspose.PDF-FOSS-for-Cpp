// =============================================================================
// x_image_replace_smoke_test — XImageCollection::Replace. Replacing a loaded
// image's content keeps its resource name but swaps the XObject: after save,
// the image's dimensions and bytes reflect the replacement.
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
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}
std::string Jpeg32() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg")
        .string();
}
std::string Png16() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "png_decoder" / "rgb_checker_16.png")
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

TEST(XImageReplaceSmoke, ReplaceSwapsImageContent) {
    const std::string seeded =
        (std::filesystem::temp_directory_path() / "asp_repl_seed.pdf").string();
    const std::string out =
        (std::filesystem::temp_directory_path() / "asp_repl_out.pdf").string();

    // Seed: a 32x32 JPEG (DCTDecode → raw bytes start with the SOI marker).
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(
            Jpeg32(), Rectangle{100.0, 100.0, 300.0, 300.0, true}));
        doc.Save(seeded);
    }
    {
        Document doc{seeded};
        auto& images = doc.Pages()[1].Resources().Images();
        ASSERT_EQ(images.Count(), 1);
        EXPECT_EQ(images[1].Width(), 32);
        const auto raw = images[1].GetRawImageData();
        ASSERT_GE(raw.size(), 2u);
        EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0xFF);  // JPEG SOI

        // Replace with a 16x16 PNG.
        images.Replace(1, ReadAllBytes(Png16()));
        doc.Save(out);
    }

    // After reload the (same, single) image is now 16x16 and no longer JPEG.
    Document doc{out};
    auto& images = doc.Pages()[1].Resources().Images();
    ASSERT_EQ(images.Count(), 1);
    EXPECT_EQ(images[1].Width(), 16);
    EXPECT_EQ(images[1].Height(), 16);
    const auto raw = images[1].GetRawImageData();
    ASSERT_GE(raw.size(), 2u);
    EXPECT_NE(static_cast<unsigned char>(raw[0]), 0xFF);  // not JPEG anymore

    std::filesystem::remove(seeded);
    std::filesystem::remove(out);
}

TEST(XImageReplaceSmoke, ReplaceInvalidIndexThrows) {
    Document doc{(FixtureRoot() / "hello_world.pdf").string()};
    auto& images = doc.Pages()[1].Resources().Images();
    EXPECT_THROW(images.Replace(1, ReadAllBytes(Png16())), std::out_of_range);
}
