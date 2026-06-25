// Smoke test for Aspose::Pdf::Devices::BmpDevice.
#include <aspose/pdf/bmp_device.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
std::filesystem::path FixtureDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}
}

int main() {
    using namespace Aspose::Pdf::Devices;
    Aspose::Pdf::Document doc(
        (FixtureDir() / "red_rect.pdf").string());

    BmpDevice bmp(Resolution(72));
    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    bmp.Process(doc.Pages()[1], sink);
    const auto bytes = sink.str();

    Check(bytes.size() >= 54u, "Process produced ≥ 54-byte BMP header + body");
    Check(bytes[0] == 'B' && bytes[1] == 'M', "output starts with 'BM'");

    std::uint32_t declared = 0;
    std::memcpy(&declared, bytes.data() + 2, 4);
    Check(declared == bytes.size(),
          "BITMAPFILEHEADER size matches actual byte count");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
