// =============================================================================
// bmp_device_e2e_smoke_test — drives Process(Page, ostream) end-to-end
// against fixtures and asserts BMP magic bytes 'BM' + valid header.
// =============================================================================

#include <aspose/pdf/bmp_device.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>

namespace {
std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "page_renderer";
}
}

TEST(BmpDeviceE2E, RedRect_ProducesValidBmp) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::BmpDevice bmp(Aspose::Pdf::Devices::Resolution(72));
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    bmp.Process(page, sink);

    const std::string out = sink.str();
    ASSERT_GE(out.size(), 54u) << "BMP24 must be ≥ 14 + 40 byte header";

    // BITMAPFILEHEADER signature "BM".
    EXPECT_EQ(out[0], 'B');
    EXPECT_EQ(out[1], 'M');

    // File size at offset 2 must equal actual buffer size.
    std::uint32_t declared = 0;
    std::memcpy(&declared, out.data() + 2, 4);
    EXPECT_EQ(declared, out.size());

    // Pixel-data offset at byte 10 = 14 (file hdr) + 40 (info hdr) = 54.
    std::uint32_t pixel_offset = 0;
    std::memcpy(&pixel_offset, out.data() + 10, 4);
    EXPECT_EQ(pixel_offset, 54u);
}
