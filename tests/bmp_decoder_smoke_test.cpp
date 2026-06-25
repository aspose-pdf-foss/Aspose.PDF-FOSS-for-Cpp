// =============================================================================
// bmp_decoder_smoke_test — direct unit coverage for the
// foundation::bmp_decoder primitive. Verifies header parsing,
// signature/biSize/dimension validation, channel translation
// (BGR→RGBA, BGRA→RGBA), bottom-up scanline inversion, row
// padding handling, BITFIELDS mask validation, and the
// documented rejection paths.
// =============================================================================

#include "bmp_decoder.hpp"
#include "bmp_encoder.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstddef>
#include <span>

namespace {

// Build a 2x2 RGBA buffer (top-down) with distinct pixels for
// each corner. Top-left red, top-right green, bottom-left blue,
// bottom-right white. Alpha varies so BGRA path can verify
// preservation.
std::vector<std::byte> Make2x2Rgba(std::uint8_t alpha_seed = 0xFF) {
    std::vector<std::byte> p;
    auto push = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b,
                    std::uint8_t a) {
        p.push_back(std::byte{r});
        p.push_back(std::byte{g});
        p.push_back(std::byte{b});
        p.push_back(std::byte{a});
    };
    push(0xFF, 0x00, 0x00, alpha_seed);  // top-left red
    push(0x00, 0xFF, 0x00, alpha_seed);  // top-right green
    push(0x00, 0x00, 0xFF, alpha_seed);  // bottom-left blue
    push(0xFF, 0xFF, 0xFF, alpha_seed);  // bottom-right white
    return p;
}

}  // namespace

TEST(BmpDecoderSmoke, Bgr_2x2_RoundtripsThroughEncoder) {
    auto src = Make2x2Rgba(0xFF);  // alpha=0xFF for BMP24 round-trip.
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    const auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);

    const auto decoded = foundation::bmp_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.width, 2u);
    EXPECT_EQ(decoded.height, 2u);
    EXPECT_EQ(decoded.components, 4u);
    EXPECT_EQ(decoded.pixels.size(), 16u);
    // BMP24 widens alpha to 0xFF on read; src has alpha=0xFF too →
    // byte-identical round-trip.
    EXPECT_EQ(decoded.pixels, src);
}

TEST(BmpDecoderSmoke, Bgra_2x2_RoundtripPreservesAlpha) {
    // Non-trivial alpha values to verify preservation.
    std::vector<std::byte> src;
    auto push = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b,
                    std::uint8_t a) {
        src.push_back(std::byte{r});
        src.push_back(std::byte{g});
        src.push_back(std::byte{b});
        src.push_back(std::byte{a});
    };
    push(0x10, 0x20, 0x30, 0x40);
    push(0x50, 0x60, 0x70, 0x80);
    push(0x90, 0xA0, 0xB0, 0xC0);
    push(0xD0, 0xE0, 0xF0, 0xFF);

    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgra;
    const auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);

    const auto decoded = foundation::bmp_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.width, 2u);
    EXPECT_EQ(decoded.height, 2u);
    EXPECT_EQ(decoded.pixels, src);  // BMP32 pass-through round-trip.
}

TEST(BmpDecoderSmoke, Bgr_3x4_OddWidthPadding) {
    constexpr std::uint32_t W = 3, H = 4;
    std::vector<std::byte> src(W * H * 4);
    // Pattern with byte distinct per pixel position.
    for (std::size_t i = 0; i < src.size(); i += 4) {
        src[i + 0] = std::byte{static_cast<std::uint8_t>(i)};
        src[i + 1] = std::byte{static_cast<std::uint8_t>(i + 1)};
        src[i + 2] = std::byte{static_cast<std::uint8_t>(i + 2)};
        src[i + 3] = std::byte{0xFF};  // alpha=0xFF for BMP24 round-trip.
    }
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    const auto encoded = foundation::bmp_encoder::Encode(
        W, H, std::span<const std::byte>(src), opts);

    const auto decoded = foundation::bmp_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.width, W);
    EXPECT_EQ(decoded.height, H);
    EXPECT_EQ(decoded.pixels, src);  // padding correctly skipped.
}

TEST(BmpDecoderSmoke, RejectsTooShort) {
    std::vector<std::byte> tiny(3, std::byte{0});
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(tiny)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsBadSignature) {
    // 70-byte BMP-shaped buffer but wrong signature.
    std::vector<std::byte> buf(70, std::byte{0});
    buf[0] = std::byte{'X'};
    buf[1] = std::byte{'Y'};
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(buf)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsBiSizeMismatch) {
    auto src = Make2x2Rgba(0xFF);
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);
    // Corrupt biSize at offset 14 — set to 12 (BITMAPCOREHEADER).
    encoded[14] = std::byte{12};
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(encoded)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsTopDownNegativeHeight) {
    auto src = Make2x2Rgba(0xFF);
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);
    // Flip biHeight to negative (offset 22..25 = -2 LE).
    encoded[22] = std::byte{0xFE};
    encoded[23] = std::byte{0xFF};
    encoded[24] = std::byte{0xFF};
    encoded[25] = std::byte{0xFF};
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(encoded)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsNonCanonicalBitfieldsMasks) {
    auto src = Make2x2Rgba(0xFF);
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgra;
    auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);
    // Corrupt RedMask at offset 54 — change to 0x0000FF00 (wrong).
    encoded[54] = std::byte{0x00};
    encoded[55] = std::byte{0xFF};
    encoded[56] = std::byte{0x00};
    encoded[57] = std::byte{0x00};
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(encoded)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsTruncatedPixelData) {
    auto src = Make2x2Rgba(0xFF);
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);
    // Drop the last 4 bytes so the declared pixel data runs past EOF.
    encoded.resize(encoded.size() - 4);
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(encoded)),
                 std::runtime_error);
}

TEST(BmpDecoderSmoke, RejectsBiBitCountUnsupported) {
    auto src = Make2x2Rgba(0xFF);
    foundation::bmp_encoder::Options opts;
    opts.color_type = foundation::bmp_encoder::ColorType::Bgr;
    auto encoded = foundation::bmp_encoder::Encode(
        2, 2, std::span<const std::byte>(src), opts);
    // Patch biBitCount at offset 28 from 24 → 16.
    encoded[28] = std::byte{16};
    encoded[29] = std::byte{0};
    EXPECT_THROW(foundation::bmp_decoder::Decode(
                     std::span<const std::byte>(encoded)),
                 std::runtime_error);
}
