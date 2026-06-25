// =============================================================================
// tiff_settings_smoke_test — Margins + TiffSettings ctor / property
// round-trip checks.
// =============================================================================

#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/margins.hpp>
#include <aspose/pdf/page_coordinate_type.hpp>
#include <aspose/pdf/shape_type.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <gtest/gtest.h>

TEST(MarginsSmoke, DefaultZero) {
    Aspose::Pdf::Devices::Margins m;
    EXPECT_EQ(m.Left(),   0);
    EXPECT_EQ(m.Right(),  0);
    EXPECT_EQ(m.Top(),    0);
    EXPECT_EQ(m.Bottom(), 0);
}

TEST(MarginsSmoke, FourArgCtor_Roundtrip) {
    Aspose::Pdf::Devices::Margins m(10, 20, 30, 40);
    EXPECT_EQ(m.Left(),   10);
    EXPECT_EQ(m.Right(),  20);
    EXPECT_EQ(m.Top(),    30);
    EXPECT_EQ(m.Bottom(), 40);
}

TEST(MarginsSmoke, Setters_Override) {
    Aspose::Pdf::Devices::Margins m;
    m.Left(1);
    m.Right(2);
    m.Top(3);
    m.Bottom(4);
    EXPECT_EQ(m.Left(),   1);
    EXPECT_EQ(m.Right(),  2);
    EXPECT_EQ(m.Top(),    3);
    EXPECT_EQ(m.Bottom(), 4);
}

TEST(TiffSettingsSmoke, DefaultsMatchCanonicalEnums) {
    using namespace Aspose::Pdf::Devices;
    TiffSettings s;
    EXPECT_EQ(s.Compression(), CompressionType::LZW);
    EXPECT_EQ(s.Depth(), ColorDepth::Default);
    EXPECT_EQ(s.Shape(), ShapeType::None);
    EXPECT_FLOAT_EQ(s.Brightness(), 0.5f);
    EXPECT_FALSE(s.SkipBlankPages());
}

TEST(TiffSettingsSmoke, CompressionCtor) {
    using namespace Aspose::Pdf::Devices;
    TiffSettings s(CompressionType::CCITT4);
    EXPECT_EQ(s.Compression(), CompressionType::CCITT4);
}

TEST(TiffSettingsSmoke, FullCtor_AllRoundtrip) {
    using namespace Aspose::Pdf::Devices;
    Margins margins(5, 6, 7, 8);
    TiffSettings s(CompressionType::None,
                   ColorDepth::Format1bpp,
                   margins,
                   /*skipBlankPages=*/true,
                   ShapeType::Landscape);
    EXPECT_EQ(s.Compression(), CompressionType::None);
    EXPECT_EQ(s.Depth(), ColorDepth::Format1bpp);
    EXPECT_TRUE(s.SkipBlankPages());
    EXPECT_EQ(s.Shape(), ShapeType::Landscape);
    EXPECT_EQ(s.Margins().Left(), 5);
}

TEST(TiffSettingsSmoke, BrightnessRoundtrip) {
    Aspose::Pdf::Devices::TiffSettings s;
    s.Brightness(0.85f);
    EXPECT_FLOAT_EQ(s.Brightness(), 0.85f);
}

TEST(TiffSettingsSmoke, CoordinateTypeRoundtrip) {
    Aspose::Pdf::Devices::TiffSettings s;
    s.CoordinateType(Aspose::Pdf::PageCoordinateType::MediaBox);
    EXPECT_EQ(s.CoordinateType(), Aspose::Pdf::PageCoordinateType::MediaBox);
}
