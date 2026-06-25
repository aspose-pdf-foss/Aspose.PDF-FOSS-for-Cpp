#include "png_decoder.hpp"

#include "flate.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>

// PNG 1.2 baseline read. Spec authority: ISO/IEC 15948:2003 /
// W3C PNG 1.2 §3 (file structure), §4 (chunk specifications:
// IHDR, IDAT, IEND, PLTE), §6 (filter algorithms — five
// inverse paths), §9 (filtering: Paeth predictor, edge
// handling), §10 (compression: Deflate via zlib), §13 / Annex E
// (CRC algorithm — IEEE 802.3 reversed polynomial 0xEDB88320).
// IDAT decompression delegates to foundation::flate::Decode.
//
// In scope: 8-bit samples; color types Grayscale (0), Rgb (2),
// Rgba (6); all five per-scanline filter inverse paths; non-
// interlaced layout. Ancillary chunks (tEXt, pHYs, gAMA,
// cHRM, sRGB, iCCP, sBIT, etc.) are skipped silently — the
// primitive does not inspect them, but their CRCs are still
// verified during the chunk walk so a corrupted stream is
// detected even when the corruption hits an ignored chunk.
//
// Out of scope: bit depths != 8, palette images (color type 3),
// grayscale+alpha (color type 4), Adam7 interlace, multi-frame
// APNG. Out-of-scope features raise with a message naming the
// missing capability.

namespace foundation::png_decoder {

namespace {

constexpr std::array<std::uint8_t, 8> kSignature = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
};

constexpr std::uint32_t kMaxDim = 0x7FFFFFFF;

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("png_decoder: " + msg);
}

// CRC-32 (IEEE 802.3 reversed polynomial 0xEDB88320). Same
// algorithm + table the encoder uses; for v1 each primitive
// keeps its own copy to keep the .cpp self-contained.
const std::array<std::uint32_t, 256>& CrcTable() {
    static const std::array<std::uint32_t, 256> table = []() {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[n] = c;
        }
        return t;
    }();
    return table;
}

std::uint32_t Crc32(std::span<const std::byte> data) {
    const auto& table = CrcTable();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (auto b : data) {
        crc = table[(crc ^ static_cast<std::uint8_t>(b)) & 0xFF]
              ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

std::uint32_t ReadU32BE(std::span<const std::byte> b, std::size_t off) {
    return (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(b[off + 0])) << 24) |
           (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(b[off + 1])) << 16) |
           (static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(b[off + 2])) << 8) |
           static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(b[off + 3]));
}

bool TypeEquals(std::span<const std::byte> b, std::size_t off,
                const char type[4]) {
    return static_cast<char>(b[off + 0]) == type[0] &&
           static_cast<char>(b[off + 1]) == type[1] &&
           static_cast<char>(b[off + 2]) == type[2] &&
           static_cast<char>(b[off + 3]) == type[3];
}

// PNG §9.4 PaethPredictor (a, b, c). All inputs are unsigned
// bytes; intermediate arithmetic is signed 32-bit so the |·|
// distances are exact. Returns one of the three input bytes.
std::uint8_t PaethPredictor(std::uint8_t a, std::uint8_t b,
                            std::uint8_t c) {
    const int p = static_cast<int>(a) + static_cast<int>(b) -
                  static_cast<int>(c);
    const int pa = std::abs(p - static_cast<int>(a));
    const int pb = std::abs(p - static_cast<int>(b));
    const int pc = std::abs(p - static_cast<int>(c));
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

// Apply one scanline's filter inverse in place. `row_start` and
// `prior_start` index into the unfiltered output buffer; the
// caller passes nullptr for prior_start on the first scanline
// (PNG §9.2: prior bytes treated as zero). bpp is "filter
// distance" — for 8-bit color types this matches the per-pixel
// byte count.
void UnfilterScanline(std::uint8_t filter_type,
                      std::uint8_t* row, const std::uint8_t* prior,
                      std::size_t row_data_bytes, std::uint32_t bpp) {
    auto prior_byte = [&](std::size_t i) -> std::uint8_t {
        return prior ? prior[i] : 0;
    };
    auto left_byte = [&](std::size_t i) -> std::uint8_t {
        return (i >= bpp) ? row[i - bpp] : 0;
    };
    auto upper_left_byte = [&](std::size_t i) -> std::uint8_t {
        return (prior && i >= bpp) ? prior[i - bpp] : 0;
    };
    switch (filter_type) {
        case 0:  // None
            // raw[i] = filtered[i] — already in place after the
            // initial copy from the decompressed stream; nothing
            // to do.
            break;
        case 1:  // Sub
            for (std::size_t i = 0; i < row_data_bytes; ++i) {
                row[i] = static_cast<std::uint8_t>(
                    row[i] + left_byte(i));
            }
            break;
        case 2:  // Up
            for (std::size_t i = 0; i < row_data_bytes; ++i) {
                row[i] = static_cast<std::uint8_t>(
                    row[i] + prior_byte(i));
            }
            break;
        case 3:  // Average
            for (std::size_t i = 0; i < row_data_bytes; ++i) {
                const int avg =
                    (static_cast<int>(left_byte(i)) +
                     static_cast<int>(prior_byte(i))) / 2;
                row[i] = static_cast<std::uint8_t>(
                    row[i] + static_cast<std::uint8_t>(avg));
            }
            break;
        case 4:  // Paeth
            for (std::size_t i = 0; i < row_data_bytes; ++i) {
                row[i] = static_cast<std::uint8_t>(
                    row[i] + PaethPredictor(left_byte(i),
                                            prior_byte(i),
                                            upper_left_byte(i)));
            }
            break;
        default:
            Fail("filter type byte not in {0..4} (got " +
                 std::to_string(filter_type) + ")");
    }
}

}  // namespace

DecodedImage Decode(std::span<const std::byte> input) {
    // Signature.
    if (input.size() < kSignature.size()) {
        Fail("input shorter than 8-byte PNG signature");
    }
    for (std::size_t i = 0; i < kSignature.size(); ++i) {
        if (static_cast<std::uint8_t>(input[i]) != kSignature[i]) {
            Fail("PNG signature mismatch at byte " +
                 std::to_string(i));
        }
    }

    // Chunk walk.
    std::vector<std::byte> idat_concat;
    bool ihdr_seen = false;
    bool iend_seen = false;
    std::uint32_t width = 0, height = 0;
    std::uint8_t color_type = 0;

    std::size_t cursor = kSignature.size();
    while (cursor < input.size()) {
        if (cursor + 8 > input.size()) {
            Fail("chunk header truncated at offset " +
                 std::to_string(cursor));
        }
        const std::uint32_t length = ReadU32BE(input, cursor);
        if (length > kMaxDim) {
            Fail("chunk length > 2^31 - 1 at offset " +
                 std::to_string(cursor));
        }
        const std::size_t type_off = cursor + 4;
        const std::size_t data_off = cursor + 8;
        const std::size_t crc_off = data_off + length;
        if (crc_off + 4 > input.size()) {
            Fail("chunk truncated (length " +
                 std::to_string(length) + ") at offset " +
                 std::to_string(cursor));
        }

        // CRC-32 over (type | data).
        const std::uint32_t expected_crc = ReadU32BE(input, crc_off);
        const std::uint32_t actual_crc = Crc32(
            std::span<const std::byte>(
                input.data() + type_off, 4 + length));
        if (actual_crc != expected_crc) {
            Fail("CRC mismatch at chunk offset " +
                 std::to_string(cursor));
        }

        if (TypeEquals(input, type_off, "IHDR")) {
            if (ihdr_seen) {
                Fail("duplicate IHDR chunk");
            }
            if (length != 13) {
                Fail("IHDR length != 13 (got " +
                     std::to_string(length) + ")");
            }
            width = ReadU32BE(input, data_off + 0);
            height = ReadU32BE(input, data_off + 4);
            const std::uint8_t bit_depth =
                static_cast<std::uint8_t>(input[data_off + 8]);
            color_type =
                static_cast<std::uint8_t>(input[data_off + 9]);
            const std::uint8_t compression =
                static_cast<std::uint8_t>(input[data_off + 10]);
            const std::uint8_t filter =
                static_cast<std::uint8_t>(input[data_off + 11]);
            const std::uint8_t interlace =
                static_cast<std::uint8_t>(input[data_off + 12]);
            if (width == 0 || height == 0) {
                Fail("IHDR width/height must be > 0");
            }
            if (width > kMaxDim || height > kMaxDim) {
                Fail("IHDR width/height must fit in int32");
            }
            if (bit_depth != 8) {
                Fail("bit_depth != 8 (got " +
                     std::to_string(bit_depth) +
                     ") — only 8-bit samples in scope");
            }
            if (color_type != 0 && color_type != 2 &&
                color_type != 6) {
                Fail("color_type not in {0 (Grayscale), 2 (Rgb), "
                     "6 (Rgba)} (got " +
                     std::to_string(color_type) +
                     ") — palette / grayscale+alpha out of scope");
            }
            if (compression != 0) {
                Fail("compression != 0 (got " +
                     std::to_string(compression) + ")");
            }
            if (filter != 0) {
                Fail("filter != 0 (got " +
                     std::to_string(filter) + ")");
            }
            if (interlace != 0) {
                Fail("interlace != 0 (got " +
                     std::to_string(interlace) +
                     ") — Adam7 out of scope");
            }
            ihdr_seen = true;
        } else if (TypeEquals(input, type_off, "IDAT")) {
            if (!ihdr_seen) {
                Fail("IDAT before IHDR");
            }
            if (iend_seen) {
                Fail("IDAT after IEND");
            }
            const std::byte* data_ptr = input.data() + data_off;
            idat_concat.insert(idat_concat.end(),
                               data_ptr, data_ptr + length);
        } else if (TypeEquals(input, type_off, "IEND")) {
            if (length != 0) {
                Fail("IEND length != 0");
            }
            iend_seen = true;
        } else {
            // Ancillary chunk — silently skipped (CRC already
            // verified above).
        }

        cursor = crc_off + 4;
        if (iend_seen) break;
    }

    if (!ihdr_seen) Fail("missing IHDR chunk");
    if (idat_concat.empty()) Fail("missing IDAT chunk");
    if (!iend_seen) Fail("missing IEND chunk");

    // Decompress IDAT.
    const auto decompressed = foundation::flate::Decode(
        std::span<const std::byte>(idat_concat));

    // Validate decompressed length matches expected scanline shape.
    const std::uint32_t bpp = (color_type == 0) ? 1 :
                              (color_type == 2) ? 3 : 4;
    const std::size_t row_data_bytes =
        static_cast<std::size_t>(width) * bpp;
    const std::size_t row_stride = 1 + row_data_bytes;  // 1-byte filter prefix
    const std::size_t expected_total =
        static_cast<std::size_t>(height) * row_stride;
    if (decompressed.size() != expected_total) {
        Fail("decompressed IDAT length " +
             std::to_string(decompressed.size()) +
             " != expected " + std::to_string(expected_total));
    }

    // Filter inverse: produce per-scanline raw bytes (no filter
    // prefix) into a working buffer, then expand into RGBA8.
    std::vector<std::uint8_t> raw(
        static_cast<std::size_t>(height) * row_data_bytes);
    const std::uint8_t* prior_row = nullptr;
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::size_t fline_off = y * row_stride;
        const std::uint8_t filter_type =
            static_cast<std::uint8_t>(decompressed[fline_off]);
        std::uint8_t* row = raw.data() + y * row_data_bytes;
        // Copy filtered bytes (skip filter prefix) into the
        // working buffer; UnfilterScanline updates row in place.
        for (std::size_t i = 0; i < row_data_bytes; ++i) {
            row[i] = static_cast<std::uint8_t>(
                decompressed[fline_off + 1 + i]);
        }
        UnfilterScanline(filter_type, row, prior_row,
                         row_data_bytes, bpp);
        prior_row = row;
    }

    // Photometric expansion → RGBA8.
    DecodedImage out;
    out.width = width;
    out.height = height;
    out.components = 4;
    out.pixels.resize(static_cast<std::size_t>(width) *
                      static_cast<std::size_t>(height) * 4);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* src = raw.data() + y * row_data_bytes;
        std::byte* dst =
            out.pixels.data() +
            static_cast<std::size_t>(y) *
                static_cast<std::size_t>(width) * 4;
        for (std::uint32_t x = 0; x < width; ++x) {
            std::byte* q = dst + x * 4;
            switch (color_type) {
                case 0: {  // Grayscale
                    const std::uint8_t s = src[x];
                    q[0] = std::byte{s};
                    q[1] = std::byte{s};
                    q[2] = std::byte{s};
                    q[3] = std::byte{0xFF};
                    break;
                }
                case 2: {  // Rgb
                    const std::uint8_t* p = src + x * 3;
                    q[0] = std::byte{p[0]};
                    q[1] = std::byte{p[1]};
                    q[2] = std::byte{p[2]};
                    q[3] = std::byte{0xFF};
                    break;
                }
                case 6: {  // Rgba
                    const std::uint8_t* p = src + x * 4;
                    q[0] = std::byte{p[0]};
                    q[1] = std::byte{p[1]};
                    q[2] = std::byte{p[2]};
                    q[3] = std::byte{p[3]};
                    break;
                }
            }
        }
    }

    return out;
}

}  // namespace foundation::png_decoder
