// =============================================================================
// Go-parity: image_test.go / image_add_integration_test.go /
// image_remove_integration_test.go → canonical C++ Page::AddImage +
// Page.Resources().Images() (XImageCollection).
//
// Ported (canonical): add image from file + from bytes (round-trip), extract /
// enumerate page images with dimensions, delete an image (round-trip).
// Skipped: Go-internal image-engine tests (DetectImageFormat, ParseJPEGHeader,
// Create*XObject, SerializeContentOps) and invented features (OptimizeImages,
// image convert, XImage replace).
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

std::filesystem::path TestsRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}

std::string HelloWorld() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (TestsRoot().parent_path() / "pdfs" / "hello_world.pdf").string();
}

std::string Jpeg() {
    return (TestsRoot() / "fixtures" / "dctdecode" / "gradient_32.jpg").string();
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

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_img_" + tag + ".pdf"))
        .string();
}

Rectangle R() { return Rectangle{100.0, 100.0, 300.0, 300.0, true}; }

}  // namespace

// TestAddImage — place an image from a file; it becomes a page image resource.
TEST(GoImage, AddImageFromFile) {
    const std::string out = TmpOut("file");
    {
        Document doc{HelloWorld()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(Jpeg(), R()));
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Pages()[1].Resources().Images().Count(), 1);
    std::filesystem::remove(out);
}

// TestAddImageFromStream — place an image from bytes.
TEST(GoImage, AddImageFromBytes) {
    const std::string out = TmpOut("bytes");
    {
        Document doc{HelloWorld()};
        doc.Pages()[1].AddImage(ReadAllBytes(Jpeg()), R());
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Pages()[1].Resources().Images().Count(), 1);
    std::filesystem::remove(out);
}

// TestExtractImages / TestImageInfos — enumerate images with dimensions.
TEST(GoImage, ExtractImagesWithDimensions) {
    const std::string out = TmpOut("extract");
    {
        Document doc{HelloWorld()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(Jpeg(), R()));
        doc.Save(out);
    }
    Document doc{out};
    XImageCollection& images = doc.Pages()[1].Resources().Images();
    ASSERT_GE(images.Count(), 1);
    EXPECT_GT(images[1].Width(), 0);
    EXPECT_GT(images[1].Height(), 0);
    std::filesystem::remove(out);
}

// TestRemoveImageRoundTrip — deleting an image persists through save.
TEST(GoImage, RemoveImageRoundTrip) {
    const std::string seeded = TmpOut("seed");
    const std::string out = TmpOut("removed");
    {
        Document doc{HelloWorld()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(Jpeg(), R()));
        doc.Save(seeded);
    }
    {
        Document doc{seeded};
        auto& images = doc.Pages()[1].Resources().Images();
        ASSERT_EQ(images.Count(), 1);
        images.Delete(1);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Resources().Images().Count(), 0);
    std::filesystem::remove(seeded);
    std::filesystem::remove(out);
}
