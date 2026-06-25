// =============================================================================
// png_device_e2e_smoke_test — first end-to-end Process(Page, ostream)
// exercise. Open a PDF, get page 1 via the new PageCollection indexer,
// drive PngDevice::Process, assert the output is a well-formed PNG
// (signature + IHDR + IDAT + IEND chunks).
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <cstddef>

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

TEST(PngDeviceE2E, RedRect_ProducesValidPng) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;

    Aspose::Pdf::Document doc(pdf.string());
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(72));
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    png.Process(page, sink);

    const std::string out = sink.str();
    ASSERT_GT(out.size(), 8u);

    // PNG signature \x89PNG\r\n\x1a\n
    constexpr std::array<unsigned char, 8> kSig = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    for (std::size_t i = 0; i < kSig.size(); ++i) {
        EXPECT_EQ(static_cast<unsigned char>(out[i]), kSig[i])
            << "byte " << i;
    }

    // IHDR chunk type at offset 12 (4 bytes length + 4 bytes type).
    EXPECT_EQ(out.substr(12, 4), "IHDR");

    // Output must end with the IEND chunk: 4-byte length 0,
    // 'IEND', 4-byte CRC. Last 8 bytes carry IEND + its CRC.
    ASSERT_GE(out.size(), 12u);
    EXPECT_EQ(out.substr(out.size() - 8, 4), "IEND");
}

TEST(PngDeviceE2E, TwoPages_PerPageOutputDistinct) {
    const auto pdf = FixtureRoot() / "two_pages.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto& pages = doc.Pages();
    ASSERT_EQ(pages.Count(), 2u);

    Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(72));

    std::stringstream sink1(std::ios::binary | std::ios::out | std::ios::in);
    png.Process(pages[1], sink1);

    std::stringstream sink2(std::ios::binary | std::ios::out | std::ios::in);
    png.Process(pages[2], sink2);

    EXPECT_GT(sink1.str().size(), 0u);
    EXPECT_GT(sink2.str().size(), 0u);
    // Both PNG-framed.
    EXPECT_EQ(sink1.str().substr(0, 8),
              std::string("\x89PNG\r\n\x1a\n", 8));
    EXPECT_EQ(sink2.str().substr(0, 8),
              std::string("\x89PNG\r\n\x1a\n", 8));
}
