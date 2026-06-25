#include "tiff_encoder.hpp"

#include "flate.hpp"
#include "lzw.hpp"
#include "runlength.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

// TIFF 6.0 baseline write. Spec authority: TIFF 6.0 (Adobe,
// 1992-06-03) §2 (file structure: header + IFD + fields), §3
// (baseline images), §7 (compression schemes — PackBits §9,
// LZW §13), §8.5 (ExtraSamples — alpha channel handling),
// Adobe Deflate (TIFF tag value 8, identical wire format to
// PDF /FlateDecode). Strip compression delegates to
// foundation::lzw / foundation::flate / foundation::runlength.
//
// Scope mirrors foundation::tiff_decoder's read scope: strips
// only, 8-bit samples only, four lossless compressions
// (None / LZW / Deflate / PackBits), four photometrics
// (BlackIsZero, Rgb, Rgb-with-alpha, Palette). WhiteIsZero is
// excluded; CCITT (Group 4) compression is not yet exposed as a
// TIFF Compression option — foundation::ccitt::Encode exists but
// wiring it needs a public Compression::Group4 enum value (a
// separate public-surface change). Multi-page output via chained
// IFDs.

namespace foundation::tiff_encoder {

namespace {

// TIFF field-type codes per TIFF 6.0 §2 page 16.
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;

// TIFF Compression-tag values per §7.
constexpr std::uint16_t kCompNone = 1;
constexpr std::uint16_t kCompLzw = 5;
constexpr std::uint16_t kCompDeflate = 8;
constexpr std::uint16_t kCompPackBits = 32773;

// TIFF PhotometricInterpretation-tag values per §3.
constexpr std::uint16_t kPhotoBlackIsZero = 1;
constexpr std::uint16_t kPhotoRgb = 2;
constexpr std::uint16_t kPhotoPalette = 3;

// In-IFD tag codes. Set is the 13 baseline tags the spec
// yaml's table enumerates plus ExtraSamples (338) for the
// SPP=4 RGBA case.
constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometric = 262;
constexpr std::uint16_t kTagStripOffsets = 273;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagRowsPerStrip = 278;
constexpr std::uint16_t kTagStripByteCounts = 279;
constexpr std::uint16_t kTagXResolution = 282;
constexpr std::uint16_t kTagYResolution = 283;
constexpr std::uint16_t kTagResolutionUnit = 296;
constexpr std::uint16_t kTagColorMap = 320;
constexpr std::uint16_t kTagExtraSamples = 338;

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("tiff_encoder: " + msg);
}

// Writeable byte buffer with little-endian helpers + a
// back-patch helper. TIFF 6.0 §2 allows either byte order;
// we always emit little-endian since the marker we write is
// 'II'.
struct ByteBuffer {
    std::vector<std::byte> data;

    [[nodiscard]] std::size_t Pos() const noexcept { return data.size(); }

    void Pad16() {
        if (data.size() & 1u) data.push_back(std::byte{0});
    }

    void U8(std::uint8_t v) { data.push_back(std::byte{v}); }

    void U16(std::uint16_t v) {
        data.push_back(static_cast<std::byte>(v & 0xFFu));
        data.push_back(static_cast<std::byte>((v >> 8) & 0xFFu));
    }

    void U32(std::uint32_t v) {
        data.push_back(static_cast<std::byte>(v & 0xFFu));
        data.push_back(static_cast<std::byte>((v >> 8) & 0xFFu));
        data.push_back(static_cast<std::byte>((v >> 16) & 0xFFu));
        data.push_back(static_cast<std::byte>((v >> 24) & 0xFFu));
    }

    void Bytes(std::span<const std::byte> b) {
        data.insert(data.end(), b.begin(), b.end());
    }

    // Patch a 4-byte slot at `at` with the LE 32-bit value.
    // Used for back-patching the first-IFD-offset slot in
    // the header and the next-IFD-offset slot at the end of
    // each preceding IFD.
    void PatchU32(std::size_t at, std::uint32_t v) {
        data[at + 0] = static_cast<std::byte>(v & 0xFFu);
        data[at + 1] = static_cast<std::byte>((v >> 8) & 0xFFu);
        data[at + 2] = static_cast<std::byte>((v >> 16) & 0xFFu);
        data[at + 3] = static_cast<std::byte>((v >> 24) & 0xFFu);
    }
};

// One IFD entry constructed during emit. Tag-numeric order is
// required by TIFF 6.0 §2 ("entries must be sorted by tag
// value"); we collect entries unsorted then sort once before
// writing the IFD.
struct IfdEntry {
    std::uint16_t tag;
    std::uint16_t type;
    std::uint32_t count;
    // The 4-byte value/offset slot. Per §2: when value size
    // <= 4 bytes, value is left-justified here; otherwise
    // this is a file offset to the value.
    std::array<std::byte, 4> value_slot{};
};

[[nodiscard]] IfdEntry EntryShort1(std::uint16_t tag,
                                   std::uint16_t value) {
    IfdEntry e{};
    e.tag = tag;
    e.type = kTypeShort;
    e.count = 1;
    e.value_slot[0] = static_cast<std::byte>(value & 0xFFu);
    e.value_slot[1] = static_cast<std::byte>((value >> 8) & 0xFFu);
    return e;
}

[[nodiscard]] IfdEntry EntryLong1(std::uint16_t tag,
                                  std::uint32_t value) {
    IfdEntry e{};
    e.tag = tag;
    e.type = kTypeLong;
    e.count = 1;
    e.value_slot[0] = static_cast<std::byte>(value & 0xFFu);
    e.value_slot[1] = static_cast<std::byte>((value >> 8) & 0xFFu);
    e.value_slot[2] = static_cast<std::byte>((value >> 16) & 0xFFu);
    e.value_slot[3] = static_cast<std::byte>((value >> 24) & 0xFFu);
    return e;
}

[[nodiscard]] IfdEntry EntryOutOfLine(std::uint16_t tag,
                                      std::uint16_t type,
                                      std::uint32_t count,
                                      std::uint32_t offset) {
    IfdEntry e{};
    e.tag = tag;
    e.type = type;
    e.count = count;
    e.value_slot[0] = static_cast<std::byte>(offset & 0xFFu);
    e.value_slot[1] = static_cast<std::byte>((offset >> 8) & 0xFFu);
    e.value_slot[2] = static_cast<std::byte>((offset >> 16) & 0xFFu);
    e.value_slot[3] = static_cast<std::byte>((offset >> 24) & 0xFFu);
    return e;
}

// Translate the public Compression enum to the on-wire TIFF
// tag value.
[[nodiscard]] std::uint16_t CompressionTag(Compression c) {
    switch (c) {
        case Compression::None:     return kCompNone;
        case Compression::Lzw:      return kCompLzw;
        case Compression::Deflate:  return kCompDeflate;
        case Compression::PackBits: return kCompPackBits;
    }
    Fail("internal: unknown Compression enum");
}

// Translate the public Photometric enum to the on-wire TIFF
// tag value (rejecting WhiteIsZero per scope).
[[nodiscard]] std::uint16_t PhotometricTag(Photometric p) {
    switch (p) {
        case Photometric::WhiteIsZero:
            Fail("Photometric::WhiteIsZero is out of v1 scope");
        case Photometric::BlackIsZero: return kPhotoBlackIsZero;
        case Photometric::Rgb:         return kPhotoRgb;
        case Photometric::Palette:     return kPhotoPalette;
    }
    Fail("internal: unknown Photometric enum");
}

// Strip-compression dispatch — routes through the existing
// foundation primitives so the four supported compressions
// share a single dispatch site with tiff_decoder's read side.
[[nodiscard]] std::vector<std::byte> CompressStrip(
        std::span<const std::byte> uncompressed,
        Compression compression) {
    switch (compression) {
        case Compression::None:
            return std::vector<std::byte>(
                uncompressed.begin(), uncompressed.end());
        case Compression::Lzw:
            return foundation::lzw::Encode(
                uncompressed, foundation::lzw::EarlyChange::Tiff);
        case Compression::Deflate:
            return foundation::flate::Encode(uncompressed);
        case Compression::PackBits:
            return foundation::runlength::Encode(uncompressed);
    }
    Fail("internal: unknown Compression enum");
}

// Photometric / SPP / BPS-driven sample preparation. Converts
// the input RGBA8 buffer into the on-wire byte sequence
// dictated by (photometric, samples_per_pixel, bits_per_sample)
// per spec §3 + §8.5 + the wider-depth scope.
[[nodiscard]] std::vector<std::byte> PrepareSamples(
        const Page& page, Photometric photometric,
        std::uint32_t spp, std::uint32_t bps) {
    const std::size_t pixel_count =
        static_cast<std::size_t>(page.width) * page.height;
    std::vector<std::byte> out;

    if (photometric == Photometric::Rgb && spp == 4 && bps == 8) {
        // RGBA pass-through.
        out.assign(page.rgba.begin(), page.rgba.end());
        return out;
    }
    if (photometric == Photometric::Rgb && spp == 3 && bps == 8) {
        // Drop the alpha byte.
        out.reserve(pixel_count * 3);
        for (std::size_t i = 0; i < pixel_count; ++i) {
            const std::byte* p = page.rgba.data() + i * 4;
            out.push_back(p[0]);
            out.push_back(p[1]);
            out.push_back(p[2]);
        }
        return out;
    }
    if (photometric == Photometric::BlackIsZero && spp == 1 &&
        bps == 8) {
        // Collapse R=G=B=Y into one sample per pixel; reject
        // non-grayscale (strict R=G=B at every pixel).
        out.reserve(pixel_count);
        for (std::size_t i = 0; i < pixel_count; ++i) {
            const std::byte* p = page.rgba.data() + i * 4;
            if (p[0] != p[1] || p[1] != p[2]) {
                Fail("Photometric::BlackIsZero with bits_per_sample=8 "
                     "requires R == G == B at every pixel; mismatch "
                     "at pixel " + std::to_string(i));
            }
            out.push_back(p[0]);
        }
        return out;
    }
    if (photometric == Photometric::BlackIsZero && spp == 1 &&
        bps == 1) {
        // 1bpp luma threshold: BT.601 weights × 1000 truncated;
        // bit = 1 when luma >= 128 (BlackIsZero: 0=black,
        // 1=white). MSB-first 8 px/byte; rows padded to whole
        // bytes (row stride = (width+7)/8).
        const std::size_t row_bytes =
            (static_cast<std::size_t>(page.width) + 7) / 8;
        out.assign(row_bytes * page.height, std::byte{0});
        for (std::uint32_t y = 0; y < page.height; ++y) {
            std::byte* dst = out.data() + y * row_bytes;
            const std::byte* src =
                page.rgba.data() + static_cast<std::size_t>(y) *
                                       page.width * 4;
            for (std::uint32_t x = 0; x < page.width; ++x) {
                const auto r = static_cast<int>(
                    std::to_integer<std::uint8_t>(src[x * 4 + 0]));
                const auto g = static_cast<int>(
                    std::to_integer<std::uint8_t>(src[x * 4 + 1]));
                const auto b = static_cast<int>(
                    std::to_integer<std::uint8_t>(src[x * 4 + 2]));
                const int luma = (r * 299 + g * 587 + b * 114) / 1000;
                if (luma >= 128) {
                    const auto mask = static_cast<std::uint8_t>(
                        0x80u >> (x % 8));
                    dst[x / 8] = static_cast<std::byte>(
                        std::to_integer<std::uint8_t>(dst[x / 8]) | mask);
                }
            }
        }
        return out;
    }
    if (photometric == Photometric::Palette && spp == 1 &&
        bps == 8) {
        // Use the input R byte as the 8bpp palette index.
        out.reserve(pixel_count);
        for (std::size_t i = 0; i < pixel_count; ++i) {
            out.push_back(page.rgba[i * 4]);
        }
        return out;
    }
    if (photometric == Photometric::Palette && spp == 1 &&
        bps == 4) {
        // 4bpp palette: low nibble of input R is the index;
        // pack 2 indices per byte, high nibble = first pixel
        // of the pair. Rows padded to whole bytes (row stride
        // = (width+1)/2).
        const std::size_t row_bytes =
            (static_cast<std::size_t>(page.width) + 1) / 2;
        out.assign(row_bytes * page.height, std::byte{0});
        for (std::uint32_t y = 0; y < page.height; ++y) {
            std::byte* dst = out.data() + y * row_bytes;
            const std::byte* src =
                page.rgba.data() + static_cast<std::size_t>(y) *
                                       page.width * 4;
            for (std::uint32_t x = 0; x < page.width; ++x) {
                const auto v = static_cast<std::uint8_t>(
                    std::to_integer<std::uint8_t>(src[x * 4]) & 0x0Fu);
                if ((x & 1u) == 0) {
                    dst[x / 2] = static_cast<std::byte>(v << 4);
                } else {
                    dst[x / 2] = static_cast<std::byte>(
                        std::to_integer<std::uint8_t>(dst[x / 2]) | v);
                }
            }
        }
        return out;
    }
    Fail("internal: unreachable PrepareSamples branch");
}

// Validate (photometric, spp, bps) tuple per the spec's error
// model. Valid tuples:
//   (BlackIsZero, 1, 1)  — 1bpp grayscale, luma-thresholded
//   (BlackIsZero, 1, 8)  — 8bpp grayscale, strict R=G=B
//   (Palette,     1, 4)  — 4bpp palette, 16-entry color_map
//   (Palette,     1, 8)  — 8bpp palette, 256-entry color_map
//   (Rgb,         3, 8)  — 24bpp RGB
//   (Rgb,         4, 8)  — 32bpp RGBA
void ValidatePhotometricSppBps(Photometric photometric,
                               std::uint32_t spp,
                               std::uint32_t bps) {
    if (bps != 1 && bps != 4 && bps != 8) {
        Fail("bits_per_sample must be 1, 4, or 8 (got " +
             std::to_string(bps) + ")");
    }
    if (spp != 1 && spp != 3 && spp != 4) {
        Fail("samples_per_pixel must be 1, 3, or 4 (got " +
             std::to_string(spp) + ")");
    }
    const auto reject = [&]() {
        Fail("invalid (photometric, samples_per_pixel, "
             "bits_per_sample) tuple: (" +
             std::to_string(static_cast<int>(photometric)) + ", " +
             std::to_string(spp) + ", " + std::to_string(bps) + ")");
    };
    switch (photometric) {
        case Photometric::WhiteIsZero:
            Fail("Photometric::WhiteIsZero is out of v1 scope");
        case Photometric::BlackIsZero:
            if (spp != 1) reject();
            if (bps != 1 && bps != 8) reject();
            break;
        case Photometric::Rgb:
            if (spp != 3 && spp != 4) reject();
            if (bps != 8) reject();
            break;
        case Photometric::Palette:
            if (spp != 1) reject();
            if (bps != 4 && bps != 8) reject();
            break;
    }
}

}  // namespace

std::vector<std::byte> Encode(std::span<const Page> pages,
                              Options const& options) {
    if (pages.empty()) {
        Fail("pages list is empty");
    }
    if (options.dpi == 0) {
        Fail("dpi must be > 0");
    }
    ValidatePhotometricSppBps(options.photometric,
                              options.samples_per_pixel,
                              options.bits_per_sample);
    if (options.photometric == Photometric::Palette) {
        const std::size_t expected_entries =
            3u * (1u << options.bits_per_sample);
        if (options.color_map.size() != expected_entries) {
            Fail("Photometric::Palette at bits_per_sample=" +
                 std::to_string(options.bits_per_sample) +
                 " requires a " + std::to_string(expected_entries) +
                 "-entry color_map (got " +
                 std::to_string(options.color_map.size()) + ")");
        }
    }

    // Per-page rgba length sanity check — fail early before
    // any output bytes are emitted.
    for (std::size_t pi = 0; pi < pages.size(); ++pi) {
        const auto& page = pages[pi];
        const std::uint64_t expected =
            static_cast<std::uint64_t>(page.width) *
            static_cast<std::uint64_t>(page.height) * 4ULL;
        if (page.rgba.size() != expected) {
            Fail("page " + std::to_string(pi) +
                 ": rgba.size() (" +
                 std::to_string(page.rgba.size()) +
                 ") != width*height*4 (" +
                 std::to_string(expected) + ")");
        }
        if (page.width == 0 || page.height == 0) {
            Fail("page " + std::to_string(pi) +
                 ": width and height must be > 0");
        }
    }

    const std::uint16_t compression_tag =
        CompressionTag(options.compression);
    const std::uint16_t photometric_tag =
        PhotometricTag(options.photometric);
    const std::uint16_t spp =
        static_cast<std::uint16_t>(options.samples_per_pixel);
    const bool emit_extrasamples =
        (options.photometric == Photometric::Rgb && spp == 4);

    ByteBuffer out;

    // 1. Header — 'II' + magic 42 + first-IFD-offset (back-
    // patched after the first page's IFD lands).
    out.U8('I');
    out.U8('I');
    out.U16(42);
    const std::size_t first_ifd_slot = out.Pos();
    out.U32(0);  // placeholder

    std::size_t prev_next_ifd_slot = first_ifd_slot;

    for (std::size_t pi = 0; pi < pages.size(); ++pi) {
        const auto& page = pages[pi];

        // 2a. Sample preparation.
        const auto samples = PrepareSamples(
            page, options.photometric, spp,
            options.bits_per_sample);

        // 2b. Strip compression.
        const auto compressed = CompressStrip(
            std::span<const std::byte>(samples), options.compression);
        const std::uint32_t strip_offset =
            static_cast<std::uint32_t>(out.Pos());
        out.Bytes(compressed);
        const std::uint32_t strip_byte_count =
            static_cast<std::uint32_t>(compressed.size());

        // 2c. Out-of-line tag values. Each must be word-
        // aligned per §2 page 14, so pad before each block
        // whose length isn't already even.
        out.Pad16();

        // BitsPerSample: only out-of-line when SPP > 2 (count
        // * 2-byte SHORT > 4 bytes). SPP=1 fits in the slot;
        // SPP=2 fits exactly; SPP=3 needs 6 bytes; SPP=4
        // needs 8 bytes. Each SHORT is bits_per_sample (NOT
        // hardcoded 8 — wider-depth scope passes 1 or 4 here
        // for Grayscale/Palette at SPP=1, but the 1bpp/4bpp
        // paths take the in-line SPP=1 branch below).
        std::uint32_t bits_per_sample_offset = 0;
        if (spp > 2) {
            bits_per_sample_offset =
                static_cast<std::uint32_t>(out.Pos());
            for (std::uint16_t s = 0; s < spp; ++s) {
                out.U16(static_cast<std::uint16_t>(
                    options.bits_per_sample));
            }
            out.Pad16();
        }

        // XResolution / YResolution as RATIONAL (two LONGs).
        // Always out-of-line (8 bytes > 4-byte slot).
        const std::uint32_t x_res_offset =
            static_cast<std::uint32_t>(out.Pos());
        out.U32(options.dpi);
        out.U32(1);
        const std::uint32_t y_res_offset =
            static_cast<std::uint32_t>(out.Pos());
        out.U32(options.dpi);
        out.U32(1);
        out.Pad16();

        // ColorMap: 3 * 256 SHORT entries (palette only).
        std::uint32_t color_map_offset = 0;
        std::uint32_t color_map_count = 0;
        if (options.photometric == Photometric::Palette) {
            color_map_offset =
                static_cast<std::uint32_t>(out.Pos());
            color_map_count = static_cast<std::uint32_t>(
                options.color_map.size());
            for (auto v : options.color_map) out.U16(v);
            out.Pad16();
        }

        // 2d. Build IFD entries, sort by tag, then write.
        std::vector<IfdEntry> entries;
        entries.reserve(14);
        entries.push_back(EntryLong1(kTagImageWidth, page.width));
        entries.push_back(EntryLong1(kTagImageLength, page.height));
        if (spp == 1) {
            entries.push_back(EntryShort1(
                kTagBitsPerSample,
                static_cast<std::uint16_t>(options.bits_per_sample)));
        } else if (spp == 2) {
            // Two SHORTs in slot.
            IfdEntry e{};
            e.tag = kTagBitsPerSample;
            e.type = kTypeShort;
            e.count = 2;
            const auto bps_byte = static_cast<std::uint8_t>(
                options.bits_per_sample);
            e.value_slot[0] = std::byte{bps_byte};
            e.value_slot[2] = std::byte{bps_byte};
            entries.push_back(e);
        } else {
            entries.push_back(EntryOutOfLine(
                kTagBitsPerSample, kTypeShort, spp,
                bits_per_sample_offset));
        }
        entries.push_back(EntryShort1(
            kTagCompression, compression_tag));
        entries.push_back(EntryShort1(
            kTagPhotometric, photometric_tag));
        entries.push_back(EntryLong1(
            kTagStripOffsets, strip_offset));
        entries.push_back(EntryShort1(
            kTagSamplesPerPixel, spp));
        entries.push_back(EntryLong1(
            kTagRowsPerStrip, page.height));
        entries.push_back(EntryLong1(
            kTagStripByteCounts, strip_byte_count));
        entries.push_back(EntryOutOfLine(
            kTagXResolution, kTypeRational, 1, x_res_offset));
        entries.push_back(EntryOutOfLine(
            kTagYResolution, kTypeRational, 1, y_res_offset));
        entries.push_back(EntryShort1(
            kTagResolutionUnit, 2));  // 2 = inch
        if (options.photometric == Photometric::Palette) {
            entries.push_back(EntryOutOfLine(
                kTagColorMap, kTypeShort,
                color_map_count, color_map_offset));
        }
        if (emit_extrasamples) {
            // ExtraSamples=2 (unassociated alpha) per §8.5.
            entries.push_back(EntryShort1(kTagExtraSamples, 2));
        }

        std::sort(entries.begin(), entries.end(),
                  [](const IfdEntry& a, const IfdEntry& b) {
                      return a.tag < b.tag;
                  });

        // 2e. IFD must start on a word boundary (§2 page 14).
        out.Pad16();
        const std::uint32_t ifd_offset =
            static_cast<std::uint32_t>(out.Pos());
        out.U16(static_cast<std::uint16_t>(entries.size()));
        for (const auto& e : entries) {
            out.U16(e.tag);
            out.U16(e.type);
            out.U32(e.count);
            out.Bytes(std::span<const std::byte>(
                e.value_slot.data(), e.value_slot.size()));
        }
        const std::size_t this_next_ifd_slot = out.Pos();
        out.U32(0);  // placeholder for next-IFD-offset

        // 2f. Back-patch: previous IFD's next-IFD slot (or
        // header's first-IFD slot for page 0) gets this IFD's
        // offset.
        out.PatchU32(prev_next_ifd_slot, ifd_offset);
        prev_next_ifd_slot = this_next_ifd_slot;
    }
    // Last IFD's next-IFD-offset stays 0 by construction.

    return std::move(out.data);
}

}  // namespace foundation::tiff_encoder
