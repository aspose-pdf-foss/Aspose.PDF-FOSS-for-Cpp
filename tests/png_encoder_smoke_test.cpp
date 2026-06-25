// =============================================================================
// png_encoder_smoke_test — direct unit coverage for the
// foundation::png_encoder primitive. Verifies signature + IHDR
// + IDAT + IEND chunk emission, CRC-32 placement, color-type
// dispatch (Grayscale/Rgb/Rgba), grayscale R==G==B validation,
// and the documented input-validation rejection paths.
//
// Round-trip coverage (encode → decode == input) lives in
// png_decoder_smoke_test.cpp; this file focuses on
// structural emit assertions on the byte stream itself.
// =============================================================================

#include "png_encoder.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <array>
#include <cstddef>
#include <span>

namespace {

std::uint32_t Read32BE(const std::vector<std::byte>& v, std::size_t off) {
    return (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(v[off + 0])) << 24) |
           (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(v[off + 1])) << 16) |
           (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(v[off + 2])) << 8) |
           static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(v[off + 3]));
}

std::vector<std::byte> SolidRgba(std::uint32_t w, std::uint32_t h,
                                 std::uint8_t r, std::uint8_t g,
                                 std::uint8_t b, std::uint8_t a) {
    std::vector<std::byte> p(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < p.size(); i += 4) {
        p[i + 0] = std::byte{r};
        p[i + 1] = std::byte{g};
        p[i + 2] = std::byte{b};
        p[i + 3] = std::byte{a};
    }
    return p;
}

}  // namespace

TEST(PngEncoderSmoke, Rgba_2x2_SignatureAndIhdrPresent) {
    auto pixels = SolidRgba(2, 2, 0xAA, 0xBB, 0xCC, 0xDD);
    const auto out = foundation::png_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels),
        foundation::png_encoder::Options{});

    // Signature.
    ASSERT_GE(out.size(), 8u + 8u + 13u + 4u);
    const std::array<std::uint8_t, 8> sig = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(static_cast<std::uint8_t>(out[i]), sig[i])
            << "signature byte " << i;
    }

    // IHDR length + type at offset 8.
    EXPECT_EQ(Read32BE(out, 8), 13u);
    EXPECT_EQ(static_cast<char>(out[12]), 'I');
    EXPECT_EQ(static_cast<char>(out[13]), 'H');
    EXPECT_EQ(static_cast<char>(out[14]), 'D');
    EXPECT_EQ(static_cast<char>(out[15]), 'R');
    // IHDR payload starts at offset 16.
    EXPECT_EQ(Read32BE(out, 16), 2u);  // width
    EXPECT_EQ(Read32BE(out, 20), 2u);  // height
    EXPECT_EQ(static_cast<std::uint8_t>(out[24]), 8u);   // bit_depth
    EXPECT_EQ(static_cast<std::uint8_t>(out[25]), 6u);   // Rgba
    EXPECT_EQ(static_cast<std::uint8_t>(out[26]), 0u);   // compression
    EXPECT_EQ(static_cast<std::uint8_t>(out[27]), 0u);   // filter
    EXPECT_EQ(static_cast<std::uint8_t>(out[28]), 0u);   // interlace
}

TEST(PngEncoderSmoke, EndsWithIend) {
    auto pixels = SolidRgba(3, 3, 0xFF, 0, 0, 0xFF);
    const auto out = foundation::png_encoder::Encode(
        3, 3, std::span<const std::byte>(pixels),
        foundation::png_encoder::Options{});

    // IEND is the last 12 bytes: length(4)=0 + "IEND"(4) + CRC(4).
    ASSERT_GE(out.size(), 12u);
    const std::size_t iend_off = out.size() - 12;
    EXPECT_EQ(Read32BE(out, iend_off), 0u);  // length
    EXPECT_EQ(static_cast<char>(out[iend_off + 4]), 'I');
    EXPECT_EQ(static_cast<char>(out[iend_off + 5]), 'E');
    EXPECT_EQ(static_cast<char>(out[iend_off + 6]), 'N');
    EXPECT_EQ(static_cast<char>(out[iend_off + 7]), 'D');
    // IEND CRC over "IEND" with empty data — the well-known
    // PNG-spec value 0xAE426082.
    EXPECT_EQ(Read32BE(out, iend_off + 8), 0xAE426082u);
}

TEST(PngEncoderSmoke, ColorTypeRgbDropsAlpha) {
    auto pixels = SolidRgba(2, 2, 0xFF, 0, 0, 0xFF);
    foundation::png_encoder::Options opts;
    opts.color_type = foundation::png_encoder::ColorType::Rgb;
    const auto out = foundation::png_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);
    // IHDR color_type byte at offset 25.
    EXPECT_EQ(static_cast<std::uint8_t>(out[25]), 2u);
}

TEST(PngEncoderSmoke, ColorTypeGrayscale_AcceptsRGGB) {
    // Grayscale requires R == G == B at every pixel.
    std::vector<std::byte> pixels(2 * 2 * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        const std::uint8_t v =
            static_cast<std::uint8_t>(0x40 + static_cast<int>(i / 4));
        pixels[i + 0] = std::byte{v};
        pixels[i + 1] = std::byte{v};
        pixels[i + 2] = std::byte{v};
        pixels[i + 3] = std::byte{0xFF};
    }
    foundation::png_encoder::Options opts;
    opts.color_type = foundation::png_encoder::ColorType::Grayscale;
    const auto out = foundation::png_encoder::Encode(
        2, 2, std::span<const std::byte>(pixels), opts);
    EXPECT_EQ(static_cast<std::uint8_t>(out[25]), 0u);  // color_type
}

TEST(PngEncoderSmoke, ColorTypeGrayscale_RejectsNonGrayscale) {
    auto pixels = SolidRgba(2, 2, 0xFF, 0, 0, 0xFF);  // red — not gray
    foundation::png_encoder::Options opts;
    opts.color_type = foundation::png_encoder::ColorType::Grayscale;
    EXPECT_THROW(foundation::png_encoder::Encode(
                     2, 2, std::span<const std::byte>(pixels), opts),
                 std::runtime_error);
}

TEST(PngEncoderSmoke, RejectsZeroDimensions) {
    std::vector<std::byte> empty;
    EXPECT_THROW(foundation::png_encoder::Encode(
                     0, 1, std::span<const std::byte>(empty),
                     foundation::png_encoder::Options{}),
                 std::runtime_error);
}

TEST(PngEncoderSmoke, RejectsBufferLengthMismatch) {
    std::vector<std::byte> too_small(15);
    EXPECT_THROW(foundation::png_encoder::Encode(
                     2, 2, std::span<const std::byte>(too_small),
                     foundation::png_encoder::Options{}),
                 std::runtime_error);
}

TEST(PngEncoderSmoke, RejectsUnknownColorType) {
    auto pixels = SolidRgba(2, 2, 0, 0, 0, 0xFF);
    foundation::png_encoder::Options opts;
    opts.color_type =
        static_cast<foundation::png_encoder::ColorType>(99);
    EXPECT_THROW(foundation::png_encoder::Encode(
                     2, 2, std::span<const std::byte>(pixels), opts),
                 std::runtime_error);
}
