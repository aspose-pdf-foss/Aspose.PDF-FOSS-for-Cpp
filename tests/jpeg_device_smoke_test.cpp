// =============================================================================
// jpeg_device_smoke_test — JpegDevice ctor parameter round-trip checks
// (quality is a ctor parameter, not a property in the canonical surface).
// =============================================================================

#include <aspose/pdf/jpeg_device.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

TEST(JpegDeviceSmoke, DefaultCtor_DefaultGeometry) {
    Aspose::Pdf::Devices::JpegDevice device;
    EXPECT_EQ(device.Width(), 0);
    EXPECT_EQ(device.Height(), 0);
}

TEST(JpegDeviceSmoke, QualityCtor_DoesNotCrash) {
    Aspose::Pdf::Devices::JpegDevice device(85);
    EXPECT_EQ(device.Width(), 0);
    EXPECT_EQ(device.Height(), 0);
}

TEST(JpegDeviceSmoke, ResolutionAndQuality_Roundtrip) {
    Aspose::Pdf::Devices::Resolution res(150, 150);
    Aspose::Pdf::Devices::JpegDevice device(res, 92);
    EXPECT_EQ(device.Resolution().X(), 150);
    EXPECT_EQ(device.Resolution().Y(), 150);
}

TEST(JpegDeviceSmoke, WidthHeightResolutionQuality_Roundtrip) {
    Aspose::Pdf::Devices::Resolution res(96, 96);
    Aspose::Pdf::Devices::JpegDevice device(640, 480, res, 70);
    EXPECT_EQ(device.Width(), 640);
    EXPECT_EQ(device.Height(), 480);
}

TEST(JpegDeviceSmoke, PageSizeAndQuality_Roundtrip) {
    Aspose::Pdf::Devices::Resolution res(72);
    Aspose::Pdf::Devices::JpegDevice device(
        Aspose::Pdf::PageSize::PageLetter(), res, 80);
    EXPECT_EQ(device.Width(), 612);
    EXPECT_EQ(device.Height(), 792);
}
