// Smoke test for Aspose::Pdf::IIndexBitmapConverter +
// Aspose::Pdf::BitmapInfo. Confirms the interface compiles and a
// custom subclass dispatches through TiffDevice as expected.
#include <aspose/pdf/bitmap_info.hpp>
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/i_index_bitmap_converter.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include <cstddef>
#include <utility>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
std::filesystem::path FixtureDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

class CountingConverter : public Aspose::Pdf::IIndexBitmapConverter {
public:
    int n1 = 0, n4 = 0, n8 = 0;
    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get1BppImage(const Aspose::Pdf::BitmapInfo& s) override {
        ++n1; return Stub(s);
    }
    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get4BppImage(const Aspose::Pdf::BitmapInfo& s) override {
        ++n4; return Stub(s);
    }
    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get8BppImage(const Aspose::Pdf::BitmapInfo& s) override {
        ++n8; return Stub(s);
    }
private:
    static std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Stub(const Aspose::Pdf::BitmapInfo& s) {
        // Two-colour quantised output: black (0,0,0,255) for even
        // pixels, white (255,255,255,255) for odd. Within the
        // 1bpp/4bpp/8bpp contract (≤ 2 distinct RGB colours).
        const auto n = static_cast<std::size_t>(s.Width()) * s.Height();
        std::vector<std::uint8_t> rgba(n * 4, 0u);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t v =
                (i & 1u) ? std::uint8_t{255} : std::uint8_t{0};
            rgba[i * 4 + 0] = v;
            rgba[i * 4 + 1] = v;
            rgba[i * 4 + 2] = v;
            rgba[i * 4 + 3] = 0xFFu;
        }
        return std::make_unique<Aspose::Pdf::BitmapInfo>(
            std::move(rgba), s.Width(), s.Height(),
            Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
    }
};
}

int main() {
    using namespace Aspose::Pdf;

    // BitmapInfo shape round-trip.
    BitmapInfo rgba(std::vector<std::uint8_t>(16, 0xAA), 2, 2,
                    BitmapInfo::PixelFormat::Rgba32);
    Check(rgba.Width() == 2, "BitmapInfo Width");
    Check(rgba.Height() == 2, "BitmapInfo Height");
    Check(rgba.Format() == BitmapInfo::PixelFormat::Rgba32,
          "BitmapInfo Format");

    // End-to-end: TiffDevice with custom converter routes 8bpp.
    Document doc((FixtureDir() / "red_rect.pdf").string());
    CountingConverter conv;
    Devices::TiffSettings settings;
    settings.Depth(Devices::ColorDepth::Format8bpp);
    Devices::TiffDevice tiff(
        Devices::Resolution(72), settings, conv);

    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);

    Check(conv.n8 == 1, "Format8bpp dispatched to Get8BppImage");
    Check(conv.n4 == 0, "no Get4BppImage call");
    Check(conv.n1 == 0, "no Get1BppImage call");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
