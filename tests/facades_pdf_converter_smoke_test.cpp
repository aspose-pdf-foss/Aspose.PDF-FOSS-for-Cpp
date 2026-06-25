// =============================================================================
// facades_pdf_converter_smoke_test — beat Fa12 of the Facades cluster.
// PdfConverter rasterises a PDF to images / TIFF. v1 wires the
// file-output conversion through the REAL rendering pipeline: SaveAsTIFF
// → multi-page TIFF (Devices::TiffDevice); HasNextImage/GetNextImage →
// per-page PNG (Devices::PngDevice); PageCount → real page count.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_converter.hpp>
#include <aspose/pdf/page_coordinate_type.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_pdfconv_" + name)).string();
}

std::string Head(const std::string& path, std::size_t n) {
    std::ifstream f(path, std::ios::binary);
    std::string buf(n, '\0');
    f.read(buf.data(), static_cast<std::streamsize>(n));
    buf.resize(static_cast<std::size_t>(f.gcount()));
    return buf;
}

}  // namespace

TEST(FacadesPdfConverterSmoke, PageCountReal) {
    Document doc{HelloWorldPdf()};
    PdfConverter conv{doc};
    EXPECT_EQ(conv.PageCount(), static_cast<int>(doc.Pages().Count()));
    EXPECT_GT(conv.PageCount(), 0);
}

TEST(FacadesPdfConverterSmoke, UnboundPageCountZero) {
    PdfConverter conv;
    EXPECT_EQ(conv.PageCount(), 0);
    EXPECT_FALSE(conv.HasNextImage());
}

TEST(FacadesPdfConverterSmoke, SaveAsTIFFRendersRealFile) {
    Document doc{HelloWorldPdf()};
    PdfConverter conv{doc};
    const std::string out = TempPath("all.tiff");
    conv.SaveAsTIFF(out);
    ASSERT_TRUE(std::filesystem::exists(out));
    EXPECT_GT(std::filesystem::file_size(out), 0u);
    // TIFF magic: "II" (little-endian) or "MM" (big-endian).
    const std::string head = Head(out, 2);
    EXPECT_TRUE(head == "II" || head == "MM") << "head=" << head;
    std::filesystem::remove(out);
}

TEST(FacadesPdfConverterSmoke, SaveAsTIFFWithCompression) {
    Document doc{HelloWorldPdf()};
    PdfConverter conv{doc};
    const std::string out = TempPath("lzw.tiff");
    conv.SaveAsTIFF(out, Aspose::Pdf::Devices::CompressionType::LZW);
    ASSERT_TRUE(std::filesystem::exists(out));
    EXPECT_GT(std::filesystem::file_size(out), 0u);
    std::filesystem::remove(out);
}

TEST(FacadesPdfConverterSmoke, GetNextImageRendersPng) {
    Document doc{HelloWorldPdf()};
    PdfConverter conv{doc};
    conv.DoConvert();

    int rendered = 0;
    while (conv.HasNextImage()) {
        const std::string out = TempPath("page" + std::to_string(rendered) +
                                         ".png");
        conv.GetNextImage(out);
        ASSERT_TRUE(std::filesystem::exists(out));
        // PNG signature 0x89 'P' 'N' 'G'.
        const std::string head = Head(out, 4);
        ASSERT_GE(head.size(), 4u);
        EXPECT_EQ(static_cast<unsigned char>(head[0]), 0x89);
        EXPECT_EQ(head.substr(1, 3), "PNG");
        std::filesystem::remove(out);
        ++rendered;
    }
    EXPECT_EQ(rendered, static_cast<int>(doc.Pages().Count()));
}

TEST(FacadesPdfConverterSmoke, PropertyRoundtrip) {
    PdfConverter conv;
    conv.StartPage(2);
    conv.EndPage(5);
    conv.ShowHiddenAreas(true);
    conv.CoordinateType(PageCoordinateType::MediaBox);
    conv.Password("pw");
    conv.UserPassword("upw");
    conv.Resolution(Aspose::Pdf::Devices::Resolution(300));

    EXPECT_EQ(conv.StartPage(), 2);
    EXPECT_EQ(conv.EndPage(), 5);
    EXPECT_TRUE(conv.ShowHiddenAreas());
    EXPECT_EQ(conv.CoordinateType(), PageCoordinateType::MediaBox);
    EXPECT_EQ(conv.Password(), "pw");
    EXPECT_EQ(conv.UserPassword(), "upw");
}
