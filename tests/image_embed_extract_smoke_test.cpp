// =============================================================================
// image_embed_extract_smoke_test — image embed (Page::AddImage) + extract
// (PdfExtractor) — parity gap 4. AddImage embeds a JPEG (DCTDecode
// passthrough) or PNG (decode → FlateDecode) as a page /XObject and
// draws it; PdfExtractor walks image XObjects and writes them out. Closes
// pdflib's image embed/extract parity gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path Root() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}
std::string HelloWorldPdf() {
    return (Root() / "pdfs" / "hello_world.pdf").string();
}
std::string JpegFixture() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg").string();
}
std::string PngFixture() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "png_decoder" / "rgb_checker_16.png").string();
}
std::string Tmp(const std::string& n) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_img_" + n)).string();
}
std::vector<char> ReadBytes(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(f),
                             std::istreambuf_iterator<char>());
}

}  // namespace

TEST(ImageEmbedExtractSmoke, EmbedJpegExtractRoundTrip) {
    const std::vector<char> original = ReadBytes(JpegFixture());
    ASSERT_FALSE(original.empty());
    const std::string out = Tmp("embed_jpg.pdf");
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        ASSERT_TRUE(page.AddImage(JpegFixture(),
                                  Rectangle(100, 100, 300, 300, true)));
        doc.Save(out);
    }
    Document re{out};
    PdfExtractor ex{re};
    ex.ExtractImage();
    ASSERT_TRUE(ex.HasNextImage());
    const std::string imgOut = Tmp("extracted.jpg");
    ASSERT_TRUE(ex.GetNextImage(imgOut));
    EXPECT_FALSE(ex.HasNextImage());  // only one image
    // DCTDecode passthrough → extracted bytes identical to the original.
    EXPECT_EQ(ReadBytes(imgOut), original);
    std::filesystem::remove(out);
    std::filesystem::remove(imgOut);
}

TEST(ImageEmbedExtractSmoke, EmbedPngProducesAnImage) {
    const std::string out = Tmp("embed_png.pdf");
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        ASSERT_TRUE(page.AddImage(PngFixture(),
                                  Rectangle(50, 50, 250, 250, true)));
        doc.Save(out);
    }
    Document re{out};
    PdfExtractor ex{re};
    ex.ExtractImage();
    EXPECT_TRUE(ex.HasNextImage());
    std::filesystem::remove(out);
}

TEST(ImageEmbedExtractSmoke, NoImagesInUnstampedDoc) {
    Document doc{HelloWorldPdf()};
    PdfExtractor ex{doc};
    ex.ExtractImage();
    EXPECT_FALSE(ex.HasNextImage());
}

TEST(ImageEmbedExtractSmoke, AddImageBadPathReturnsFalse) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    EXPECT_FALSE(page.AddImage("/no/such/image.jpg",
                               Rectangle(0, 0, 10, 10, true)));
}
