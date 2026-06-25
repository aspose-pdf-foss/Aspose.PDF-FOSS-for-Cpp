// =============================================================================
// bmp_device_smoke_test — BmpDevice ctor parameter round-trip checks.
// =============================================================================

#include <aspose/pdf/bmp_device.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

TEST(BmpDeviceSmoke, DefaultCtor) {
    Aspose::Pdf::Devices::BmpDevice device;
    EXPECT_EQ(device.Width(), 0);
}

TEST(BmpDeviceSmoke, ResolutionCtor_Roundtrip) {
    Aspose::Pdf::Devices::Resolution res(96);
    Aspose::Pdf::Devices::BmpDevice device(res);
    EXPECT_EQ(device.Resolution().X(), 96);
}

TEST(BmpDeviceSmoke, WidthHeightCtor_Roundtrip) {
    Aspose::Pdf::Devices::BmpDevice device(800, 600);
    EXPECT_EQ(device.Width(), 800);
    EXPECT_EQ(device.Height(), 600);
}

TEST(BmpDeviceSmoke, PageSizeAndResolution_Roundtrip) {
    Aspose::Pdf::Devices::Resolution res(150);
    Aspose::Pdf::Devices::BmpDevice device(
        Aspose::Pdf::PageSize::A5(), res);
    // Canonical Aspose.PDF 26.4.0 A5 = 421 × 595.
    EXPECT_EQ(device.Width(), 421);
    EXPECT_EQ(device.Height(), 595);
    EXPECT_EQ(device.Resolution().X(), 150);
}
