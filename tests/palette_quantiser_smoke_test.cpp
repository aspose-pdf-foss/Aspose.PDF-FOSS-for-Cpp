// =============================================================================
// palette_quantiser_smoke_test — exercises the median-cut palette
// quantiser foundation primitive against synthetic RGBA8 inputs.
// =============================================================================

#include "palette_quantiser.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstddef>

namespace {

// Build a width × height RGBA8 buffer filled with a single colour.
std::vector<std::uint8_t> SolidRGBA(std::uint32_t w, std::uint32_t h,
                                    std::uint8_t r, std::uint8_t g,
                                    std::uint8_t b) {
    std::vector<std::uint8_t> out(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < w * h; ++i) {
        out[i * 4 + 0] = r;
        out[i * 4 + 1] = g;
        out[i * 4 + 2] = b;
        out[i * 4 + 3] = 255;
    }
    return out;
}

}  // namespace

TEST(PaletteQuantiserSmoke, RejectsTooFewColors) {
    auto pix = SolidRGBA(2, 2, 128, 0, 0);
    EXPECT_THROW(
        foundation::palette_quantiser::Quantise(pix, 2, 2, /*max_colors=*/1),
        std::invalid_argument);
}

TEST(PaletteQuantiserSmoke, RejectsTooManyColors) {
    auto pix = SolidRGBA(2, 2, 128, 0, 0);
    EXPECT_THROW(
        foundation::palette_quantiser::Quantise(pix, 2, 2, /*max_colors=*/512),
        std::invalid_argument);
}

TEST(PaletteQuantiserSmoke, SolidColor_OneEntryPalette) {
    auto pix = SolidRGBA(8, 8, 220, 64, 32);
    auto out = foundation::palette_quantiser::Quantise(pix, 8, 8, 16);
    // Median-cut on a single colour ends with one bucket spanning
    // every pixel; further buckets can't split (size < 2 OR
    // extent == 0). Palette must be exactly 1 entry equal to the
    // input colour.
    ASSERT_EQ(out.palette.size(), 1u);
    EXPECT_EQ(out.palette[0][0], 220u);
    EXPECT_EQ(out.palette[0][1], 64u);
    EXPECT_EQ(out.palette[0][2], 32u);
    EXPECT_EQ(out.indices.size(), 64u);
    for (auto idx : out.indices) EXPECT_EQ(idx, 0u);
}

TEST(PaletteQuantiserSmoke, TwoColors_TwoEntryPalette) {
    // Half red, half blue.
    std::vector<std::uint8_t> pix(4 * 4 * 4);
    for (std::size_t i = 0; i < 16; ++i) {
        const bool is_red = (i % 2) == 0;
        pix[i * 4 + 0] = is_red ? 255 : 0;
        pix[i * 4 + 1] = 0;
        pix[i * 4 + 2] = is_red ? 0 : 255;
        pix[i * 4 + 3] = 255;
    }
    auto out = foundation::palette_quantiser::Quantise(pix, 4, 4, 16);
    EXPECT_EQ(out.palette.size(), 2u);
    EXPECT_EQ(out.indices.size(), 16u);
    // Every input pixel maps to one of the two palette entries; the
    // two indices observed must be 0 and 1.
    bool saw_zero = false, saw_one = false;
    for (auto idx : out.indices) {
        if (idx == 0) saw_zero = true;
        if (idx == 1) saw_one = true;
    }
    EXPECT_TRUE(saw_zero);
    EXPECT_TRUE(saw_one);
}

TEST(PaletteQuantiserSmoke, GradientHonoursMaxColors) {
    // 256-step gradient. Asking for 16 palette entries must produce
    // a 16-entry palette, not 256.
    std::vector<std::uint8_t> pix(256 * 4);
    for (std::size_t i = 0; i < 256; ++i) {
        pix[i * 4 + 0] = static_cast<std::uint8_t>(i);
        pix[i * 4 + 1] = static_cast<std::uint8_t>(i);
        pix[i * 4 + 2] = static_cast<std::uint8_t>(i);
        pix[i * 4 + 3] = 255;
    }
    auto out = foundation::palette_quantiser::Quantise(pix, 256, 1, 16);
    EXPECT_EQ(out.palette.size(), 16u);
    EXPECT_EQ(out.indices.size(), 256u);
}
