#include <aspose/pdf/tiff_device.hpp>

#include <aspose/pdf/bitmap_info.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/i_index_bitmap_converter.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/page_size.hpp>

#include "binarize_bradley.hpp"
#include "page_renderer.hpp"
#include "palette_quantiser.hpp"
#include "tiff_decoder.hpp"
#include "tiff_encoder.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <span>
#include <stdexcept>
#include <vector>

namespace Aspose::Pdf::Devices {

namespace {

// ---------------------------------------------------------------------------
// Public TiffDevice: page render → palette quantise (when palette
// mode) → foundation::tiff_encoder::Encode. The TIFF byte-format
// writer used to live inline here (~580 LOC) and shipped 2026-05-03;
// `foundation::tiff_encoder` was extended to cover 1bpp BlackIsZero
// (luma-thresholded) + 4bpp Palette so this layer became a thin
// delegate.
// ---------------------------------------------------------------------------

// Translate the public CompressionType enum to foundation's
// Compression enum class. CCITT3/CCITT4 raise (no
// foundation::ccitt::Encode primitive yet).
[[nodiscard]] foundation::tiff_encoder::Compression
MapCompression(CompressionType c) {
    using foundation::tiff_encoder::Compression;
    switch (c) {
        case CompressionType::None: return Compression::None;
        case CompressionType::LZW:  return Compression::Lzw;
        case CompressionType::RLE:  return Compression::PackBits;
        case CompressionType::CCITT3:
        case CompressionType::CCITT4:
            throw std::runtime_error(
                "TiffDevice: CCITT compression for TIFF encoding "
                "is not supported in the FOSS build (no "
                "foundation::ccitt::Encode primitive). Use "
                "CompressionType::LZW, RLE, or None.");
    }
    return Compression::None;
}

// Map the public ColorDepth enum to foundation tiff_encoder's
// (Photometric, samples_per_pixel, bits_per_sample) tuple.
struct EncodingShape {
    foundation::tiff_encoder::Photometric photometric;
    std::uint32_t samples_per_pixel;
    std::uint32_t bits_per_sample;
};

[[nodiscard]] EncodingShape ShapeFromDepth(ColorDepth depth) {
    using foundation::tiff_encoder::Photometric;
    switch (depth) {
        case ColorDepth::Default:
        case ColorDepth::Format24bpp:
            return {Photometric::Rgb, 3, 8};
        case ColorDepth::Format8bpp:
            return {Photometric::Palette, 1, 8};
        case ColorDepth::Format4bpp:
            return {Photometric::Palette, 1, 4};
        case ColorDepth::Format1bpp:
            return {Photometric::BlackIsZero, 1, 1};
    }
    throw std::runtime_error("TiffDevice: unknown ColorDepth");
}

// Build a 16-bit ColorMap (3 * map_size SHORTs, all reds then
// greens then blues) from a palette_quantiser result. Each
// channel byte is scaled to 0..65535 via the canonical *257
// widening (so the high byte of each SHORT is exactly the
// 8-bit channel value, matching tiff_decoder's `entry >> 8`).
[[nodiscard]] std::vector<std::uint16_t> BuildColorMap(
        const foundation::palette_quantiser::IndexedImage& idx,
        std::size_t map_size) {
    std::vector<std::uint16_t> map(map_size * 3, 0);
    for (std::size_t i = 0; i < idx.palette.size() && i < map_size; ++i) {
        const auto& e = idx.palette[i];
        map[0 * map_size + i] = static_cast<std::uint16_t>(e[0] * 257);
        map[1 * map_size + i] = static_cast<std::uint16_t>(e[1] * 257);
        map[2 * map_size + i] = static_cast<std::uint16_t>(e[2] * 257);
    }
    return map;
}

// One page's foundation::tiff_encoder::Page input plus an owned
// pixel buffer (so the Page's std::span stays valid through
// Encode) plus a per-page color_map (palette modes only).
struct PreparedPage {
    std::vector<std::byte> rgba_owned;
    std::vector<std::uint16_t> color_map;  // empty unless palette
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Run page_renderer + (palette_quantiser if palette-mode) + RGBA
// shape preparation. Returns a PreparedPage whose rgba_owned is
// either:
//   * For 1bpp / 24bpp / 32bpp: the rendered RGBA8 directly.
//   * For 4bpp / 8bpp palette: an RGBA8 buffer where R = palette
//     index, G = B = 0, A = 0xFF — the foundation primitive reads
//     R as the palette index for palette modes (G/B/A ignored).
[[nodiscard]] PreparedPage PreparePage(
        const foundation::page_renderer::RenderedPage& rendered,
        ColorDepth depth,
        ::Aspose::Pdf::IIndexBitmapConverter* converter) {
    PreparedPage out;
    out.width = rendered.width;
    out.height = rendered.height;
    const std::size_t pixel_count =
        static_cast<std::size_t>(rendered.width) * rendered.height;

    if (depth == ColorDepth::Format1bpp ||
        depth == ColorDepth::Format24bpp ||
        depth == ColorDepth::Default) {
        // Foundation does luma threshold (1bpp) or alpha drop
        // (24bpp) internally — pass rendered RGBA8 through.
        out.rgba_owned.assign(
            reinterpret_cast<const std::byte*>(rendered.pixels.data()),
            reinterpret_cast<const std::byte*>(
                rendered.pixels.data() + rendered.pixels.size()));
        return out;
    }

    // Palette modes: quantise to 16 / 256 colors, then build an
    // RGBA buffer where R = index for the foundation encoder.
    const bool four_bit = (depth == ColorDepth::Format4bpp);
    const std::uint32_t max_colors = four_bit ? 16u : 256u;

    foundation::palette_quantiser::IndexedImage idx;
    if (converter) {
        // Build a BitmapInfo (Rgba32) input for the caller's
        // converter; the canonical .NET surface signature takes
        // System.Drawing.Bitmap, which the cpp port substitutes
        // with the cross-platform flat POD BitmapInfo.
        ::Aspose::Pdf::BitmapInfo input(
            std::vector<std::uint8_t>(
                rendered.pixels.begin(), rendered.pixels.end()),
            static_cast<int>(rendered.width),
            static_cast<int>(rendered.height),
            ::Aspose::Pdf::BitmapInfo::PixelFormat::Rgba32);
        auto bm = four_bit
            ? converter->Get4BppImage(input)
            : converter->Get8BppImage(input);
        if (!bm) {
            throw std::runtime_error(
                "TiffDevice: IIndexBitmapConverter returned null");
        }

        // Reconstruct palette + per-pixel indices by bucketing
        // unique RGB triples from the converter's quantised RGBA
        // output. Throws if the converter exceeds max_colors
        // distinct colours (a contract violation).
        const auto& out_pixels = bm->PixelBytes();
        const std::size_t out_count = pixel_count;
        if (out_pixels.size() < out_count * 4) {
            throw std::runtime_error(
                "TiffDevice: IIndexBitmapConverter returned a "
                "BitmapInfo with insufficient pixel bytes for "
                "the declared dimensions");
        }
        idx.width = rendered.width;
        idx.height = rendered.height;
        idx.indices.resize(out_count);
        for (std::size_t i = 0; i < out_count; ++i) {
            std::array<std::uint8_t, 3> rgb{
                out_pixels[i * 4 + 0],
                out_pixels[i * 4 + 1],
                out_pixels[i * 4 + 2],
            };
            auto it = std::find(idx.palette.begin(),
                                idx.palette.end(), rgb);
            if (it == idx.palette.end()) {
                if (idx.palette.size() >= max_colors) {
                    throw std::runtime_error(
                        "TiffDevice: IIndexBitmapConverter "
                        "returned a BitmapInfo with more than "
                        "max_colors distinct RGB triples");
                }
                idx.palette.push_back(rgb);
                idx.indices[i] = static_cast<std::uint8_t>(
                    idx.palette.size() - 1);
            } else {
                idx.indices[i] = static_cast<std::uint8_t>(
                    std::distance(idx.palette.begin(), it));
            }
        }
    } else {
        idx = foundation::palette_quantiser::Quantise(
            std::span<const std::uint8_t>(
                rendered.pixels.data(),
                rendered.pixels.size()),
            rendered.width, rendered.height, max_colors);
    }

    out.color_map = BuildColorMap(
        idx, four_bit ? 16u : 256u);

    // Build RGBA wrapper: R = index, G = B = 0, A = 0xFF.
    out.rgba_owned.assign(pixel_count * 4, std::byte{0});
    for (std::size_t i = 0; i < pixel_count; ++i) {
        out.rgba_owned[i * 4 + 0] =
            static_cast<std::byte>(idx.indices[i]);
        out.rgba_owned[i * 4 + 3] = std::byte{0xFFu};
    }
    return out;
}

// Encode `prepared` pages into a single (possibly multi-page)
// TIFF byte stream via foundation::tiff_encoder::Encode.
[[nodiscard]] std::vector<std::byte> EncodePages(
        const std::vector<PreparedPage>& prepared,
        ColorDepth depth,
        CompressionType compression,
        std::uint32_t dpi) {
    const auto shape = ShapeFromDepth(depth);

    std::vector<foundation::tiff_encoder::Page> pages;
    pages.reserve(prepared.size());
    for (const auto& p : prepared) {
        pages.push_back(foundation::tiff_encoder::Page{
            .width = p.width,
            .height = p.height,
            .rgba = std::span<const std::byte>(
                p.rgba_owned.data(), p.rgba_owned.size()),
        });
    }

    foundation::tiff_encoder::Options options{};
    options.compression = MapCompression(compression);
    options.photometric = shape.photometric;
    options.samples_per_pixel = shape.samples_per_pixel;
    options.bits_per_sample = shape.bits_per_sample;
    options.dpi = dpi;
    if (shape.photometric ==
            foundation::tiff_encoder::Photometric::Palette) {
        // Palette modes: every prepared page's color_map matches.
        // foundation requires color_map to be non-empty for
        // Palette photometric; pass the first page's map.
        options.color_map = std::span<const std::uint16_t>(
            prepared.front().color_map.data(),
            prepared.front().color_map.size());
    }

    return foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(
            pages.data(), pages.size()),
        options);
}

// Stream-out helper — write encoded TIFF bytes to an ostream.
void EmitTo(std::ostream& output, std::span<const std::byte> bytes) {
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
}

}  // namespace

// ---------------------------------------------------------------------------
// 13 ctors (5 IIndexBitmapConverter-taking ones commented out at anchor)
// ---------------------------------------------------------------------------

TiffDevice::TiffDevice() = default;

TiffDevice::TiffDevice(const class Resolution& resolution)
    : resolution_(resolution) {}

TiffDevice::TiffDevice(const class Resolution& resolution,
                       const class TiffSettings& settings)
    : resolution_(resolution), settings_(settings) {}

TiffDevice::TiffDevice(const class TiffSettings& settings)
    : settings_(settings) {}

TiffDevice::TiffDevice(int width, int height,
                       const class Resolution& resolution,
                       const class TiffSettings& settings)
    : width_(width), height_(height),
      resolution_(resolution), settings_(settings) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class Resolution& resolution,
                       const class TiffSettings& settings)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())),
      resolution_(resolution), settings_(settings) {}

TiffDevice::TiffDevice(int width, int height,
                       const class Resolution& resolution)
    : width_(width), height_(height), resolution_(resolution) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class Resolution& resolution)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())),
      resolution_(resolution) {}

TiffDevice::TiffDevice(int width, int height,
                       const class TiffSettings& settings)
    : width_(width), height_(height), settings_(settings) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class TiffSettings& settings)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())),
      settings_(settings) {}

TiffDevice::TiffDevice(int width, int height)
    : width_(width), height_(height) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())) {}

// Caller-pluggable IIndexBitmapConverter overloads — same as
// the non-converter ctors but stash a non-owning pointer the
// indexed-mode encoding path consults instead of the built-in
// median-cut quantiser.

TiffDevice::TiffDevice(const class Resolution& resolution,
                       const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : resolution_(resolution), settings_(settings),
      converter_(&converter) {}

TiffDevice::TiffDevice(const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : settings_(settings), converter_(&converter) {}

TiffDevice::TiffDevice(int width, int height,
                       const class Resolution& resolution,
                       const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : width_(width), height_(height),
      resolution_(resolution), settings_(settings),
      converter_(&converter) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class Resolution& resolution,
                       const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())),
      resolution_(resolution), settings_(settings),
      converter_(&converter) {}

TiffDevice::TiffDevice(int width, int height,
                       const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : width_(width), height_(height),
      settings_(settings), converter_(&converter) {}

TiffDevice::TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
                       const class TiffSettings& settings,
                       ::Aspose::Pdf::IIndexBitmapConverter& converter)
    : width_(static_cast<int>(pageSize.Width())),
      height_(static_cast<int>(pageSize.Height())),
      settings_(settings), converter_(&converter) {}

// ---------------------------------------------------------------------------
// Process overrides
// ---------------------------------------------------------------------------

void TiffDevice::Process(const ::Aspose::Pdf::Page& page,
                         std::ostream& output) {
    const auto& bytes = page.doc_->bytes_;
    const double dpi = static_cast<double>(resolution_.X());
    const auto rendered = foundation::page_renderer::Render(
        std::span<const std::byte>(bytes.data(), bytes.size()),
        page.leaf_index_, dpi);

    std::vector<PreparedPage> prepared;
    prepared.push_back(PreparePage(
        rendered, settings_.Depth(), converter_));
    const auto encoded = EncodePages(
        prepared, settings_.Depth(), settings_.Compression(),
        static_cast<std::uint32_t>(resolution_.X()));
    EmitTo(output, std::span<const std::byte>(
        encoded.data(), encoded.size()));
}

void TiffDevice::Process(const ::Aspose::Pdf::Document& document,
                         int fromPage, int toPage,
                         std::ostream& output) {
    auto& pages = const_cast<::Aspose::Pdf::Document&>(document).Pages();
    const auto count = static_cast<int>(pages.Count());
    if (fromPage < 1 || toPage > count || fromPage > toPage) {
        throw std::out_of_range(
            "Aspose::Pdf::Devices::TiffDevice::Process: page range "
            "out of bounds");
    }

    std::vector<PreparedPage> prepared;
    prepared.reserve(static_cast<std::size_t>(toPage - fromPage + 1));
    for (int p = fromPage; p <= toPage; ++p) {
        auto page = pages[p];
        const auto& bytes = page.doc_->bytes_;
        const double dpi = static_cast<double>(resolution_.X());
        const auto rendered = foundation::page_renderer::Render(
            std::span<const std::byte>(bytes.data(), bytes.size()),
            page.leaf_index_, dpi);
        prepared.push_back(PreparePage(
            rendered, settings_.Depth(), converter_));
    }

    const auto encoded = EncodePages(
        prepared, settings_.Depth(), settings_.Compression(),
        static_cast<std::uint32_t>(resolution_.X()));
    EmitTo(output, std::span<const std::byte>(
        encoded.data(), encoded.size()));
}

// ---------------------------------------------------------------------------
// BinarizeBradley — Bradley's adaptive thresholding (2007).
// Input and output are both TIFF byte streams. The input is
// decoded via foundation::tiff_decoder, luminance is collapsed
// to a single channel, an integral-image O(1)-per-pixel local
// mean is computed, the per-pixel decision is luma <
// local_mean * (1 - threshold), and the binarised result is
// emitted as a 1bpp BlackIsZero TIFF.
// ---------------------------------------------------------------------------

void TiffDevice::BinarizeBradley(std::istream& inputImageStream,
                                 std::ostream& outputImageStream,
                                 double threshold) {
    // Slurp the entire input. The TIFF format is random-access by
    // file offset, so a streaming decoder isn't workable; we
    // need the full byte buffer in memory.
    std::vector<char> input_chars(
        (std::istreambuf_iterator<char>(inputImageStream)),
        std::istreambuf_iterator<char>());
    std::vector<std::byte> input_bytes(input_chars.size());
    std::memcpy(input_bytes.data(), input_chars.data(),
                input_chars.size());

    if (input_bytes.empty()) {
        throw std::runtime_error(
            "BinarizeBradley: input TIFF is empty");
    }

    if (threshold < 0.0 || threshold > 1.0) {
        throw std::runtime_error(
            "BinarizeBradley: threshold must be in [0, 1]");
    }

    const auto decoded = foundation::tiff_decoder::Decode(
        std::span<const std::byte>(
            input_bytes.data(), input_bytes.size()));

    const std::uint32_t w = decoded.width;
    const std::uint32_t h = decoded.height;
    const auto* rgba = reinterpret_cast<const std::uint8_t*>(
        decoded.pixels.data());

    // Collapse RGBA to grayscale via the standard ITU-R BT.601
    // luma weights (integer cross-multiplication form with
    // round-half-up via the +500 nudge).
    std::vector<std::byte> grey(
        static_cast<std::size_t>(w) * h);
    for (std::size_t i = 0; i < grey.size(); ++i) {
        const auto rv = rgba[i * 4 + 0];
        const auto gv = rgba[i * 4 + 1];
        const auto bv = rgba[i * 4 + 2];
        grey[i] = std::byte{static_cast<std::uint8_t>(
            (static_cast<unsigned>(rv) * 299u
             + static_cast<unsigned>(gv) * 587u
             + static_cast<unsigned>(bv) * 114u
             + 500u) / 1000u)};
    }

    // Delegate to the foundation integer-only Bradley primitive
    // (shared spec yaml — every target produces byte-identical
    // bilevel output). Window size = max(2, w / 8) per Bradley
    // & Roth 2007; threshold maps to integer percent via
    // round-half-up.
    const std::uint32_t window_size = std::max(2u, w / 8u);
    const auto threshold_percent =
        static_cast<std::uint32_t>(threshold * 100.0 + 0.5);
    const auto bilevel = foundation::binarize_bradley::Apply(
        std::span<const std::byte>(grey.data(), grey.size()),
        w, h, window_size, threshold_percent);

    // Expand bilevel MSB-first → RGBA8 (BLACK = (0,0,0,255);
    // WHITE = (255,255,255,255)) so the existing
    // tiff_encoder::Encode RGBA8 path emits the output.
    const std::uint32_t stride_in = (w + 7) / 8;
    std::vector<std::byte> rgba_binarised(
        static_cast<std::size_t>(w) * h * 4);
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            const auto byte_val = std::to_integer<std::uint8_t>(
                bilevel[y * stride_in + (x >> 3)]);
            const int bit = (byte_val >> (7 - (x & 7))) & 1;
            const auto v = bit == 0
                ? std::byte{0x00}
                : std::byte{0xFFu};
            const std::size_t pi =
                (static_cast<std::size_t>(y) * w + x) * 4;
            rgba_binarised[pi + 0] = v;
            rgba_binarised[pi + 1] = v;
            rgba_binarised[pi + 2] = v;
            rgba_binarised[pi + 3] = std::byte{0xFFu};
        }
    }
    foundation::tiff_encoder::Page out_page{
        .width = w,
        .height = h,
        .rgba = std::span<const std::byte>(
            rgba_binarised.data(), rgba_binarised.size()),
    };
    foundation::tiff_encoder::Options options{};
    options.dpi = static_cast<std::uint32_t>(resolution_.X());

    const auto encoded = foundation::tiff_encoder::Encode(
        std::span<const foundation::tiff_encoder::Page>(&out_page, 1),
        options);
    EmitTo(outputImageStream, std::span<const std::byte>(
        encoded.data(), encoded.size()));
}

// ---------------------------------------------------------------------------
// Property accessors
// ---------------------------------------------------------------------------

::Aspose::Pdf::RenderingOptions& TiffDevice::RenderingOptions() {
    return rendering_options_;
}
void TiffDevice::RenderingOptions(const ::Aspose::Pdf::RenderingOptions& v) {
    rendering_options_ = v;
}

::Aspose::Pdf::Devices::FormPresentationMode
TiffDevice::FormPresentationMode() const noexcept {
    return form_presentation_mode_;
}
void TiffDevice::FormPresentationMode(
        ::Aspose::Pdf::Devices::FormPresentationMode v) noexcept {
    form_presentation_mode_ = v;
}

const class TiffSettings& TiffDevice::Settings() const noexcept {
    return settings_;
}

const class Resolution& TiffDevice::Resolution() const noexcept {
    return resolution_;
}

int TiffDevice::Width() const noexcept { return width_; }
int TiffDevice::Height() const noexcept { return height_; }

}  // namespace Aspose::Pdf::Devices
