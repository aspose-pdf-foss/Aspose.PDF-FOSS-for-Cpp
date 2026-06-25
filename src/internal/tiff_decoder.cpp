#include "tiff_decoder.hpp"

#include "flate.hpp"
#include "lzw.hpp"
#include "runlength.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

// TIFF 6.0 baseline read. Spec authority: TIFF 6.0 (Adobe,
// 1992-06-03) §2 (file structure), §3 (baseline images), §7
// (compression schemes), §8.5 (ExtraSamples). The primitive
// supports the first-IFD only, 8-bit samples only, strips
// only, and four lossless compressions (None / LZW / Deflate /
// PackBits) routed through foundation::lzw / foundation::flate
// / foundation::runlength.

namespace foundation::tiff_decoder {

namespace {

// TIFF field-type codes per §2 page 16. We accept SHORT and
// LONG for integer tags + RATIONAL for the resolution tags.
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;

// In-scope tag codes per the spec yaml's tag table.
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
constexpr std::uint16_t kTagPlanarConfig = 284;
constexpr std::uint16_t kTagPredictor = 317;
constexpr std::uint16_t kTagColorMap = 320;
constexpr std::uint16_t kTagTileWidth = 322;
constexpr std::uint16_t kTagTileLength = 323;
constexpr std::uint16_t kTagExtraSamples = 338;

// Compression-tag values we accept per §7. CCITT (2/3/4),
// old/new JPEG (6/7), and any extension code raise.
constexpr std::uint16_t kCompNone = 1;
constexpr std::uint16_t kCompCcittRle = 2;
constexpr std::uint16_t kCompCcittT4 = 3;
constexpr std::uint16_t kCompCcittT6 = 4;
constexpr std::uint16_t kCompLzw = 5;
constexpr std::uint16_t kCompOldJpeg = 6;
constexpr std::uint16_t kCompJpeg = 7;
constexpr std::uint16_t kCompDeflate = 8;
constexpr std::uint16_t kCompPackBits = 32773;

// Photometric-tag values we accept per §3 + §8.5.
constexpr std::uint16_t kPhotoWhiteIsZero = 0;
constexpr std::uint16_t kPhotoBlackIsZero = 1;
constexpr std::uint16_t kPhotoRgb = 2;
constexpr std::uint16_t kPhotoPalette = 3;

// Reader bound to a TIFF byte stream + its byte order. Endian
// resolution happens at header parse, after which every
// multi-byte field passes through Read16 / Read32.
struct ByteReader {
    std::span<const std::byte> data;
    bool little_endian = true;

    [[nodiscard]] std::uint8_t U8(std::size_t off) const {
        if (off >= data.size()) {
            throw std::runtime_error(
                "tiff_decoder: read past end of input at offset "
                + std::to_string(off));
        }
        return static_cast<std::uint8_t>(data[off]);
    }

    [[nodiscard]] std::uint16_t U16(std::size_t off) const {
        if (off + 2 > data.size()) {
            throw std::runtime_error(
                "tiff_decoder: read past end of input at offset "
                + std::to_string(off));
        }
        const auto b0 = static_cast<std::uint16_t>(
            static_cast<std::uint8_t>(data[off + 0]));
        const auto b1 = static_cast<std::uint16_t>(
            static_cast<std::uint8_t>(data[off + 1]));
        return little_endian
            ? static_cast<std::uint16_t>(b0 | (b1 << 8))
            : static_cast<std::uint16_t>((b0 << 8) | b1);
    }

    [[nodiscard]] std::uint32_t U32(std::size_t off) const {
        if (off + 4 > data.size()) {
            throw std::runtime_error(
                "tiff_decoder: read past end of input at offset "
                + std::to_string(off));
        }
        const auto b0 = static_cast<std::uint32_t>(
            static_cast<std::uint8_t>(data[off + 0]));
        const auto b1 = static_cast<std::uint32_t>(
            static_cast<std::uint8_t>(data[off + 1]));
        const auto b2 = static_cast<std::uint32_t>(
            static_cast<std::uint8_t>(data[off + 2]));
        const auto b3 = static_cast<std::uint32_t>(
            static_cast<std::uint8_t>(data[off + 3]));
        return little_endian
            ? (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24))
            : ((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
    }
};

// One IFD entry (12 bytes per §2 page 14).
struct IfdEntry {
    std::uint16_t tag;
    std::uint16_t type;
    std::uint32_t count;
    // Raw 4-byte value/offset slot. The interpretation depends
    // on (type, count): per §2 page 15, when the value's total
    // size ≤ 4 bytes the value is left-justified in the slot
    // (i.e. stored in the lower-numbered bytes); otherwise the
    // slot holds an offset to the value.
    std::array<std::byte, 4> value_slot;
};

// Read a SHORT value from a 4-byte slot at the given offset
// within the slot (0 or 2). Honours the file's byte order.
[[nodiscard]] std::uint16_t ReadShortFromSlot(
        const std::array<std::byte, 4>& slot,
        std::size_t off, bool little_endian) {
    const auto b0 = static_cast<std::uint16_t>(
        static_cast<std::uint8_t>(slot[off + 0]));
    const auto b1 = static_cast<std::uint16_t>(
        static_cast<std::uint8_t>(slot[off + 1]));
    return little_endian
        ? static_cast<std::uint16_t>(b0 | (b1 << 8))
        : static_cast<std::uint16_t>((b0 << 8) | b1);
}

// Read a LONG value from a 4-byte slot.
[[nodiscard]] std::uint32_t ReadLongFromSlot(
        const std::array<std::byte, 4>& slot, bool little_endian) {
    const auto b0 = static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(slot[0]));
    const auto b1 = static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(slot[1]));
    const auto b2 = static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(slot[2]));
    const auto b3 = static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(slot[3]));
    return little_endian
        ? (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24))
        : ((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
}

// Pull a count of integer (SHORT or LONG) values from an entry.
// SHORT count=1 fits in slot bytes 0..1; SHORT count=2 fits in
// slot bytes 0..3; SHORT count>=3 is out-of-line via the
// slot's offset. LONG count=1 fits in the full slot;
// LONG count>=2 is out-of-line.
[[nodiscard]] std::vector<std::uint32_t> ReadIntegerArray(
        const ByteReader& reader, const IfdEntry& entry) {
    if (entry.type != kTypeShort && entry.type != kTypeLong) {
        throw std::runtime_error(
            "tiff_decoder: expected SHORT or LONG field type for tag "
            + std::to_string(entry.tag) + ", got "
            + std::to_string(entry.type));
    }
    const std::size_t element_size = (entry.type == kTypeShort) ? 2 : 4;
    const std::size_t total_size = element_size * entry.count;
    std::vector<std::uint32_t> values;
    values.reserve(entry.count);
    if (total_size <= 4) {
        // In-slot values, left-justified.
        for (std::uint32_t i = 0; i < entry.count; ++i) {
            const std::size_t off = i * element_size;
            if (entry.type == kTypeShort) {
                values.push_back(ReadShortFromSlot(
                    entry.value_slot, off, reader.little_endian));
            } else {
                values.push_back(ReadLongFromSlot(
                    entry.value_slot, reader.little_endian));
            }
        }
    } else {
        // Out-of-line: the slot holds the byte offset.
        const std::uint32_t value_offset = ReadLongFromSlot(
            entry.value_slot, reader.little_endian);
        for (std::uint32_t i = 0; i < entry.count; ++i) {
            const std::size_t off = static_cast<std::size_t>(
                value_offset) + i * element_size;
            values.push_back(entry.type == kTypeShort
                ? reader.U16(off)
                : reader.U32(off));
        }
    }
    return values;
}

// Pull a single integer (SHORT or LONG, count=1).
[[nodiscard]] std::uint32_t ReadSingleInteger(
        const ByteReader& reader, const IfdEntry& entry) {
    if (entry.count != 1) {
        throw std::runtime_error(
            "tiff_decoder: expected count=1 for tag "
            + std::to_string(entry.tag) + ", got "
            + std::to_string(entry.count));
    }
    const auto values = ReadIntegerArray(reader, entry);
    return values[0];
}

// Decompress one strip per the file's Compression tag.
[[nodiscard]] std::vector<std::byte> DecompressStrip(
        std::span<const std::byte> compressed,
        std::uint16_t compression) {
    switch (compression) {
        case kCompNone:
            return std::vector<std::byte>(
                compressed.begin(), compressed.end());
        case kCompLzw:
            return foundation::lzw::Decode(
                compressed, foundation::lzw::EarlyChange::Tiff);
        case kCompDeflate:
            return foundation::flate::Decode(compressed);
        case kCompPackBits:
            return foundation::runlength::Decode(compressed);
        default:
            throw std::runtime_error(
                "tiff_decoder: unreachable — unsupported compression "
                + std::to_string(compression));
    }
}

// Validate compression tag against the in-scope set, raising a
// caller-friendly message that names the offending compression.
void ValidateCompression(std::uint16_t compression) {
    switch (compression) {
        case kCompNone:
        case kCompLzw:
        case kCompDeflate:
        case kCompPackBits:
            return;
        case kCompCcittRle:
        case kCompCcittT4:
        case kCompCcittT6:
            throw std::runtime_error(
                "tiff_decoder: CCITT compression ("
                + std::to_string(compression)
                + ") is not supported (bilevel-only)");
        case kCompOldJpeg:
        case kCompJpeg:
            throw std::runtime_error(
                "tiff_decoder: JPEG compression ("
                + std::to_string(compression)
                + ") is not supported");
        default:
            throw std::runtime_error(
                "tiff_decoder: unsupported compression "
                + std::to_string(compression));
    }
}

void ValidatePhotometric(std::uint16_t photometric) {
    if (photometric != kPhotoBlackIsZero
            && photometric != kPhotoRgb
            && photometric != kPhotoPalette) {
        if (photometric == kPhotoWhiteIsZero) {
            throw std::runtime_error(
                "tiff_decoder: WhiteIsZero photometric "
                "interpretation is not supported");
        }
        throw std::runtime_error(
            "tiff_decoder: unsupported photometric "
            "interpretation " + std::to_string(photometric));
    }
}

}  // namespace

DecodedImage Decode(std::span<const std::byte> input) {
    if (input.empty()) {
        throw std::runtime_error("tiff_decoder: input is empty");
    }
    if (input.size() < 8) {
        throw std::runtime_error(
            "tiff_decoder: input shorter than 8-byte header");
    }

    // 1. Header parse. §2 page 13.
    ByteReader reader{input, true};
    const auto bom0 = static_cast<std::uint8_t>(input[0]);
    const auto bom1 = static_cast<std::uint8_t>(input[1]);
    if (bom0 == 'I' && bom1 == 'I') {
        reader.little_endian = true;
    } else if (bom0 == 'M' && bom1 == 'M') {
        reader.little_endian = false;
    } else {
        throw std::runtime_error(
            "tiff_decoder: not a TIFF (byte-order marker is neither "
            "'II' nor 'MM')");
    }
    const auto magic = reader.U16(2);
    if (magic != 42) {
        throw std::runtime_error(
            "tiff_decoder: not a TIFF (magic number is "
            + std::to_string(magic) + ", expected 42)");
    }
    const auto first_ifd_offset = reader.U32(4);
    if (first_ifd_offset == 0
            || first_ifd_offset >= input.size()) {
        throw std::runtime_error(
            "tiff_decoder: first-IFD offset out of range");
    }

    // 2. IFD walk. §2 page 14. We read every entry but only
    // remember the in-scope tags. Out-of-scope tags are silently
    // ignored unless their VALUE indicates an unsupported
    // capability (PlanarConfig=2, Predictor != 1, tile tags
    // present at all).
    const auto entry_count = reader.U16(first_ifd_offset);
    const std::size_t entries_off = first_ifd_offset + 2;
    if (entries_off + entry_count * 12 + 4 > input.size()) {
        throw std::runtime_error(
            "tiff_decoder: IFD truncated (entry count "
            + std::to_string(entry_count) + " runs past end of file)");
    }

    bool have_image_width = false;
    bool have_image_length = false;
    bool have_bits_per_sample = false;
    bool have_compression = false;
    bool have_photometric = false;
    bool have_strip_offsets = false;
    bool have_samples_per_pixel = false;
    bool have_rows_per_strip = false;
    bool have_strip_byte_counts = false;
    bool have_color_map = false;

    std::uint32_t image_width = 0;
    std::uint32_t image_length = 0;
    std::vector<std::uint32_t> bits_per_sample;
    std::uint16_t compression = kCompNone;
    std::uint16_t photometric = kPhotoBlackIsZero;
    std::vector<std::uint32_t> strip_offsets;
    std::uint32_t samples_per_pixel = 1;
    std::uint32_t rows_per_strip = 0;
    std::vector<std::uint32_t> strip_byte_counts;
    std::vector<std::uint32_t> color_map;
    std::vector<std::uint32_t> extra_samples;

    for (std::uint32_t i = 0; i < entry_count; ++i) {
        const std::size_t entry_off = entries_off + i * 12;
        IfdEntry entry;
        entry.tag = reader.U16(entry_off + 0);
        entry.type = reader.U16(entry_off + 2);
        entry.count = reader.U32(entry_off + 4);
        std::memcpy(entry.value_slot.data(),
                    input.data() + entry_off + 8, 4);

        switch (entry.tag) {
            case kTagImageWidth:
                image_width = ReadSingleInteger(reader, entry);
                have_image_width = true;
                break;
            case kTagImageLength:
                image_length = ReadSingleInteger(reader, entry);
                have_image_length = true;
                break;
            case kTagBitsPerSample:
                bits_per_sample = ReadIntegerArray(reader, entry);
                have_bits_per_sample = true;
                break;
            case kTagCompression:
                compression = static_cast<std::uint16_t>(
                    ReadSingleInteger(reader, entry));
                have_compression = true;
                break;
            case kTagPhotometric:
                photometric = static_cast<std::uint16_t>(
                    ReadSingleInteger(reader, entry));
                have_photometric = true;
                break;
            case kTagStripOffsets:
                strip_offsets = ReadIntegerArray(reader, entry);
                have_strip_offsets = true;
                break;
            case kTagSamplesPerPixel:
                samples_per_pixel = ReadSingleInteger(reader, entry);
                have_samples_per_pixel = true;
                break;
            case kTagRowsPerStrip:
                rows_per_strip = ReadSingleInteger(reader, entry);
                have_rows_per_strip = true;
                break;
            case kTagStripByteCounts:
                strip_byte_counts = ReadIntegerArray(reader, entry);
                have_strip_byte_counts = true;
                break;
            case kTagColorMap:
                color_map = ReadIntegerArray(reader, entry);
                have_color_map = true;
                break;
            case kTagExtraSamples:
                extra_samples = ReadIntegerArray(reader, entry);
                break;
            case kTagPlanarConfig: {
                const auto pc = ReadSingleInteger(reader, entry);
                if (pc != 1) {
                    throw std::runtime_error(
                        "tiff_decoder: PlanarConfiguration=2 (separate "
                        "planes) is not supported");
                }
                break;
            }
            case kTagPredictor: {
                const auto p = ReadSingleInteger(reader, entry);
                if (p != 1) {
                    throw std::runtime_error(
                        "tiff_decoder: Predictor=" + std::to_string(p)
                        + " is not supported (only Predictor=1 / no "
                        "predictor)");
                }
                break;
            }
            case kTagTileWidth:
            case kTagTileLength:
                throw std::runtime_error(
                    "tiff_decoder: tiled TIFF is not supported "
                    "(strips only)");
            case kTagXResolution:
            case kTagYResolution:
            case kTagResolutionUnit:
                // Diagnostic-only — read but don't validate beyond
                // the type check via the spec yaml's "ignore" rule.
                break;
            default:
                // Out-of-scope tag — silently ignored per §2 page 14.
                break;
        }
    }

    // 3. Validation. §3 + §8.5.
    if (!have_image_width || !have_image_length
            || !have_bits_per_sample || !have_compression
            || !have_photometric || !have_strip_offsets
            || !have_strip_byte_counts) {
        throw std::runtime_error(
            "tiff_decoder: missing required tag in first IFD "
            "(ImageWidth, ImageLength, BitsPerSample, Compression, "
            "Photometric, StripOffsets, StripByteCounts)");
    }
    if (image_width == 0 || image_length == 0) {
        throw std::runtime_error(
            "tiff_decoder: zero-dimension image rejected");
    }
    // SamplesPerPixel defaults to 1 per §3 page 22 if absent.
    if (!have_samples_per_pixel) samples_per_pixel = 1;
    if (samples_per_pixel != 1
            && samples_per_pixel != 3
            && samples_per_pixel != 4) {
        throw std::runtime_error(
            "tiff_decoder: SamplesPerPixel=" + std::to_string(
                samples_per_pixel) + " not in {1, 3, 4}");
    }
    if (bits_per_sample.size() != samples_per_pixel) {
        throw std::runtime_error(
            "tiff_decoder: BitsPerSample count "
            + std::to_string(bits_per_sample.size())
            + " != SamplesPerPixel "
            + std::to_string(samples_per_pixel));
    }
    // Every entry in BitsPerSample must equal the same bps value.
    const std::uint32_t bps = bits_per_sample[0];
    for (auto v : bits_per_sample) {
        if (v != bps) {
            throw std::runtime_error(
                "tiff_decoder: heterogeneous BitsPerSample arrays "
                "are not supported");
        }
    }
    // bps value-set per (SPP, Photometric):
    //   SPP=1, BlackIsZero: bps ∈ {1, 8}
    //   SPP=1, Palette:     bps ∈ {4, 8}
    //   SPP=3, RGB:         bps = 8
    //   SPP=4, RGB+Alpha:   bps = 8
    {
        bool valid = false;
        if (samples_per_pixel == 1
                && photometric == kPhotoBlackIsZero
                && (bps == 1 || bps == 8)) {
            valid = true;
        } else if (samples_per_pixel == 1
                && photometric == kPhotoPalette
                && (bps == 4 || bps == 8)) {
            valid = true;
        } else if (samples_per_pixel == 3
                && photometric == kPhotoRgb
                && bps == 8) {
            valid = true;
        } else if (samples_per_pixel == 4
                && photometric == kPhotoRgb
                && bps == 8) {
            valid = true;
        }
        if (!valid) {
            throw std::runtime_error(
                "tiff_decoder: invalid (SamplesPerPixel="
                + std::to_string(samples_per_pixel)
                + ", Photometric="
                + std::to_string(photometric)
                + ", BitsPerSample=" + std::to_string(bps)
                + ") tuple");
        }
    }
    ValidateCompression(compression);
    ValidatePhotometric(photometric);
    if (strip_offsets.size() != strip_byte_counts.size()) {
        throw std::runtime_error(
            "tiff_decoder: StripOffsets count "
            + std::to_string(strip_offsets.size())
            + " != StripByteCounts count "
            + std::to_string(strip_byte_counts.size()));
    }
    if (strip_offsets.empty()) {
        throw std::runtime_error(
            "tiff_decoder: zero-strip image rejected");
    }
    // RowsPerStrip defaults to ImageLength when absent (single
    // strip per §3 page 27).
    if (!have_rows_per_strip || rows_per_strip == 0) {
        rows_per_strip = image_length;
    }
    if (photometric == kPhotoPalette) {
        if (samples_per_pixel != 1) {
            throw std::runtime_error(
                "tiff_decoder: Palette photometric requires "
                "SamplesPerPixel=1");
        }
        if (!have_color_map) {
            throw std::runtime_error(
                "tiff_decoder: Palette photometric requires the "
                "ColorMap tag");
        }
        // ColorMap carries 3 * 2^BitsPerSample SHORT entries.
        const std::size_t expected =
            static_cast<std::size_t>(3) * (1u << bits_per_sample[0]);
        if (color_map.size() != expected) {
            throw std::runtime_error(
                "tiff_decoder: ColorMap size " + std::to_string(
                    color_map.size()) + " != 3 * 2^bps = "
                + std::to_string(expected));
        }
    }
    if (samples_per_pixel == 4) {
        // §8.5: ExtraSamples must declare the 4th sample is alpha.
        // We accept ExtraSamples=2 (unassociated alpha) only;
        // associated alpha (1) implies premultiplied data which
        // changes the round-trip semantics.
        if (extra_samples.empty()) {
            throw std::runtime_error(
                "tiff_decoder: SamplesPerPixel=4 without ExtraSamples "
                "is ambiguous");
        }
        if (extra_samples.size() != 1 || extra_samples[0] != 2) {
            throw std::runtime_error(
                "tiff_decoder: ExtraSamples must be {2} (unassociated "
                "alpha) for SamplesPerPixel=4");
        }
        if (photometric != kPhotoRgb) {
            throw std::runtime_error(
                "tiff_decoder: SamplesPerPixel=4 requires "
                "Photometric=RGB");
        }
    }

    // 4. Strip decode. Concatenate all strips into a single
    // sample buffer in scan-line order. rowBytes varies by bps:
    //   bps=1, SPP=1: rowBytes = (image_width + 7) / 8
    //   bps=4, SPP=1: rowBytes = (image_width + 1) / 2
    //   bps=8, SPP=N: rowBytes = image_width * N
    std::size_t row_bytes = 0;
    if (bps == 1) {
        row_bytes = (static_cast<std::size_t>(image_width) + 7) / 8;
    } else if (bps == 4) {
        row_bytes = (static_cast<std::size_t>(image_width) + 1) / 2;
    } else {
        row_bytes = static_cast<std::size_t>(image_width)
                    * samples_per_pixel;
    }
    const std::size_t total_samples =
        row_bytes * image_length;
    std::vector<std::byte> samples;
    samples.reserve(total_samples);
    std::uint32_t y0 = 0;
    for (std::size_t i = 0; i < strip_offsets.size(); ++i) {
        const std::uint32_t strip_rows = std::min(
            rows_per_strip, image_length - y0);
        const std::size_t expected_strip_bytes =
            static_cast<std::size_t>(strip_rows) * row_bytes;
        const std::uint32_t strip_off = strip_offsets[i];
        const std::uint32_t strip_len = strip_byte_counts[i];
        if (static_cast<std::size_t>(strip_off) + strip_len
                > input.size()) {
            throw std::runtime_error(
                "tiff_decoder: strip " + std::to_string(i)
                + " runs past end of file");
        }
        const std::span<const std::byte> compressed{
            input.data() + strip_off, strip_len};
        std::vector<std::byte> decompressed = DecompressStrip(
            compressed, compression);
        if (decompressed.size() != expected_strip_bytes) {
            throw std::runtime_error(
                "tiff_decoder: strip " + std::to_string(i)
                + " decompressed to " + std::to_string(
                    decompressed.size())
                + " bytes, expected " + std::to_string(
                    expected_strip_bytes));
        }
        samples.insert(samples.end(),
                       decompressed.begin(), decompressed.end());
        y0 += strip_rows;
    }
    if (samples.size() != total_samples) {
        throw std::runtime_error(
            "tiff_decoder: total decompressed samples "
            + std::to_string(samples.size()) + " != expected "
            + std::to_string(total_samples));
    }

    // 5. Photometric expansion to RGBA8.
    const std::size_t pixel_count =
        static_cast<std::size_t>(image_width) * image_length;
    DecodedImage out;
    out.width = image_width;
    out.height = image_length;
    out.components = 4;
    out.pixels.resize(pixel_count * 4);

    auto* const dst = reinterpret_cast<std::uint8_t*>(
        out.pixels.data());
    const auto* const src = reinterpret_cast<const std::uint8_t*>(
        samples.data());

    if (photometric == kPhotoBlackIsZero && bps == 1) {
        // 1bpp bilevel: unpack MSB-first 8 px/byte. Padding
        // bits past image_width in the last byte of each row
        // are ignored.
        for (std::uint32_t y = 0; y < image_length; ++y) {
            const std::uint8_t* src_row = src + y * row_bytes;
            std::uint8_t* dst_row = dst + y * image_width * 4;
            for (std::uint32_t x = 0; x < image_width; ++x) {
                const std::uint8_t bit =
                    (src_row[x / 8] >> (7 - (x % 8))) & 1u;
                const std::uint8_t v = bit ? 0xFFu : 0x00u;
                dst_row[x * 4 + 0] = v;
                dst_row[x * 4 + 1] = v;
                dst_row[x * 4 + 2] = v;
                dst_row[x * 4 + 3] = 0xFFu;
            }
        }
    } else if (photometric == kPhotoBlackIsZero) {
        // 8bpp grayscale: R = G = B = sample; A = 0xFF.
        for (std::size_t i = 0; i < pixel_count; ++i) {
            const auto y = src[i];
            dst[i * 4 + 0] = y;
            dst[i * 4 + 1] = y;
            dst[i * 4 + 2] = y;
            dst[i * 4 + 3] = 0xFFu;
        }
    } else if (photometric == kPhotoRgb && samples_per_pixel == 3) {
        // Direct 3-channel copy; A = 0xFF.
        for (std::size_t i = 0; i < pixel_count; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 0xFFu;
        }
    } else if (photometric == kPhotoRgb && samples_per_pixel == 4) {
        // Direct 4-channel copy.
        std::memcpy(dst, src, pixel_count * 4);
    } else if (photometric == kPhotoPalette) {
        // Index lookup into ColorMap (3 * 2^bps SHORT entries:
        // all reds, then all greens, then all blues; each entry
        // scaled to 0..65535). High byte of each entry gives the
        // 8-bit channel value. bps=4 unpacks 2 indices/byte
        // high-nibble-first; bps=8 reads one byte per pixel.
        const std::uint32_t map_size = 1u << bps;
        for (std::uint32_t y = 0; y < image_length; ++y) {
            const std::uint8_t* src_row = src + y * row_bytes;
            std::uint8_t* dst_row = dst + y * image_width * 4;
            for (std::uint32_t x = 0; x < image_width; ++x) {
                std::uint8_t idx;
                if (bps == 4) {
                    const std::uint8_t b = src_row[x / 2];
                    idx = static_cast<std::uint8_t>(
                        ((x & 1u) == 0) ? (b >> 4) : (b & 0x0Fu));
                } else {
                    idx = src_row[x];
                }
                const auto r16 = color_map[0 * map_size + idx];
                const auto g16 = color_map[1 * map_size + idx];
                const auto b16 = color_map[2 * map_size + idx];
                dst_row[x * 4 + 0] = static_cast<std::uint8_t>(
                    (r16 >> 8) & 0xFF);
                dst_row[x * 4 + 1] = static_cast<std::uint8_t>(
                    (g16 >> 8) & 0xFF);
                dst_row[x * 4 + 2] = static_cast<std::uint8_t>(
                    (b16 >> 8) & 0xFF);
                dst_row[x * 4 + 3] = 0xFFu;
            }
        }
    } else {
        // Validation above should have prevented this, but
        // surface it explicitly so a future spec relaxation
        // doesn't silently fall through.
        throw std::runtime_error(
            "tiff_decoder: unhandled (Photometric, SamplesPerPixel) "
            "combination");
    }

    return out;
}

}  // namespace foundation::tiff_decoder
