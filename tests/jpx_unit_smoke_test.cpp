#include "jpx.hpp"
#include "jpx_detail.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

TEST(JpxSmoke, EmptyInputThrows) {
    std::vector<std::byte> empty;
    EXPECT_THROW(foundation::jpx::Decode(empty), std::runtime_error);
}

TEST(JpxSmoke, NonJpegMagicThrows) {
    // Not a JP2 signature box and not an SOC marker.
    std::vector<std::byte> junk(16, std::byte{0x00});
    EXPECT_THROW(foundation::jpx::Decode(junk), std::runtime_error);
}

namespace {

// Canonical MQ-coder test sequence — ISO/IEC 15444-1 Annex C (Table C.2 is
// identical to ITU-T T.88 Table E.1, so the conformance vector is shared).
const std::array<std::uint8_t, 32> kMqTestStream = {{
    0x84, 0xC7, 0x3B, 0xFC, 0xE1, 0xA1, 0x43, 0x04, 0x02, 0x20, 0x00, 0x00,
    0x41, 0x0D, 0xBB, 0x86, 0xF4, 0x31, 0x7F, 0xFF, 0x88, 0xFF, 0x37, 0x47,
    0x1A, 0xDB, 0x6A, 0xDF, 0xFF, 0xAC, 0x00, 0x00,
}};

// Expected decoded output, 256 decisions packed MSB-first.
const std::array<std::uint8_t, 32> kMqExpected = {{
    0x00, 0x02, 0x00, 0x51, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x52, 0x87, 0x2A,
    0xAA, 0xAA, 0xAA, 0xAA, 0x82, 0xC0, 0x20, 0x00, 0xFC, 0xD7, 0x9E, 0xF6,
    0xBF, 0x7F, 0xED, 0x90, 0x4F, 0x46, 0xA3, 0xBF,
}};

}  // namespace

TEST(JpxMq, T88AnnexE3Sequence) {
    foundation::jpx::detail::MqDecoder mq(kMqTestStream.data(), kMqTestStream.size());
    foundation::jpx::detail::MqContext cx;  // index 0, mps 0

    std::array<std::uint8_t, 32> got{};
    for (int i = 0; i < 256; ++i) {
        const int bit = mq.Decode(cx);
        ASSERT_TRUE(bit == 0 || bit == 1) << "bit " << i;
        got[i >> 3] = static_cast<std::uint8_t>(got[i >> 3] | (bit << (7 - (i & 7))));
    }
    EXPECT_EQ(got, kMqExpected);
}

TEST(JpxDwt, Reversible53RoundTrip) {
    std::vector<std::int32_t> x{5, -3, 9, 0, 12, 7, -1, 4};  // even length
    std::vector<std::int32_t> y = x;
    foundation::jpx::detail::Fdwt53_1d(y);     // forward 1-level
    foundation::jpx::detail::Idwt53_1d(y);     // inverse 1-level
    EXPECT_EQ(y, x);                            // 5/3 is exactly reversible
}

TEST(JpxDwt, Reversible53RoundTripOddLength) {
    std::vector<std::int32_t> x{5, -3, 9, 0, 12, 7, -1};  // odd length
    std::vector<std::int32_t> y = x;
    foundation::jpx::detail::Fdwt53_1d(y);
    foundation::jpx::detail::Idwt53_1d(y);
    EXPECT_EQ(y, x);
}

TEST(JpxDwt, Irreversible97RoundTrip) {
    std::vector<double> x{5, -3, 9, 0, 12, 7, -1, 4};
    std::vector<double> y = x;
    foundation::jpx::detail::Fdwt97_1d(y);
    foundation::jpx::detail::Idwt97_1d(y);
    for (size_t i = 0; i < x.size(); ++i) EXPECT_NEAR(y[i], x[i], 1e-9);
}

TEST(JpxDwt, Irreversible97RoundTripOddLength) {
    std::vector<double> x{5, -3, 9, 0, 12, 7, -1};
    std::vector<double> y = x;
    foundation::jpx::detail::Fdwt97_1d(y);
    foundation::jpx::detail::Idwt97_1d(y);
    for (size_t i = 0; i < x.size(); ++i) EXPECT_NEAR(y[i], x[i], 1e-9);
}

namespace {

// Forward 5/3 DWT applied across every row, then every column, of a
// row-major w*h plane — one decomposition level (ISO/IEC 15444-1 Annex F,
// the separable two-dimensional transform of F.3.1).
void Fdwt2d53OneLevel(std::vector<std::int32_t>& plane, std::uint32_t w, std::uint32_t h) {
    std::vector<std::int32_t> row(w), col(h);
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) row[x] = plane[y * w + x];
        foundation::jpx::detail::Fdwt53_1d(row);
        for (std::uint32_t x = 0; x < w; ++x) plane[y * w + x] = row[x];
    }
    for (std::uint32_t x = 0; x < w; ++x) {
        for (std::uint32_t y = 0; y < h; ++y) col[y] = plane[y * w + x];
        foundation::jpx::detail::Fdwt53_1d(col);
        for (std::uint32_t y = 0; y < h; ++y) plane[y * w + x] = col[y];
    }
}

}  // namespace

TEST(JpxDwt, Plane53RoundTrip2D) {
    constexpr std::uint32_t w = 4, h = 4;
    std::vector<std::int32_t> x{
        1,  2,  3,  4,
        5,  6,  7,  8,
        9, 10, 11, 12,
        13, -2, -7, 0,
    };
    std::vector<std::int32_t> y = x;
    Fdwt2d53OneLevel(y, w, h);
    foundation::jpx::detail::Idwt2d_53(y, w, h, 1);
    EXPECT_EQ(y, x);
}

TEST(JpxTagTree, HierarchicalDecode) {
    // Bitstreams hand-derived from ISO/IEC 15444-1 Annex B.10.2 (and verified
    // against OpenJPEG opj_tgt_encode). A 1x1 tree encoding the value 2 emits
    // two refine 0-bits then a 1-bit -> "001" -> 0x20.
    {
        const std::array<std::uint8_t, 1> bits{{0x20}};
        const auto v = foundation::jpx::detail::TagTreeDecodeAll(bits, 1, 1);
        ASSERT_EQ(v.size(), 1u);
        EXPECT_EQ(v[0], 2);
    }
    // A 2x1 tree with leaf values {0, 2} (root = min = 0): decoding leaf0
    // reads "11" (root then leaf pinned at 0), leaf1 reads "001" -> "11001"
    // -> 0xC8. This exercises the shared root node across two code-blocks.
    {
        const std::array<std::uint8_t, 1> bits{{0xC8}};
        const auto v = foundation::jpx::detail::TagTreeDecodeAll(bits, 2, 1);
        ASSERT_EQ(v.size(), 2u);
        EXPECT_EQ(v[0], 0);
        EXPECT_EQ(v[1], 2);
    }
}

namespace {

// Fixture root for the jpx codestream/box parser tests — same convention as
// tests/bmp_device_e2e_smoke_test.cpp's FixtureRoot().
std::filesystem::path JpxFixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path() / "fixtures" / "jpx";
}

std::vector<std::byte> ReadFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open fixture: " + path.string());
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

}  // namespace

TEST(JpxParser, ParsesSizFromRawCodestream) {
    const auto bytes = ReadFile(JpxFixtureRoot() / "rgb_5x3_32.j2k");
    const auto image = foundation::jpx::detail::ParseCodestream(bytes);

    EXPECT_EQ(image.width, 32u);
    EXPECT_EQ(image.height, 32u);
    ASSERT_EQ(image.components.size(), 3u);
    for (const auto& c : image.components) {
        EXPECT_EQ(c.bit_depth, 8);
        EXPECT_FALSE(c.is_signed);
    }
    EXPECT_EQ(image.wavelet, foundation::jpx::detail::Wavelet::Reversible53);
    EXPECT_EQ(image.levels, 2);
    EXPECT_TRUE(image.mct);
}

TEST(JpxParser, ParsesMultiTileGeometry) {
    // 32x32 RGB, 2x2 tiles (16x16). The parser now assembles the tile grid and
    // each tile's bitstream (multi-tile compositing is exercised by the
    // reconstruction tests); this checks the geometry + per-tile assembly.
    const auto bytes = ReadFile(JpxFixtureRoot() / "multitile_rgb_32.j2k");
    const auto image = foundation::jpx::detail::ParseCodestream(bytes);
    EXPECT_EQ(image.tiles_x, 2u);
    EXPECT_EQ(image.tiles_y, 2u);
    ASSERT_EQ(image.tiles.size(), 4u);
    for (const auto& t : image.tiles) EXPECT_FALSE(t.data.empty());
}

TEST(JpxParser, DetectsJp2SignatureBox) {
    const auto bytes = ReadFile(JpxFixtureRoot() / "jp2_boxed_rgb_32.jp2");
    const auto cs = foundation::jpx::detail::Jp2FindCodestream(bytes);

    ASSERT_GE(cs.size(), 2u);
    EXPECT_EQ(cs[0], std::byte{0xFF});
    EXPECT_EQ(cs[1], std::byte{0x4F});

    const auto image = foundation::jpx::detail::ParseCodestream(cs);
    EXPECT_EQ(image.width, 32u);
    EXPECT_EQ(image.height, 32u);
    EXPECT_EQ(image.components.size(), 3u);
}

TEST(JpxParser, AcceptsCmykRlcp) {
    // Synthesised single-tile, single-layer, 4-component CMYK (colr EnumCS 12),
    // RLCP, 9/7 — must parse without throwing and report 4 components.
    int enum_cs = -1;
    const auto bytes = ReadFile(JpxFixtureRoot() / "cmyk_9x7_32.jp2");
    const auto cs = foundation::jpx::detail::Jp2FindCodestream(bytes, &enum_cs);
    EXPECT_EQ(enum_cs, 12);  // CMYK
    const auto image = foundation::jpx::detail::ParseCodestream(cs);
    EXPECT_EQ(image.components.size(), 4u);
    EXPECT_EQ(image.wavelet, foundation::jpx::detail::Wavelet::Irreversible97);
}

TEST(JpxTier1, Gray1LevelByteExact) {
    // 8x8 grayscale, no DWT, 5/3 lossless. Exercises tier-2 packet
    // parsing + tier-1 EBCOT + DC level-shift in isolation (no wavelet,
    // no MCT). Must match the OpenJPEG-oracle sidecar byte-for-byte.
    const auto bytes = ReadFile(JpxFixtureRoot() / "gray_1level_8.j2k");
    const auto expected = ReadFile(JpxFixtureRoot() / "gray_1level_8.pixels");
    auto img = foundation::jpx::Decode(bytes);
    EXPECT_EQ(img.width, 8u);
    EXPECT_EQ(img.height, 8u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), expected.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), expected.begin()));
}

TEST(JpxRecon, Gray53ByteExact) {
    // 32x32 grayscale, 5/3 reversible, 2 decomposition levels. Exercises
    // the multi-resolution packet walk + inverse DWT (no MCT) — ISO/IEC
    // 15444-1 Annex B (packets), Annex F (5/3 synthesis).
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "gray_5x3_32.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "gray_5x3_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

TEST(JpxParser, Parses16BitDepth) {
    const auto bytes = ReadFile(JpxFixtureRoot() / "gray16_5x3_32.j2k");
    const auto image = foundation::jpx::detail::ParseCodestream(bytes);
    ASSERT_EQ(image.components.size(), 1u);
    EXPECT_EQ(image.components[0].bit_depth, 16);
    EXPECT_FALSE(image.components[0].is_signed);
}

TEST(JpxRecon, Gray16BitByteExact) {
    // 32x32 16-bit grayscale, 5/3 reversible, 1 level. Exercises the >8-bit
    // sample path: the [0, 2^16-1] samples are scaled full-range down to 8-bit
    // RGBA on output (field 33342_*_16bit_*). A renderer that throws on
    // Ssiz != 8, or that truncates the 16-bit value to its low byte, fails this.
    auto img = foundation::jpx::Decode(
        ReadFile(JpxFixtureRoot() / "gray16_5x3_32.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "gray16_5x3_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

TEST(JpxRecon, Rgb53ByteExact) {
    // 32x32 RGB, 5/3 reversible + RCT, 2 decomposition levels — adds the
    // inverse reversible colour transform (Annex G.2) on top of Gray53.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "rgb_5x3_32.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "rgb_5x3_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

TEST(JpxRecon, Multilevel53ByteExact) {
    // 64x64 RGB, 5/3 reversible + RCT, 3 decomposition levels.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "multilevel_5x3_64.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "multilevel_5x3_64.pixels");
    EXPECT_EQ(img.width, 64u);
    EXPECT_EQ(img.height, 64u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

TEST(JpxRecon, Jp2Boxed53ByteExact) {
    // Same geometry as Rgb53ByteExact, but JP2-boxed — exercises the JP2
    // box-unwrap path feeding the same reconstruction pipeline.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "jp2_boxed_rgb_32.jp2"));
    auto exp = ReadFile(JpxFixtureRoot() / "jp2_boxed_rgb_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

namespace {

// Peak-signal-to-noise ratio (dB) between a decoded RGBA8 buffer and the
// oracle sidecar. The 9/7 path is lossy (float wavelet + quantization), so it
// is gated by PSNR rather than byte-equality, mirroring the fixture-authoring
// script's "PSNR gate" note. Returns +inf for an exact match.
double RgbaPsnr(const std::vector<std::byte>& got, const std::vector<std::byte>& exp) {
    double mse = 0.0;
    std::size_t n = 0;
    for (std::size_t i = 0; i < got.size(); ++i) {
        if (i % 4 == 3) continue;  // skip the constant alpha channel
        const double d = static_cast<double>(static_cast<std::uint8_t>(got[i])) -
                         static_cast<double>(static_cast<std::uint8_t>(exp[i]));
        mse += d * d;
        ++n;
    }
    if (n == 0 || mse == 0.0) return std::numeric_limits<double>::infinity();
    mse /= static_cast<double>(n);
    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

}  // namespace

TEST(JpxRecon, Gradient97Psnr) {
    // 64x64 grayscale, 9/7 irreversible, 2 decomposition levels. Exercises the
    // float synthesis + inverse quantization (no MCT) — ISO/IEC 15444-1 Annex
    // E (dequant) + Annex F (9/7 synthesis). Lossy: gated by PSNR.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "gradient_9x7_64.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "gradient_9x7_64.pixels");
    EXPECT_EQ(img.width, 64u);
    EXPECT_EQ(img.height, 64u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_GE(RgbaPsnr(img.pixels, exp), 50.0);
}

TEST(JpxRecon, Rgb97Psnr) {
    // 64x64 RGB, 9/7 irreversible + ICT, 2 decomposition levels — adds the
    // inverse irreversible colour transform (Annex G.1) on top of Gradient97.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "rgb_9x7_64.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "rgb_9x7_64.pixels");
    EXPECT_EQ(img.width, 64u);
    EXPECT_EQ(img.height, 64u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_GE(RgbaPsnr(img.pixels, exp), 50.0);
}

TEST(JpxRecon, MultiTile53ByteExact) {
    // 32x32 RGB, 5/3 reversible + RCT, 2x2 tiles (16x16). Exercises tile-part
    // assembly, per-tile decode, and canvas compositing — Annex B.3. Lossless,
    // so it must match the OpenJPEG-oracle sidecar byte-for-byte.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "multitile_rgb_32.j2k"));
    auto exp = ReadFile(JpxFixtureRoot() / "multitile_rgb_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_TRUE(std::equal(img.pixels.begin(), img.pixels.end(), exp.begin()));
}

TEST(JpxRecon, Cmyk97Psnr) {
    // 32x32 4-component CMYK, 9/7 irreversible + MCT on components 0-2, RLCP,
    // single tile / single layer. Decodes to RGBA via inverse ICT (0-2) +
    // CMYK->RGB (Annex E dequant, Annex G ICT, OpenJPEG color_cmyk_to_rgb).
    // Lossy (9/7 + CMYK product): gated by PSNR against the OpenJPEG sidecar.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "cmyk_9x7_32.jp2"));
    auto exp = ReadFile(JpxFixtureRoot() / "cmyk_9x7_32.pixels");
    EXPECT_EQ(img.width, 32u);
    EXPECT_EQ(img.height, 32u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_GE(RgbaPsnr(img.pixels, exp), 50.0);  // measured ~64 dB
}

TEST(JpxRecon, CmykLayers97Psnr) {
    // 160x160 CMYK, 9/7, RLCP, 3 quality layers. The 160px finest-detail
    // subbands exceed the 64px code-block, so subbands hold several code-blocks
    // (exercises the hierarchical tag tree); the 3 layers exercise cross-layer
    // pass/segment accumulation (Annex B.10/B.12).
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "cmyk_layers_9x7_160.jp2"));
    auto exp = ReadFile(JpxFixtureRoot() / "cmyk_layers_9x7_160.pixels");
    EXPECT_EQ(img.width, 160u);
    EXPECT_EQ(img.height, 160u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_GE(RgbaPsnr(img.pixels, exp), 45.0);
}

TEST(JpxRecon, CmykTiledLayers97Psnr) {
    // 96x96 CMYK, 9/7, RLCP, 2x2 tiles (48px), 3 quality layers — the page-1
    // background shape in miniature: tile-part assembly + multi-tile compositing
    // + multi-layer accumulation + CMYK->RGB together.
    auto img = foundation::jpx::Decode(ReadFile(JpxFixtureRoot() / "cmyk_tiled_layers_9x7_96.jp2"));
    auto exp = ReadFile(JpxFixtureRoot() / "cmyk_tiled_layers_9x7_96.pixels");
    EXPECT_EQ(img.width, 96u);
    EXPECT_EQ(img.height, 96u);
    EXPECT_EQ(img.components, 4u);
    ASSERT_EQ(img.pixels.size(), exp.size());
    EXPECT_GE(RgbaPsnr(img.pixels, exp), 45.0);
}
