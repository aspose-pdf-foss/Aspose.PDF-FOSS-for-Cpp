// Smoke test for Aspose::Pdf::Devices::JpegDevice.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/jpeg_device.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

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

    JpegDevice jpeg(Resolution(72), /*quality=*/85);
    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    jpeg.Process(doc.Pages()[1], sink);
    const auto bytes = sink.str();

    Check(bytes.size() >= 4u, "Process produced output");
    Check(static_cast<unsigned char>(bytes[0]) == 0xFFu &&
          static_cast<unsigned char>(bytes[1]) == 0xD8u,
          "output starts with JPEG SOI marker");
    Check(static_cast<unsigned char>(bytes[bytes.size() - 2]) == 0xFFu &&
          static_cast<unsigned char>(bytes[bytes.size() - 1]) == 0xD9u,
          "output ends with JPEG EOI marker");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
