// =============================================================================
// svg_import_smoke_test — beat S-1. Document(filename, SvgLoadOptions) imports
// an SVG as a single-page PDF sized to the SVG, rendering the basic vector
// primitives. (Text / transforms / curves / gradients are deferred.)
// =============================================================================

#include <filesystem>
#include <fstream>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/load_options.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/svg_load_options.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::string WriteTempSvg(const std::string& name, const std::string& body) {
    const std::string path =
        (std::filesystem::temp_directory_path() / name).string();
    std::ofstream f(path, std::ios::binary);
    f << body;
    return path;
}

}  // namespace

// An SVG with width/height imports as one page of that size, with shapes.
TEST(SvgImportSmoke, ImportsSvgWithWidthHeight) {
    const std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" "
        "height=\"150\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"40\" fill=\"#ff0000\"/>"
        "<circle cx=\"150\" cy=\"100\" r=\"30\" fill=\"blue\"/>"
        "<line x1=\"0\" y1=\"0\" x2=\"200\" y2=\"150\" stroke=\"black\"/>"
        "</svg>";
    const std::string path = WriteTempSvg("aspose_in.svg", svg);
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_svg_out.pdf").string();

    {
        Document doc{path, SvgLoadOptions{}};
        ASSERT_EQ(doc.Pages().Count(), 1);
        const Rectangle r = doc.Pages()[1].Rect();
        EXPECT_NEAR(r.Width(), 200.0, 1.0);
        EXPECT_NEAR(r.Height(), 150.0, 1.0);
        doc.Save(out);
    }
    // The produced PDF re-opens cleanly.
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 1);

    std::filesystem::remove(path);
    std::filesystem::remove(out);
}

// viewBox drives the page size when width/height are absent.
TEST(SvgImportSmoke, ViewBoxSizing) {
    const std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 300 100\">"
        "<polygon points=\"0,0 300,0 150,100\" fill=\"green\"/>"
        "<path d=\"M 10 10 L 90 10 L 90 90 Z\" fill=\"#0a0\"/>"
        "</svg>";
    const std::string path = WriteTempSvg("aspose_vb.svg", svg);

    Document doc{path, SvgLoadOptions{}};
    ASSERT_EQ(doc.Pages().Count(), 1);
    const Rectangle r = doc.Pages()[1].Rect();
    EXPECT_NEAR(r.Width(), 300.0, 1.0);
    EXPECT_NEAR(r.Height(), 100.0, 1.0);

    std::filesystem::remove(path);
}

// SvgLoadOptions / LoadOptions property surface.
TEST(SvgImportSmoke, LoadOptionsProperties) {
    SvgLoadOptions opts;
    EXPECT_FALSE(opts.AdjustPageSize());
    opts.AdjustPageSize(true);
    EXPECT_TRUE(opts.AdjustPageSize());

    EXPECT_FALSE(opts.DisableFontLicenseVerifications());  // inherited
    opts.DisableFontLicenseVerifications(true);
    EXPECT_TRUE(opts.DisableFontLicenseVerifications());
}
