// =============================================================================
// jpeg_device_e2e_smoke_test — drives Process(Page, ostream) end-to-end
// against fixtures and asserts JPEG magic bytes (SOI / EOI markers).
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/jpeg_device.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
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

TEST(JpegDeviceE2E, RedRect_ProducesValidJpeg) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::JpegDevice jpeg(
        Aspose::Pdf::Devices::Resolution(72), /*quality=*/85);
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    jpeg.Process(page, sink);

    const std::string out = sink.str();
    ASSERT_GE(out.size(), 4u);
    // SOI marker FF D8 at start, EOI marker FF D9 at end.
    EXPECT_EQ(static_cast<unsigned char>(out[0]), 0xFFu);
    EXPECT_EQ(static_cast<unsigned char>(out[1]), 0xD8u);
    EXPECT_EQ(static_cast<unsigned char>(out[out.size() - 2]), 0xFFu);
    EXPECT_EQ(static_cast<unsigned char>(out[out.size() - 1]), 0xD9u);
}

TEST(JpegDeviceE2E, QualityAffectsSize) {
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());

    auto run = [&](int q) {
        Aspose::Pdf::Devices::JpegDevice jpeg(
            Aspose::Pdf::Devices::Resolution(72), q);
        std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
        jpeg.Process(doc.Pages()[1], sink);
        return sink.str().size();
    };

    const auto low = run(20);
    const auto high = run(95);
    EXPECT_GT(high, low)
        << "higher quality JPEG should produce more bytes ("
        << "q20=" << low << " B, q95=" << high << " B)";
}
