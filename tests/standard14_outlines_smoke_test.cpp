// =============================================================================
// standard14_outlines_smoke_test — exercises the Standard14 PostScript-
// name → TrueType bytes resolver (foundation::standard14_outlines).
// Bundled Liberation lookups, filesystem probe, candidate fallthrough.
// =============================================================================

#include "standard14_outlines.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstddef>
#include <span>

namespace {

bool LooksLikeTrueType(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4) return false;
    // sfnt magic: 00 01 00 00 (TTF) or 4F 54 54 4F (OTTO)
    return (bytes[0] == 0x00 && bytes[1] == 0x01 &&
            bytes[2] == 0x00 && bytes[3] == 0x00) ||
           (bytes[0] == 0x4F && bytes[1] == 0x54 &&
            bytes[2] == 0x54 && bytes[3] == 0x4F);
}

bool LooksLikeTrueTypeByte(std::span<const std::byte> bytes) {
    if (bytes.size() < 4) return false;
    auto b0 = static_cast<unsigned char>(bytes[0]);
    auto b1 = static_cast<unsigned char>(bytes[1]);
    auto b2 = static_cast<unsigned char>(bytes[2]);
    auto b3 = static_cast<unsigned char>(bytes[3]);
    return (b0 == 0x00 && b1 == 0x01 && b2 == 0x00 && b3 == 0x00) ||
           (b0 == 0x4F && b1 == 0x54 && b2 == 0x54 && b3 == 0x4F);
}

}  // namespace

TEST(Standard14OutlinesSmoke, BundledHelvetica_IsTrueType) {
    auto bytes = foundation::standard14_outlines::Bundled("Helvetica");
    ASSERT_GT(bytes.size(), 4u);
    EXPECT_TRUE(LooksLikeTrueType(bytes))
        << "bundled Helvetica must start with sfnt magic";
}

TEST(Standard14OutlinesSmoke, AllTwelveBundleEntriesPresent) {
    using ::foundation::standard14_outlines::Bundled;
    for (auto name : {"Helvetica", "Helvetica-Bold",
                      "Helvetica-Oblique", "Helvetica-BoldOblique",
                      "Times-Roman", "Times-Bold",
                      "Times-Italic", "Times-BoldItalic",
                      "Courier", "Courier-Bold",
                      "Courier-Oblique", "Courier-BoldOblique"}) {
        auto bytes = Bundled(name);
        ASSERT_GT(bytes.size(), 4u) << "bundle missing: " << name;
        EXPECT_TRUE(LooksLikeTrueType(bytes)) << "bundle invalid: " << name;
    }
}

TEST(Standard14OutlinesSmoke, BundledSymbol_NotProvided) {
    // Symbol + ZapfDingbats are deferred — the bundle has no
    // entry. Bundled() returns empty span.
    EXPECT_TRUE(foundation::standard14_outlines::Bundled("Symbol").empty());
    EXPECT_TRUE(foundation::standard14_outlines::Bundled("ZapfDingbats").empty());
}

TEST(Standard14OutlinesSmoke, UnknownName_ReturnsEmpty) {
    EXPECT_TRUE(foundation::standard14_outlines::Bundled("NotAFont").empty());
}

TEST(Standard14OutlinesSmoke, ResolveFallsBackToBundle) {
    // Empty search dirs → must fall through to bundled Liberation.
    std::vector<std::string> dirs;
    auto bytes = foundation::standard14_outlines::Resolve(
        "Helvetica", std::span<const std::string>(dirs));
    ASSERT_GT(bytes.size(), 4u);
    EXPECT_TRUE(LooksLikeTrueTypeByte(bytes));
}

TEST(Standard14OutlinesSmoke, ResolvePicksFromTempDir) {
    // Stage a temp dir that contains a fake but valid TTF named
    // exactly LiberationSans-Regular.ttf — Resolve must pick it
    // rather than fall through to the bundled Helvetica. We
    // distinguish the two by size: our planted file has a
    // distinctive size that doesn't match the bundle.
    namespace fs = std::filesystem;
    auto tmp = fs::temp_directory_path() /
               "aspose_std14_test_dir";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    // Plant a minimal valid-looking sfnt header (4-byte magic +
    // padding to 200 bytes of zeros). This isn't a real font;
    // the test asserts the right BYTES come back, not that they
    // parse.
    const std::vector<std::uint8_t> planted = [] {
        std::vector<std::uint8_t> v(200, 0);
        v[0] = 0x00; v[1] = 0x01; v[2] = 0x00; v[3] = 0x00;
        return v;
    }();
    {
        std::ofstream out(tmp / "LiberationSans-Regular.ttf",
                          std::ios::binary);
        out.write(reinterpret_cast<const char*>(planted.data()),
                  static_cast<std::streamsize>(planted.size()));
    }

    std::vector<std::string> dirs = {tmp.string()};
    auto bytes = foundation::standard14_outlines::Resolve(
        "Helvetica", std::span<const std::string>(dirs));
    EXPECT_EQ(bytes.size(), planted.size())
        << "Resolve must return the planted file, not the bundle";

    fs::remove_all(tmp);
}

TEST(Standard14OutlinesSmoke, DefaultSearchDirsNonEmpty) {
    auto dirs = foundation::standard14_outlines::DefaultSearchDirs();
    EXPECT_FALSE(dirs.empty())
        << "every supported OS must declare at least one default dir";
}

// Canonicalize() maps an arbitrary /BaseFont name to one of the 12
// resolvable Standard14 names so non-embedded non-Standard14 fonts
// (Arial, TrebuchetMS, TimesNewRoman, Courier New, …) substitute to a
// metric class instead of rendering blank (field reproducer 33300-2).
TEST(Standard14OutlinesSmoke, CanonicalizeFamilyAndStyle) {
    using foundation::standard14_outlines::Canonicalize;
    // Sans family → Helvetica.
    EXPECT_EQ(Canonicalize("Arial"), "Helvetica");
    EXPECT_EQ(Canonicalize("ArialMT"), "Helvetica");
    EXPECT_EQ(Canonicalize("TrebuchetMS"), "Helvetica");
    EXPECT_EQ(Canonicalize("Verdana"), "Helvetica");
    EXPECT_EQ(Canonicalize("Calibri"), "Helvetica");
    // Style tokens (comma / dash / words) pick the variant.
    EXPECT_EQ(Canonicalize("Arial,Bold"), "Helvetica-Bold");
    EXPECT_EQ(Canonicalize("Arial-BoldItalic"), "Helvetica-BoldOblique");
    EXPECT_EQ(Canonicalize("Arial,Italic"), "Helvetica-Oblique");
    // Serif family → Times-Roman.
    EXPECT_EQ(Canonicalize("TimesNewRoman"), "Times-Roman");
    EXPECT_EQ(Canonicalize("TimesNewRomanPS-BoldMT"), "Times-Bold");
    EXPECT_EQ(Canonicalize("Georgia"), "Times-Roman");
    // Monospace family → Courier.
    EXPECT_EQ(Canonicalize("CourierNew"), "Courier");
    EXPECT_EQ(Canonicalize("Consolas-Bold"), "Courier-Bold");
    // Subset prefix is stripped.
    EXPECT_EQ(Canonicalize("ABCDEF+Arial,Bold"), "Helvetica-Bold");
    // Exact Standard14 names pass through.
    EXPECT_EQ(Canonicalize("Helvetica"), "Helvetica");
    EXPECT_EQ(Canonicalize("Times-Roman"), "Times-Roman");
    EXPECT_EQ(Canonicalize("Courier-BoldOblique"), "Courier-BoldOblique");
    // Unknown family: sans default, or serif when prefer_serif is set.
    EXPECT_EQ(Canonicalize("WingbatThing"), "Helvetica");
    EXPECT_EQ(Canonicalize("WingbatThing", /*prefer_serif=*/true),
              "Times-Roman");
}
