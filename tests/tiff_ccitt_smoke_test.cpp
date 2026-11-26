// =============================================================================
// tiff_ccitt_smoke_test — TiffDevice with CompressionType::CCITT3 / CCITT4:
// the bi-level page decodes (foundation::tiff_decoder, which now reads CCITT)
// to exactly the pixels of the uncompressed 1bpp rendering, at any colour
// depth setting; and, where Python's Pillow (libtiff) is available, an
// independent decoder reads the same pixels.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include "tiff_decoder.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Devices;

std::string Tiff(CompressionType compression, ColorDepth depth) {
    const auto pdf = std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs" /
                     "text_hello.pdf";
    Document doc(pdf.string());
    TiffSettings settings(compression);
    settings.Depth(depth);
    TiffDevice tiff(Resolution(96), settings);
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);
    return sink.str();
}

foundation::tiff_decoder::DecodedImage Decode(const std::string& bytes) {
    return foundation::tiff_decoder::Decode(
        {reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()});
}

int Black(const foundation::tiff_decoder::DecodedImage& img) {
    int n = 0;
    for (std::size_t i = 0; i < img.pixels.size(); i += 4)
        if (img.pixels[i] == std::byte{0}) ++n;
    return n;
}

}  // namespace

TEST(TiffCcitt, Group4AndGroup3MatchUncompressed1bpp) {
    const auto reference = Decode(Tiff(CompressionType::None, ColorDepth::Format1bpp));
    ASSERT_GT(Black(reference), 0);  // the page has text
    for (auto c : {CompressionType::CCITT4, CompressionType::CCITT3}) {
        const std::string bytes = Tiff(c, ColorDepth::Format1bpp);
        const auto img = Decode(bytes);
        EXPECT_EQ(img.width, reference.width);
        EXPECT_EQ(img.height, reference.height);
        EXPECT_TRUE(img.pixels == reference.pixels)
            << static_cast<int>(c) << " black " << Black(img) << " vs " << Black(reference)
            << " of " << img.width * img.height;
        // Much smaller than the raw 1bpp strip.
        EXPECT_LT(bytes.size(), reference.width * reference.height / 8 / 2);
    }
}

TEST(TiffCcitt, AnyDepthIsWrittenBilevel) {
    const auto img = Decode(Tiff(CompressionType::CCITT4, ColorDepth::Default));
    EXPECT_GT(Black(img), 0);
}

// An independent decoder (libtiff through Pillow) reads our CCITT files.
TEST(TiffCcitt, PillowReadsCcittTiffs) {
    if (std::system("python3 -c \"import PIL\" >/dev/null 2>&1") != 0)
        GTEST_SKIP() << "python3 with Pillow not available";
    const auto reference = Decode(Tiff(CompressionType::None, ColorDepth::Format1bpp));
    for (auto c : {CompressionType::CCITT4, CompressionType::CCITT3}) {
        const auto path = (std::filesystem::temp_directory_path() /
                           ("asp_ccitt_" + std::to_string(static_cast<int>(c)) + ".tif"))
                              .string();
        const std::string bytes = Tiff(c, ColorDepth::Format1bpp);
        std::ofstream(path, std::ios::binary).write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        // Pillow prints the number of black pixels.
        const std::string cmd =
            "python3 -c \"from PIL import Image; im = Image.open('" + path +
            "').convert('L'); print(sum(1 for p in im.getdata() if p == 0))\" > '" + path +
            ".txt'";
        ASSERT_EQ(std::system(cmd.c_str()), 0);
        std::ifstream in(path + ".txt");
        int black = -1;
        in >> black;
        EXPECT_EQ(black, Black(reference)) << static_cast<int>(c);
    }
}

// CCITT TIFFs written by libtiff (ImageMagick; see
// tests/fixtures/tiff_decoder/generate_ccitt.sh): left half white, right black.
TEST(TiffCcitt, DecodesLibtiffFiles) {
    for (const char* name : {"ccitt_g4_libtiff.tif", "ccitt_g3_libtiff.tif"}) {
        const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" /
                          "tiff_decoder" / name;
        std::ifstream in(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(in)), {});
        const auto img = Decode(bytes);
        ASSERT_EQ(img.width, 64u) << name;
        ASSERT_EQ(img.height, 32u) << name;
        auto red = [&](int x, int y) {
            return std::to_integer<int>(img.pixels[(static_cast<std::size_t>(y) * 64 + x) * 4]);
        };
        EXPECT_EQ(red(8, 16), 255) << name;
        EXPECT_EQ(red(56, 16), 0) << name;
    }
}
