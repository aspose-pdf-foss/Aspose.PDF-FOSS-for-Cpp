// =============================================================================
// ccitt_encode_smoke_test — foundation::ccitt::Encode (T.6 Group 4 + T.4
// Group 3 1D). Validated by round-tripping through the independently-written
// decoder: Decode(Encode(raster)) == raster, across patterns, sizes, and
// both BlackIs1 polarities. Mirrors pdflib's CcittEncode test intentions.
// =============================================================================

#include "ccitt.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

#include <gtest/gtest.h>

namespace {

using foundation::ccitt::DecodedImage;
using foundation::ccitt::Decode;
using foundation::ccitt::Encode;
using foundation::ccitt::Params;

// Pack a row-major bool grid (true = black in the default BlackIs1=false
// sense, i.e. raster bit 1) into MSB-first 1bpp bytes.
DecodedImage MakeImage(int cols, int rows,
                       const std::vector<bool>& black) {
    DecodedImage img;
    img.width = static_cast<std::uint32_t>(cols);
    img.height = static_cast<std::uint32_t>(rows);
    const int bpr = (cols + 7) / 8;
    img.bits.assign(static_cast<std::size_t>(bpr) * rows, std::byte{0});
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (black[static_cast<std::size_t>(r) * cols + c]) {
                img.bits[static_cast<std::size_t>(r) * bpr + c / 8] |=
                    std::byte{static_cast<std::uint8_t>(1u << (7 - (c % 8)))};
            }
        }
    }
    return img;
}

// Encode then decode; assert the raster survives byte-for-byte.
void ExpectRoundTrip(const DecodedImage& img, int K, bool black_is_1) {
    Params ep;
    ep.K = static_cast<std::int8_t>(K);
    ep.Columns = img.width;
    ep.Rows = img.height;
    ep.BlackIs1 = black_is_1;
    ep.EndOfBlock = true;
    const auto enc = Encode(img, ep);

    Params dp = ep;
    const auto dec = Decode(
        std::span<const std::byte>(enc.data(), enc.size()), dp);

    ASSERT_EQ(dec.width, img.width);
    ASSERT_EQ(dec.height, img.height);
    ASSERT_EQ(dec.bits.size(), img.bits.size());
    for (std::size_t i = 0; i < img.bits.size(); ++i) {
        ASSERT_EQ(dec.bits[i], img.bits[i]) << "byte " << i;
    }
}

std::vector<bool> Checkerboard(int cols, int rows, int cell) {
    std::vector<bool> b(static_cast<std::size_t>(cols) * rows);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            b[static_cast<std::size_t>(r) * cols + c] =
                ((r / cell) + (c / cell)) % 2 == 0;
    return b;
}

}  // namespace

TEST(CcittEncode, G4AllWhite) {
    auto img = MakeImage(8, 1, std::vector<bool>(8, false));
    ExpectRoundTrip(img, -1, false);
}

TEST(CcittEncode, G4AllBlack) {
    auto img = MakeImage(8, 1, std::vector<bool>(8, true));
    ExpectRoundTrip(img, -1, false);
}

TEST(CcittEncode, G4Alternating) {
    std::vector<bool> px = {true, false, true, false,
                            true, false, true, false};
    ExpectRoundTrip(MakeImage(8, 1, px), -1, false);
}

TEST(CcittEncode, G4TwoIdenticalRows) {
    std::vector<bool> px = {true, true, false, false, true, true, false, false,
                            true, true, false, false, true, true, false, false};
    ExpectRoundTrip(MakeImage(8, 2, px), -1, false);
}

TEST(CcittEncode, G4Checkerboard64) {
    auto img = MakeImage(64, 64, Checkerboard(64, 64, 8));
    ExpectRoundTrip(img, -1, false);
}

TEST(CcittEncode, G4SingleRowHalfWhiteHalfBlack) {
    std::vector<bool> px(16, false);
    for (int c = 8; c < 16; ++c) px[c] = true;  // 8 white then 8 black
    ExpectRoundTrip(MakeImage(16, 1, px), -1, false);
}

TEST(CcittEncode, G4LongRunsBeyond1728) {
    // A 2000-px-wide row exercises make-up + common make-up chaining.
    std::vector<bool> px(2000, false);
    for (int c = 1900; c < 2000; ++c) px[c] = true;
    ExpectRoundTrip(MakeImage(2000, 1, px), -1, false);
}

TEST(CcittEncode, G4BlackIs1Polarity) {
    auto img = MakeImage(32, 4, Checkerboard(32, 4, 4));
    ExpectRoundTrip(img, -1, true);
}

TEST(CcittEncode, G3AllWhite) {
    ExpectRoundTrip(MakeImage(8, 1, std::vector<bool>(8, false)), 0, false);
}

TEST(CcittEncode, G3AllBlack) {
    ExpectRoundTrip(MakeImage(8, 1, std::vector<bool>(8, true)), 0, false);
}

TEST(CcittEncode, G3Checkerboard) {
    ExpectRoundTrip(MakeImage(64, 16, Checkerboard(64, 16, 8)), 0, false);
}

TEST(CcittEncode, G3BlackIs1Polarity) {
    ExpectRoundTrip(MakeImage(40, 5, Checkerboard(40, 5, 5)), 0, true);
}

// Random sweep across sizes/patterns for both modes and polarities.
TEST(CcittEncode, RandomSweepRoundTrips) {
    std::mt19937 rng(1234);
    for (int K : {-1, 0}) {
        for (bool b1 : {false, true}) {
            for (int t = 0; t < 60; ++t) {
                const int cols = 1 + static_cast<int>(rng() % 300);
                const int rows = 1 + static_cast<int>(rng() % 16);
                std::vector<bool> px(static_cast<std::size_t>(cols) * rows);
                for (auto&& bit : px) bit = (rng() & 1) != 0;
                ExpectRoundTrip(MakeImage(cols, rows, px), K, b1);
            }
        }
    }
}

TEST(CcittEncode, RejectsKGreaterThanZero) {
    auto img = MakeImage(8, 1, std::vector<bool>(8, false));
    Params ep;
    ep.K = 2;
    ep.Columns = 8;
    ep.Rows = 1;
    EXPECT_THROW(Encode(img, ep), std::runtime_error);
}
