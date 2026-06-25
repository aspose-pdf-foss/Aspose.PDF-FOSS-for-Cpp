// Smoke test for Aspose::Pdf::Devices::TiffDevice.
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
std::filesystem::path FixtureDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}
bool LooksLikeTiff(const std::string& s) {
    if (s.size() < 4) return false;
    auto a = static_cast<unsigned char>(s[0]);
    auto b = static_cast<unsigned char>(s[1]);
    auto c = static_cast<unsigned char>(s[2]);
    auto d = static_cast<unsigned char>(s[3]);
    return (a == 0x49 && b == 0x49 && c == 0x2A && d == 0x00) ||
           (a == 0x4D && b == 0x4D && c == 0x00 && d == 0x2A);
}
}

int main() {
    using namespace Aspose::Pdf::Devices;
    Aspose::Pdf::Document doc(
        (FixtureDir() / "two_pages.pdf").string());

    // 1. Process(Page) — single page.
    TiffDevice tiff(Resolution(72));
    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);
    Check(LooksLikeTiff(sink.str()), "Process(Page) emits valid TIFF");

    // 2. Process(Document, fromPage, toPage) — multi-page.
    std::stringstream multi(
        std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc, 1, 2, multi);
    Check(LooksLikeTiff(multi.str()),
          "Process(Document range) emits valid TIFF");
    Check(multi.str().size() > sink.str().size(),
          "multi-page TIFF larger than single-page");

    // 3. Indexed-color path uses palette_quantiser.
    TiffSettings indexed;
    indexed.Depth(ColorDepth::Format8bpp);
    TiffDevice tiff8(Resolution(72), indexed);
    std::stringstream s8(
        std::ios::binary | std::ios::out | std::ios::in);
    tiff8.Process(doc.Pages()[1], s8);
    Check(LooksLikeTiff(s8.str()), "8bpp palettised TIFF emits");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
