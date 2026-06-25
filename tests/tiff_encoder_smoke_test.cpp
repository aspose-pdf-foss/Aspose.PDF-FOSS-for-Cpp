// =============================================================================
// tiff_encoder_smoke_test — direct unit coverage for the
// foundation::tiff_encoder primitive. Verifies header layout
// + IFD emission + chained multi-page IFDs + round-trip via
// foundation::tiff_decoder for the in-scope (compression,
// photometric, SPP) combinations + documented rejection paths.
// =============================================================================

#include "tiff_encoder.hpp"
#include "tiff_decoder.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstddef>
#include <span>

namespace {

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

std::vector<std::byte> GradientRgba(std::uint32_t w, std::uint32_t h) {
    std::vector<std::byte> p(static_cast<std::size_t>(w) * h * 4);
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            std::byte* q = p.data() + (y * w + x) * 4;
            q[0] = std::byte{static_cast<std::uint8_t>(x * 13 + y * 7)};
            q[1] = std::byte{static_cast<std::uint8_t>(x * 17 + y * 11)};
            q[2] = std::byte{static_cast<std::uint8_t>(x * 19 + y * 23)};
            q[3] = std::byte{0xFF};
        }
    }
    return p;
}

}  // namespace

TEST(TiffEncoderSmoke, HeaderIsLittleEndianMagic42) {
    auto pixels = SolidRgba(2, 2, 0xFF, 0x00, 0x00, 0xFF);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(pixels)};
    foundation::tiff_encoder::Options opts;
    const auto out = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);

    ASSERT_GE(out.size(), 8u);
    EXPECT_EQ(static_cast<char>(out[0]), 'I');
    EXPECT_EQ(static_cast<char>(out[1]), 'I');
    // Magic 42 LE.
    EXPECT_EQ(static_cast<std::uint8_t>(out[2]), 0x2A);
    EXPECT_EQ(static_cast<std::uint8_t>(out[3]), 0x00);
}

TEST(TiffEncoderSmoke, RgbaPassThroughRoundtrip) {
    auto src = GradientRgba(4, 4);
    foundation::tiff_encoder::Page page{4, 4,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric = foundation::tiff_encoder::Photometric::Rgb;
    opts.samples_per_pixel = 4;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);

    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.width, 4u);
    EXPECT_EQ(decoded.height, 4u);
    EXPECT_EQ(decoded.pixels, src);
}

TEST(TiffEncoderSmoke, RgbDropAlphaRoundtrip) {
    auto src = GradientRgba(3, 3);  // alpha=0xFF
    foundation::tiff_encoder::Page page{3, 3,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric = foundation::tiff_encoder::Photometric::Rgb;
    opts.samples_per_pixel = 3;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);

    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, src);  // alpha widened to 0xFF on read
}

TEST(TiffEncoderSmoke, GrayscaleRoundtrip) {
    // Build R=G=B grayscale RGBA8 buffer.
    std::vector<std::byte> src(2 * 2 * 4);
    for (std::size_t i = 0; i < src.size(); i += 4) {
        const std::uint8_t v = static_cast<std::uint8_t>(0x40 + i);
        src[i + 0] = std::byte{v};
        src[i + 1] = std::byte{v};
        src[i + 2] = std::byte{v};
        src[i + 3] = std::byte{0xFF};
    }
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric =
        foundation::tiff_encoder::Photometric::BlackIsZero;
    opts.samples_per_pixel = 1;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);

    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, src);  // grayscale replicated R=G=B on read
}

TEST(TiffEncoderSmoke, LzwCompressionRoundtrip) {
    auto src = GradientRgba(8, 8);
    foundation::tiff_encoder::Page page{8, 8,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.compression = foundation::tiff_encoder::Compression::Lzw;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);
    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, src);
}

TEST(TiffEncoderSmoke, DeflateCompressionRoundtrip) {
    auto src = GradientRgba(8, 8);
    foundation::tiff_encoder::Page page{8, 8,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.compression = foundation::tiff_encoder::Compression::Deflate;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);
    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, src);
}

TEST(TiffEncoderSmoke, PackBitsCompressionRoundtrip) {
    auto src = SolidRgba(8, 8, 0xFF, 0x00, 0x00, 0xFF);  // run-friendly
    foundation::tiff_encoder::Page page{8, 8,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.compression = foundation::tiff_encoder::Compression::PackBits;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&page, 1), opts);
    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, src);
}

TEST(TiffEncoderSmoke, MultiPageChainedIfds) {
    // Encode 3 distinct pages; verify the encoded TIFF parses
    // as 3 IFDs by walking next-IFD-offsets manually. Pixel
    // round-trip is checked on the first page (decoder reads
    // first IFD only).
    auto a = SolidRgba(2, 2, 0xFF, 0, 0, 0xFF);
    auto b = SolidRgba(2, 2, 0, 0xFF, 0, 0xFF);
    auto c = SolidRgba(2, 2, 0, 0, 0xFF, 0xFF);
    foundation::tiff_encoder::Page p_a{2, 2, std::span<const std::byte>(a)};
    foundation::tiff_encoder::Page p_b{2, 2, std::span<const std::byte>(b)};
    foundation::tiff_encoder::Page p_c{2, 2, std::span<const std::byte>(c)};
    std::vector<foundation::tiff_encoder::Page> pages = {p_a, p_b, p_c};
    foundation::tiff_encoder::Options opts;
    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(pages), opts);

    // Walk IFD chain: header[4..7] is first-IFD-offset (LE u32);
    // each IFD is entry_count(2) + entries(N*12) + next_offset(4).
    auto read_u16 = [&](std::size_t off) -> std::uint16_t {
        return static_cast<std::uint16_t>(
            static_cast<std::uint8_t>(encoded[off]) |
            (static_cast<std::uint8_t>(encoded[off + 1]) << 8));
    };
    auto read_u32 = [&](std::size_t off) -> std::uint32_t {
        return static_cast<std::uint32_t>(
            static_cast<std::uint8_t>(encoded[off + 0]) |
            (static_cast<std::uint8_t>(encoded[off + 1]) << 8) |
            (static_cast<std::uint8_t>(encoded[off + 2]) << 16) |
            (static_cast<std::uint8_t>(encoded[off + 3]) << 24));
    };
    std::uint32_t ifd_off = read_u32(4);
    int chain_len = 0;
    while (ifd_off != 0) {
        ++chain_len;
        const std::uint16_t entry_count = read_u16(ifd_off);
        const std::size_t next_slot = ifd_off + 2 + entry_count * 12;
        ifd_off = read_u32(next_slot);
    }
    EXPECT_EQ(chain_len, 3);

    // First page round-trip via decoder.
    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(encoded));
    EXPECT_EQ(decoded.pixels, a);
}

TEST(TiffEncoderSmoke, RejectsEmptyPages) {
    foundation::tiff_encoder::Options opts;
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>{},
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsRgbaBufferLengthMismatch) {
    std::vector<std::byte> bad(15);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(bad)};
    foundation::tiff_encoder::Options opts;
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsBlackIsZeroWithNonGrayscale) {
    auto src = SolidRgba(2, 2, 0xFF, 0, 0, 0xFF);  // red — not gray
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric =
        foundation::tiff_encoder::Photometric::BlackIsZero;
    opts.samples_per_pixel = 1;
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsWhiteIsZero) {
    auto src = SolidRgba(2, 2, 0, 0, 0, 0xFF);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric =
        foundation::tiff_encoder::Photometric::WhiteIsZero;
    opts.samples_per_pixel = 1;
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsPaletteWithoutColorMap) {
    auto src = SolidRgba(2, 2, 5, 0, 0, 0xFF);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric = foundation::tiff_encoder::Photometric::Palette;
    opts.samples_per_pixel = 1;
    // color_map left empty.
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsZeroDpi) {
    auto src = SolidRgba(2, 2, 0, 0, 0, 0xFF);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.dpi = 0;
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}

TEST(TiffEncoderSmoke, RejectsPhotometricSppMismatch) {
    auto src = SolidRgba(2, 2, 0, 0, 0, 0xFF);
    foundation::tiff_encoder::Page page{2, 2,
        std::span<const std::byte>(src)};
    foundation::tiff_encoder::Options opts;
    opts.photometric = foundation::tiff_encoder::Photometric::Rgb;
    opts.samples_per_pixel = 1;  // mismatch — Rgb requires 3 or 4
    EXPECT_THROW(foundation::tiff_encoder::Encode(
                     std::span<const foundation::tiff_encoder::Page>(&page, 1),
                     opts),
                 std::runtime_error);
}
