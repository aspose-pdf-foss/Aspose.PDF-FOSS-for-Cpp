#include "bmp_decoder.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

// BMP read. Spec authority: Microsoft Open Specifications
// [MS-WMF] Annex F (BITMAPFILEHEADER + BITMAPINFOHEADER) and
// the de-facto Windows Bitmap layout — 14-byte file header,
// 40-byte info header (biSize == 40), optional 12-byte
// BITFIELDS block, then pixel data at bfOffBits. Two in-scope
// variants:
//
//   * 24-bit BI_RGB: BGR triples, rows padded to 4-byte
//     boundary, alpha widened to 0xFF on read.
//   * 32-bit BI_BITFIELDS: BGRA, three uint32 LE channel
//     masks immediately after the info header (R=00FF0000,
//     G=0000FF00, B=000000FF), no row padding, alpha
//     preserved.
//
// Out of scope: 1/2/4/8-bit indexed, 16-bit RGB555/565, RLE
// compression, JPEG/PNG payload variants, top-down scanlines
// (negative biHeight), BITMAPCOREHEADER, BITMAPV4/V5HEADER.
// Anything outside the two in-scope (biBitCount,
// biCompression) pairs raises.

namespace foundation::bmp_decoder {

namespace {

constexpr std::size_t kFileHeaderSize = 14;
constexpr std::size_t kInfoHeaderSize = 40;
constexpr std::size_t kBitfieldsBlockSize = 12;
constexpr std::size_t kBfOffBitsMinRgb = kFileHeaderSize + kInfoHeaderSize;
constexpr std::size_t kBfOffBitsMinBitfields =
    kFileHeaderSize + kInfoHeaderSize + kBitfieldsBlockSize;
constexpr std::uint32_t kBiRgb = 0;
constexpr std::uint32_t kBiBitfields = 3;
constexpr std::uint32_t kCanonicalRedMask = 0x00FF0000;
constexpr std::uint32_t kCanonicalGreenMask = 0x0000FF00;
constexpr std::uint32_t kCanonicalBlueMask = 0x000000FF;

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("bmp_decoder: " + msg);
}

std::uint16_t ReadU16LE(std::span<const std::byte> b, std::size_t off) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint8_t>(b[off]) |
        (static_cast<std::uint8_t>(b[off + 1]) << 8));
}

std::uint32_t ReadU32LE(std::span<const std::byte> b, std::size_t off) {
    return static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(b[off]) |
        (static_cast<std::uint8_t>(b[off + 1]) << 8) |
        (static_cast<std::uint8_t>(b[off + 2]) << 16) |
        (static_cast<std::uint8_t>(b[off + 3]) << 24));
}

std::int32_t ReadI32LE(std::span<const std::byte> b, std::size_t off) {
    return static_cast<std::int32_t>(ReadU32LE(b, off));
}

}  // namespace

DecodedImage Decode(std::span<const std::byte> input) {
    // BITMAPFILEHEADER fits.
    if (input.size() < kFileHeaderSize) {
        Fail("input shorter than 14-byte BITMAPFILEHEADER");
    }
    // Signature.
    if (static_cast<std::uint8_t>(input[0]) != 0x42 ||  // 'B'
        static_cast<std::uint8_t>(input[1]) != 0x4D) {  // 'M'
        Fail("bfType != 'BM'");
    }
    // bytes 2..5 bfSize, 6..7 bfReserved1, 8..9 bfReserved2 — ignored
    // per spec (real-world writers occasionally miscompute bfSize).
    const std::uint32_t bf_off_bits = ReadU32LE(input, 10);

    // BITMAPINFOHEADER fits.
    if (input.size() < kFileHeaderSize + kInfoHeaderSize) {
        Fail("input shorter than 14 + 40 = 54 bytes");
    }
    const std::uint32_t bi_size = ReadU32LE(input, 14);
    if (bi_size != kInfoHeaderSize) {
        Fail("biSize != 40 (got " + std::to_string(bi_size) +
             ") — only the 40-byte BITMAPINFOHEADER is in scope");
    }
    const std::int32_t bi_width_signed = ReadI32LE(input, 18);
    const std::int32_t bi_height_signed = ReadI32LE(input, 22);
    if (bi_width_signed <= 0) {
        Fail("biWidth <= 0");
    }
    if (bi_height_signed <= 0) {
        Fail("biHeight <= 0 (top-down BMPs out of scope)");
    }
    const std::uint32_t width = static_cast<std::uint32_t>(bi_width_signed);
    const std::uint32_t height = static_cast<std::uint32_t>(bi_height_signed);
    const std::uint16_t bi_planes = ReadU16LE(input, 26);
    if (bi_planes != 1) {
        Fail("biPlanes != 1 (got " + std::to_string(bi_planes) + ")");
    }
    const std::uint16_t bi_bit_count = ReadU16LE(input, 28);
    if (bi_bit_count != 24 && bi_bit_count != 32) {
        Fail("biBitCount not in {24, 32} (got " +
             std::to_string(bi_bit_count) + ")");
    }
    const std::uint32_t bi_compression = ReadU32LE(input, 30);
    const bool is_bgra =
        (bi_bit_count == 32 && bi_compression == kBiBitfields);
    const bool is_bgr =
        (bi_bit_count == 24 && bi_compression == kBiRgb);
    if (!is_bgr && !is_bgra) {
        Fail("(biBitCount, biCompression) not in {(24, BI_RGB), "
             "(32, BI_BITFIELDS)} (got (" +
             std::to_string(bi_bit_count) + ", " +
             std::to_string(bi_compression) + "))");
    }
    // bytes 34..37 biSizeImage — advisory; ignored per spec.
    // bytes 38..41 biXPelsPerMeter, 42..45 biYPelsPerMeter — ignored.
    const std::uint32_t bi_clr_used = ReadU32LE(input, 46);
    if (bi_clr_used != 0) {
        Fail("biClrUsed != 0 (palette BMPs out of scope)");
    }
    // bytes 50..53 biClrImportant — ignored.

    // BI_BITFIELDS masks if 32-bit.
    if (is_bgra) {
        if (input.size() < kBfOffBitsMinBitfields) {
            Fail("input shorter than 14 + 40 + 12 = 66 bytes "
                 "(BITFIELDS masks won't fit)");
        }
        const std::uint32_t r_mask = ReadU32LE(input, 54);
        const std::uint32_t g_mask = ReadU32LE(input, 58);
        const std::uint32_t b_mask = ReadU32LE(input, 62);
        if (r_mask != kCanonicalRedMask ||
            g_mask != kCanonicalGreenMask ||
            b_mask != kCanonicalBlueMask) {
            Fail("BITFIELDS masks not canonical "
                 "(R=00FF0000, G=0000FF00, B=000000FF)");
        }
    }

    // bfOffBits lower-bound check.
    const std::size_t min_off_bits =
        is_bgra ? kBfOffBitsMinBitfields : kBfOffBitsMinRgb;
    if (bf_off_bits < min_off_bits) {
        Fail("bfOffBits below the minimum (" +
             std::to_string(bf_off_bits) + " < " +
             std::to_string(min_off_bits) + ")");
    }

    // Pixel data layout.
    const std::uint32_t bytes_per_pixel = is_bgra ? 4 : 3;
    const std::uint64_t row_data_bytes =
        static_cast<std::uint64_t>(width) *
        static_cast<std::uint64_t>(bytes_per_pixel);
    const std::uint64_t row_padding =
        (4 - (row_data_bytes % 4)) % 4;
    const std::uint64_t row_stride = row_data_bytes + row_padding;
    const std::uint64_t pixel_data_bytes =
        static_cast<std::uint64_t>(height) * row_stride;
    const std::uint64_t end_offset =
        static_cast<std::uint64_t>(bf_off_bits) + pixel_data_bytes;
    if (end_offset > input.size()) {
        Fail("declared pixel data runs past EOF (bfOffBits + "
             "height*row_stride = " + std::to_string(end_offset) +
             " > input.size() = " + std::to_string(input.size()) + ")");
    }

    // Output assembly. Walk file rows in reverse (file row 0 =
    // image bottom; file row height-1 = image top) so the
    // emitted RGBA buffer is row-major top-to-bottom.
    DecodedImage out;
    out.width = width;
    out.height = height;
    out.components = 4;
    out.pixels.resize(static_cast<std::size_t>(width) *
                      static_cast<std::size_t>(height) * 4);

    const std::byte* base = input.data() + bf_off_bits;
    for (std::uint32_t img_y = 0; img_y < height; ++img_y) {
        const std::uint64_t file_row = height - 1 - img_y;
        const std::byte* src_row = base + file_row * row_stride;
        std::byte* dst_row =
            out.pixels.data() +
            static_cast<std::size_t>(img_y) *
                static_cast<std::size_t>(width) * 4;
        if (is_bgra) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::byte* p = src_row + x * 4;
                std::byte* q = dst_row + x * 4;
                q[0] = p[2];  // R from B-G-R-A position 2
                q[1] = p[1];  // G
                q[2] = p[0];  // B
                q[3] = p[3];  // A preserved
            }
        } else {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::byte* p = src_row + x * 3;
                std::byte* q = dst_row + x * 4;
                q[0] = p[2];  // R from B-G-R position 2
                q[1] = p[1];  // G
                q[2] = p[0];  // B
                q[3] = std::byte{0xFF};  // A widened
            }
        }
    }

    return out;
}

}  // namespace foundation::bmp_decoder
