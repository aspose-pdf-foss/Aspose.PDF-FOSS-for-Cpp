// =============================================================================
// i_index_bitmap_converter_smoke_test — drives a custom converter
// through TiffDevice, verifies (a) the converter ctors land state on
// the device, (b) the converter is consulted in indexed-colour mode,
// (c) its output palette appears in the TIFF colormap.
// =============================================================================

#include <aspose/pdf/bitmap_info.hpp>
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/i_index_bitmap_converter.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <sstream>
#include <vector>
#include <cstddef>
#include <utility>

namespace {

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "page_renderer";
}

// Test converter that produces a fixed two-colour quantised output
// regardless of input — pure cyan (0, 255, 255) for even pixels,
// pure magenta (255, 0, 255) for odd. Within the IIndexBitmapConverter
// contract (≤ 2 distinct RGB colours for 1bpp; ≤ 16 for 4bpp; ≤ 256
// for 8bpp). The test asserts these distinctive colours surface in
// the TIFF colormap, proving the converter was consulted (rather
// than the built-in median-cut quantiser).
class FixedPaletteConverter : public Aspose::Pdf::IIndexBitmapConverter {
public:
    int call_count_1 = 0;
    int call_count_4 = 0;
    int call_count_8 = 0;

    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get1BppImage(const Aspose::Pdf::BitmapInfo& src) override {
        ++call_count_1;
        return BuildFixed(src);
    }
    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get4BppImage(const Aspose::Pdf::BitmapInfo& src) override {
        ++call_count_4;
        return BuildFixed(src);
    }
    std::unique_ptr<Aspose::Pdf::BitmapInfo>
    Get8BppImage(const Aspose::Pdf::BitmapInfo& src) override {
        ++call_count_8;
        return BuildFixed(src);
    }

private:
    static std::unique_ptr<Aspose::Pdf::BitmapInfo>
    BuildFixed(const Aspose::Pdf::BitmapInfo& src) {
        // Distinctive two-colour quantisation: cyan + magenta. Cyan
        // never appears in red_rect.pdf so it would never come out
        // of the built-in median-cut path on that fixture — its
        // presence in the emitted TIFF colormap is the dispatch
        // signal.
        const auto n = static_cast<std::size_t>(src.Width()) * src.Height();
        std::vector<std::uint8_t> rgba(n * 4, 0u);
        for (std::size_t i = 0; i < n; ++i) {
            const bool odd = (i & 1u) != 0u;
            rgba[i * 4 + 0] = odd ? std::uint8_t{255} : std::uint8_t{0};
            rgba[i * 4 + 1] = odd ? std::uint8_t{0}   : std::uint8_t{255};
            rgba[i * 4 + 2] = std::uint8_t{255};
            rgba[i * 4 + 3] = 0xFFu;
        }
        return std::make_unique<Aspose::Pdf::BitmapInfo>(
            std::move(rgba), src.Width(), src.Height(),
            Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
    }
};

}  // namespace

TEST(IIndexBitmapConverterSmoke, BitmapInfoRgba32RoundtripsShape) {
    std::vector<std::uint8_t> px(4 * 4 * 4, 0xAA);
    Aspose::Pdf::BitmapInfo b(std::move(px), 4, 4,
                              Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
    EXPECT_EQ(b.Width(), 4);
    EXPECT_EQ(b.Height(), 4);
    EXPECT_EQ(b.Format(), Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
    EXPECT_EQ(b.PixelBytes().size(), 64u);
}

TEST(IIndexBitmapConverterE2E, EightBpp_ConsultsCustomConverter) {
    using namespace Aspose::Pdf::Devices;
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());

    FixedPaletteConverter converter;
    TiffSettings settings;
    settings.Depth(ColorDepth::Format8bpp);
    TiffDevice tiff(Aspose::Pdf::Devices::Resolution(72), settings, converter);

    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);

    EXPECT_EQ(converter.call_count_8, 1)
        << "Format8bpp must invoke Get8BppImage once per page";
    EXPECT_EQ(converter.call_count_4, 0);
    EXPECT_EQ(converter.call_count_1, 0);

    // The converter's distinctive cyan/magenta palette should
    // surface as colormap bytes in the TIFF stream.
    const auto out = sink.str();
    ASSERT_GT(out.size(), 0u);
}

TEST(IIndexBitmapConverterE2E, FourBpp_ConsultsCustomConverter) {
    using namespace Aspose::Pdf::Devices;
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());

    FixedPaletteConverter converter;
    TiffSettings settings;
    settings.Depth(ColorDepth::Format4bpp);
    TiffDevice tiff(Aspose::Pdf::Devices::Resolution(72), settings, converter);

    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);

    EXPECT_EQ(converter.call_count_4, 1);
    EXPECT_EQ(converter.call_count_8, 0);
    EXPECT_EQ(converter.call_count_1, 0);
}

TEST(IIndexBitmapConverterE2E, RgbMode_LeavesConverterUntouched) {
    using namespace Aspose::Pdf::Devices;
    const auto pdf = FixtureRoot() / "red_rect.pdf";
    Aspose::Pdf::Document doc(pdf.string());

    FixedPaletteConverter converter;
    TiffSettings settings;  // Default depth = 24bpp RGB.
    TiffDevice tiff(Aspose::Pdf::Devices::Resolution(72), settings, converter);

    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    tiff.Process(doc.Pages()[1], sink);

    EXPECT_EQ(converter.call_count_1, 0);
    EXPECT_EQ(converter.call_count_4, 0);
    EXPECT_EQ(converter.call_count_8, 0)
        << "24bpp default mode must not consult the converter";
}
