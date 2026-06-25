// Smoke test for Aspose::Pdf::Devices::PngDevice.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>

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
}

int main() {
    using namespace Aspose::Pdf::Devices;

    // 1. Property round-trip.
    PngDevice empty;
    empty.TransparentBackground(true);
    Check(empty.TransparentBackground(),
          "TransparentBackground round-trips");

    PngDevice sized(800, 600, Resolution(150, 150));
    Check(sized.Width() == 800 && sized.Height() == 600,
          "Width/Height ctor round-trips");
    Check(sized.Resolution().X() == 150,
          "Resolution ctor round-trips");

    // 2. End-to-end: Process(Page, ostream) emits a real PNG.
    Aspose::Pdf::Document doc(
        (FixtureDir() / "red_rect.pdf").string());
    PngDevice png(Resolution(72));
    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    png.Process(doc.Pages()[1], sink);
    const auto bytes = sink.str();
    Check(bytes.size() > 8u, "Process produced output");
    Check(static_cast<unsigned char>(bytes[0]) == 0x89 &&
          bytes.substr(1, 3) == "PNG",
          "output starts with PNG signature");
    Check(bytes.substr(bytes.size() - 8, 4) == "IEND",
          "output ends with IEND chunk");

    // 3. TransparentBackground plumbs through to canvas-init.
    // Two renders of the same page — opaque vs transparent —
    // must differ byte-wise because the canvas-init pixels
    // dominate areas not covered by page content.
    PngDevice opaque(Resolution(72));
    PngDevice trans(Resolution(72));
    trans.TransparentBackground(true);
    std::stringstream opaque_sink(
        std::ios::binary | std::ios::out | std::ios::in);
    std::stringstream trans_sink(
        std::ios::binary | std::ios::out | std::ios::in);
    opaque.Process(doc.Pages()[1], opaque_sink);
    trans.Process(doc.Pages()[1], trans_sink);
    Check(opaque_sink.str() != trans_sink.str(),
          "TransparentBackground passes through to canvas init");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
