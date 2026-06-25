// =============================================================================
// png_device_smoke_test — verify PngDevice ctors carry their parameters
// through to ImageDevice properties and that TransparentBackground
// round-trips. End-to-end Process() exercising the encoder waits for a
// PageCollection per-page accessor (follow-up beat — canonical Aspose
// has the indexer, our v1 lib doesn't expose it yet).
// =============================================================================

#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

TEST(PngDeviceSmoke, DefaultCtor_DefaultProperties) {
    Aspose::Pdf::Devices::PngDevice device;
    EXPECT_EQ(device.Width(), 0);
    EXPECT_EQ(device.Height(), 0);
    EXPECT_FALSE(device.TransparentBackground());
}

TEST(PngDeviceSmoke, ResolutionCtor_ResolutionRoundtrip) {
    Aspose::Pdf::Devices::Resolution res(150);
    Aspose::Pdf::Devices::PngDevice device(res);
    EXPECT_EQ(device.Resolution().X(), 150);
    EXPECT_EQ(device.Resolution().Y(), 150);
}

TEST(PngDeviceSmoke, WidthHeightResolutionCtor_AllRoundtrip) {
    Aspose::Pdf::Devices::Resolution res(200, 300);
    Aspose::Pdf::Devices::PngDevice device(800, 600, res);
    EXPECT_EQ(device.Width(), 800);
    EXPECT_EQ(device.Height(), 600);
    EXPECT_EQ(device.Resolution().X(), 200);
    EXPECT_EQ(device.Resolution().Y(), 300);
}

TEST(PngDeviceSmoke, PageSizeCtor_DimensionsRoundtrip) {
    Aspose::Pdf::Devices::PngDevice device(Aspose::Pdf::PageSize::A4());
    EXPECT_EQ(device.Width(), 595);
    EXPECT_EQ(device.Height(), 842);
}

TEST(PngDeviceSmoke, TransparentBackground_RoundTrips) {
    Aspose::Pdf::Devices::PngDevice device;
    device.TransparentBackground(true);
    EXPECT_TRUE(device.TransparentBackground());
    device.TransparentBackground(false);
    EXPECT_FALSE(device.TransparentBackground());
}
