// =============================================================================
// enum_canonical_values_smoke_test — pin every public-API enum's
// numeric values to canonical Aspose.PDF 26.4.0 (extracted via
// reference enum-values record). Any drift here indicates a value
// mismatch against the canonical assembly.
// =============================================================================

#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/form_presentation_mode.hpp>
#include <aspose/pdf/page_coordinate_type.hpp>
#include <aspose/pdf/permissions.hpp>
#include <aspose/pdf/shape_type.hpp>

#include <gtest/gtest.h>

TEST(EnumCanonicalValues, PageCoordinateType) {
    using Aspose::Pdf::PageCoordinateType;
    EXPECT_EQ(static_cast<int>(PageCoordinateType::MediaBox), 0);
    EXPECT_EQ(static_cast<int>(PageCoordinateType::CropBox),  1);
}

TEST(EnumCanonicalValues, ShapeType) {
    using Aspose::Pdf::Devices::ShapeType;
    EXPECT_EQ(static_cast<int>(ShapeType::None),      0);
    EXPECT_EQ(static_cast<int>(ShapeType::Landscape), 1);
    EXPECT_EQ(static_cast<int>(ShapeType::Portrait),  2);
}

TEST(EnumCanonicalValues, CompressionType) {
    using Aspose::Pdf::Devices::CompressionType;
    EXPECT_EQ(static_cast<int>(CompressionType::LZW),    0);
    EXPECT_EQ(static_cast<int>(CompressionType::CCITT4), 1);
    EXPECT_EQ(static_cast<int>(CompressionType::CCITT3), 2);
    EXPECT_EQ(static_cast<int>(CompressionType::RLE),    3);
    EXPECT_EQ(static_cast<int>(CompressionType::None),   4);
}

TEST(EnumCanonicalValues, FormPresentationMode) {
    using Aspose::Pdf::Devices::FormPresentationMode;
    EXPECT_EQ(static_cast<int>(FormPresentationMode::Production), 0);
    EXPECT_EQ(static_cast<int>(FormPresentationMode::Editor),     1);
}

TEST(EnumCanonicalValues, ColorDepth) {
    using Aspose::Pdf::Devices::ColorDepth;
    EXPECT_EQ(static_cast<int>(ColorDepth::Default),     0);
    EXPECT_EQ(static_cast<int>(ColorDepth::Format24bpp), 1);
    EXPECT_EQ(static_cast<int>(ColorDepth::Format8bpp),  2);
    EXPECT_EQ(static_cast<int>(ColorDepth::Format4bpp),  3);
    EXPECT_EQ(static_cast<int>(ColorDepth::Format1bpp),  4);
}

TEST(EnumCanonicalValues, Permissions) {
    using Aspose::Pdf::Permissions;
    EXPECT_EQ(static_cast<int>(Permissions::PrintDocument),                  4);
    EXPECT_EQ(static_cast<int>(Permissions::ModifyContent),                  8);
    EXPECT_EQ(static_cast<int>(Permissions::ExtractContent),                 16);
    EXPECT_EQ(static_cast<int>(Permissions::ModifyTextAnnotations),          32);
    EXPECT_EQ(static_cast<int>(Permissions::FillForm),                       256);
    EXPECT_EQ(static_cast<int>(Permissions::ExtractContentWithDisabilities), 512);
    EXPECT_EQ(static_cast<int>(Permissions::AssembleDocument),               1024);
    EXPECT_EQ(static_cast<int>(Permissions::PrintingQuality),                2048);
}

TEST(EnumCanonicalValues, PermissionsBitwise) {
    using Aspose::Pdf::Permissions;
    // [Flags] composition — operator| / operator& / operator~ / operator|=
    // / operator&= / operator^= preserve enum-class type safety while
    // letting callers compose the bitfield like the .NET [Flags] enum.
    const auto all_print = Permissions::PrintDocument
                         | Permissions::PrintingQuality;
    EXPECT_EQ(static_cast<int>(all_print), 4 | 2048);

    EXPECT_EQ(static_cast<int>(all_print & Permissions::PrintDocument),
              static_cast<int>(Permissions::PrintDocument));

    Permissions p = Permissions::FillForm;
    p |= Permissions::AssembleDocument;
    EXPECT_EQ(static_cast<int>(p), 256 | 1024);
    p &= ~Permissions::FillForm;
    EXPECT_EQ(static_cast<int>(p), 1024);
}

TEST(EnumCanonicalValues, CryptoAlgorithm) {
    using Aspose::Pdf::CryptoAlgorithm;
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::RC4x40),  0);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::RC4x128), 1);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::AESx128), 2);
    EXPECT_EQ(static_cast<int>(CryptoAlgorithm::AESx256), 3);
}
