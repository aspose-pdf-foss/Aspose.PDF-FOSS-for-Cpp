// =============================================================================
// bmp_encoder_smoke_test — direct unit coverage for the
// foundation::bmp_encoder primitive. Verifies header layout,
// row padding, RGBA → BGR / BGRA translation, bottom-up row
// order, and the documented input-validation errors.
// =============================================================================

#include "bmp_encoder.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <cstddef>
#include <span>

namespace {

std::uint16_t Read16LE(const std::vector<std::byte>& v, std::size_t off) {
    return static_cast<std::uint16_t>(v[off]) |
           (static_cast<std::uint16_t>(v[off + 1]) << 8);
}

std::uint32_t Read32LE(const std::vector<std::byte>& v, std::size_t off) {
    return static_cast<std::uint32_t>(v[off]) |
           (static_cast<std::uint32_t>(v[off + 1]) << 8) |
           (static_cast<std::uint32_t>(v[off + 2]) << 16) |
           (static_cast<std::uint32_t>(v[off + 3]) << 24);
}

std::int32_t Read32LESigned(const std::vector<std::byte>& v, std::size_t off) {
    return static_cast<std::int32_t>(Read32LE(v, off));
}

// Build a 2x2 RGBA buffer where each pixel encodes (R,G,B,A)
// readable from a quick visual check.
std::vector<std::byte> Make2x2() {
    std::vector<std::byte> p;
    p.reserve(16);
    auto push = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b,
                    std::uint8_t a) {
        p.push_back(std::byte{r});
        p.push_back(std::byte{g});
        p.push_back(std::byte{b});
        p.push_back(std::byte{a});
    };
    // (0,0) = top-left red, (1,0) = top-right green
    push(0xFF, 0x00, 0x00, 0xFF);
    push(0x00, 0xFF, 0x00, 0xFF);
    // (0,1) = bottom-left blue, (1,1) = bottom-right white
    push(0x00, 0x00, 0xFF, 0xFF);
    push(0xFF, 0xFF, 0xFF, 0xFF);
    return p;
}

}  // namespace

TEST(BmpEncoderSmoke, Bgr_2x2_HeaderLayout) {
    auto pixels = Make2x2();
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    const auto out = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);

    // 14 + 40 + 2 rows * (2*3 + 2 padding) = 54 + 16 = 70.
    ASSERT_EQ(out.size(), 70u);

    // 'BM' magic.
    EXPECT_EQ(out[0], std::byte{'B'});
    EXPECT_EQ(out[1], std::byte{'M'});

    // bfSize equals total length.
    EXPECT_EQ(Read32LE(out, 2), 70u);
    // bfReserved1/2 = 0
    EXPECT_EQ(Read16LE(out, 6), 0u);
    EXPECT_EQ(Read16LE(out, 8), 0u);
    // bfOffBits = 14 + 40 = 54 (no BITFIELDS for BMP24).
    EXPECT_EQ(Read32LE(out, 10), 54u);

    // BITMAPINFOHEADER.
    EXPECT_EQ(Read32LE(out, 14), 40u);  // biSize
    EXPECT_EQ(Read32LESigned(out, 18), 2);  // biWidth
    EXPECT_EQ(Read32LESigned(out, 22), 2);  // biHeight (positive)
    EXPECT_EQ(Read16LE(out, 26), 1u);  // biPlanes
    EXPECT_EQ(Read16LE(out, 28), 24u);  // biBitCount
    EXPECT_EQ(Read32LE(out, 30), 0u);  // biCompression = BI_RGB
    EXPECT_EQ(Read32LE(out, 34), 16u);  // biSizeImage = 2 rows * 8 stride
    EXPECT_EQ(Read32LESigned(out, 38), 2835);  // biXPelsPerMeter
    EXPECT_EQ(Read32LESigned(out, 42), 2835);  // biYPelsPerMeter
    EXPECT_EQ(Read32LE(out, 46), 0u);  // biClrUsed
    EXPECT_EQ(Read32LE(out, 50), 0u);  // biClrImportant
}

TEST(BmpEncoderSmoke, Bgr_2x2_PixelOrderAndPadding) {
    auto pixels = Make2x2();
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    const auto out = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);

    // Pixel data starts at offset 54. Bottom-up: file-row 0 is
    // image-row 1 (bottom), file-row 1 is image-row 0 (top).
    // Stride = 2*3 + 2 padding = 8.
    //
    // Image-row 1 (bottom): blue, white  →  B,G,R = 0xFF,0,0  /  0xFF,0xFF,0xFF
    EXPECT_EQ(out[54], std::byte{0xFF});  // B (blue's B)
    EXPECT_EQ(out[55], std::byte{0x00});  // G
    EXPECT_EQ(out[56], std::byte{0x00});  // R
    EXPECT_EQ(out[57], std::byte{0xFF});  // B (white)
    EXPECT_EQ(out[58], std::byte{0xFF});  // G
    EXPECT_EQ(out[59], std::byte{0xFF});  // R
    EXPECT_EQ(out[60], std::byte{0});  // padding
    EXPECT_EQ(out[61], std::byte{0});  // padding

    // Image-row 0 (top): red, green  →  B,G,R = 0,0,0xFF  /  0,0xFF,0
    EXPECT_EQ(out[62], std::byte{0x00});  // B
    EXPECT_EQ(out[63], std::byte{0x00});  // G
    EXPECT_EQ(out[64], std::byte{0xFF});  // R (red)
    EXPECT_EQ(out[65], std::byte{0x00});  // B
    EXPECT_EQ(out[66], std::byte{0xFF});  // G (green)
    EXPECT_EQ(out[67], std::byte{0x00});  // R
    EXPECT_EQ(out[68], std::byte{0});  // padding
    EXPECT_EQ(out[69], std::byte{0});  // padding
}

TEST(BmpEncoderSmoke, Bgra_2x2_HeaderAndBitfields) {
    auto pixels = Make2x2();
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgra;
    const auto out = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);

    // 14 + 40 + 12 (bitfields) + 2 * 2 * 4 = 66 + 16 = 82.
    ASSERT_EQ(out.size(), 82u);

    // bfOffBits = 14 + 40 + 12 = 66.
    EXPECT_EQ(Read32LE(out, 10), 66u);

    // biBitCount / biCompression.
    EXPECT_EQ(Read16LE(out, 28), 32u);
    EXPECT_EQ(Read32LE(out, 30), 3u);  // BI_BITFIELDS

    // BITFIELDS block at 54..65.
    EXPECT_EQ(Read32LE(out, 54), 0x00FF0000u);  // RedMask
    EXPECT_EQ(Read32LE(out, 58), 0x0000FF00u);  // GreenMask
    EXPECT_EQ(Read32LE(out, 62), 0x000000FFu);  // BlueMask
}

TEST(BmpEncoderSmoke, Bgra_2x2_AlphaPreserved) {
    // Build a 2x2 with non-trivial alpha values and verify the
    // BMP32 round-trip preserves them at offset +3.
    std::vector<std::byte> pixels;
    auto push = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b,
                    std::uint8_t a) {
        pixels.push_back(std::byte{r});
        pixels.push_back(std::byte{g});
        pixels.push_back(std::byte{b});
        pixels.push_back(std::byte{a});
    };
    push(0x10, 0x20, 0x30, 0x40);
    push(0x50, 0x60, 0x70, 0x80);
    push(0x90, 0xA0, 0xB0, 0xC0);
    push(0xD0, 0xE0, 0xF0, 0xFF);

    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgra;
    const auto out = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);

    // Pixel data at offset 66. Bottom-up: image-row 1 first.
    // image-row 1 is the second pair (0x90,A0,B0,C0) and (0xD0,E0,F0,FF).
    EXPECT_EQ(out[66], std::byte{0xB0});  // B
    EXPECT_EQ(out[67], std::byte{0xA0});  // G
    EXPECT_EQ(out[68], std::byte{0x90});  // R
    EXPECT_EQ(out[69], std::byte{0xC0});  // A — preserved
    EXPECT_EQ(out[70], std::byte{0xF0});
    EXPECT_EQ(out[71], std::byte{0xE0});
    EXPECT_EQ(out[72], std::byte{0xD0});
    EXPECT_EQ(out[73], std::byte{0xFF});
    // image-row 0 next: first pair (0x10,20,30,40) and (0x50,60,70,80).
    EXPECT_EQ(out[74], std::byte{0x30});
    EXPECT_EQ(out[75], std::byte{0x20});
    EXPECT_EQ(out[76], std::byte{0x10});
    EXPECT_EQ(out[77], std::byte{0x40});  // A
    EXPECT_EQ(out[78], std::byte{0x70});
    EXPECT_EQ(out[79], std::byte{0x60});
    EXPECT_EQ(out[80], std::byte{0x50});
    EXPECT_EQ(out[81], std::byte{0x80});  // A
}

TEST(BmpEncoderSmoke, Bgr_3x4_RowPaddingThreeBytes) {
    // 3 px wide, BMP24 → row data = 9 bytes, padding to 12.
    constexpr std::uint32_t W = 3, H = 4;
    std::vector<std::byte> pixels(W * H * 4, std::byte{0});
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    const auto out = foundation::bmp_encoder::Encode(
        W, H, std::span<const std::byte>(pixels), opts);

    // 14 + 40 + 4 rows * 12 stride = 54 + 48 = 102.
    EXPECT_EQ(out.size(), 102u);
    // biSizeImage at offset 34 = 48.
    EXPECT_EQ(Read32LE(out, 34), 48u);
}

TEST(BmpEncoderSmoke, Bgra_5x4_NoRowPadding) {
    // BMP32 always 4-aligned per row regardless of width.
    constexpr std::uint32_t W = 5, H = 4;
    std::vector<std::byte> pixels(W * H * 4, std::byte{0xAA});
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgra;
    const auto out = foundation::bmp_encoder::Encode(
        W, H, std::span<const std::byte>(pixels), opts);

    // 14 + 40 + 12 + 4 rows * 5*4 stride = 66 + 80 = 146.
    EXPECT_EQ(out.size(), 146u);
    EXPECT_EQ(Read32LE(out, 34), 80u);  // biSizeImage
}

TEST(BmpEncoderSmoke, RejectsZeroDimensions) {
    std::vector<std::byte> empty;
    foundation::bmp_encoder::Options opts;
    EXPECT_THROW(
        foundation::bmp_encoder::Encode(
            0, 1, std::span<const std::byte>(empty), opts),
        std::runtime_error);
    EXPECT_THROW(
        foundation::bmp_encoder::Encode(
            1, 0, std::span<const std::byte>(empty), opts),
        std::runtime_error);
}

TEST(BmpEncoderSmoke, RejectsBufferLengthMismatch) {
    std::vector<std::byte> too_small(15, std::byte{0});  // not 2*2*4=16
    foundation::bmp_encoder::Options opts;
    EXPECT_THROW(
        foundation::bmp_encoder::Encode(
            2, 2, std::span<const std::byte>(too_small), opts),
        std::runtime_error);
}

TEST(BmpEncoderSmoke, RejectsUnknownColorType) {
    std::vector<std::byte> pixels(16, std::byte{0});
    foundation::bmp_encoder::Options opts;
    opts.color_type = static_cast<foundation::bmp_encoder::ColorType>(99);
    EXPECT_THROW(
        foundation::bmp_encoder::Encode(
            2, 2, std::span<const std::byte>(pixels), opts),
        std::runtime_error);
}
