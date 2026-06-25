#include "bmp_encoder.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

// BMP write. Spec authority: Microsoft Open Specifications
// [MS-WMF] Annex F (BITMAPFILEHEADER + BITMAPINFOHEADER) and
// the de-facto Windows Bitmap layout — 14-byte file header,
// 40-byte info header, optional 12-byte BITFIELDS block, then
// pixel data at bfOffBits. Two in-scope variants:
//
//   * 24-bit BI_RGB: BGR triples, rows padded up to a 4-byte
//     boundary, alpha dropped on emit.
//   * 32-bit BI_BITFIELDS: BGRA, three uint32 LE channel masks
//     immediately after the info header (R=00FF0000,
//     G=0000FF00, B=000000FF), no row padding (always
//     4-aligned), alpha preserved.
//
// Single-image format — no multi-image container.

namespace foundation::bmp_encoder {

namespace {

constexpr std::uint32_t kFileHeaderSize = 14;
constexpr std::uint32_t kInfoHeaderSize = 40;
constexpr std::uint32_t kBitfieldsBlockSize = 12;

// BMP per the Microsoft layout is little-endian on disk
// regardless of host byte order; the helpers below pack each
// integer LE explicitly so the body is host-endian-agnostic.
void WriteU16LE(std::vector<std::byte>& out, std::uint16_t v) {
    out.push_back(static_cast<std::byte>(v & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
}

void WriteU32LE(std::vector<std::byte>& out, std::uint32_t v) {
    out.push_back(static_cast<std::byte>(v & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
}

void WriteI32LE(std::vector<std::byte>& out, std::int32_t v) {
    WriteU32LE(out, static_cast<std::uint32_t>(v));
}

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("bmp_encoder: " + msg);
}

constexpr std::uint32_t kMaxDim = 0x7FFFFFFF;  // int32 max

}  // namespace

std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options) {
    // Input validation per the spec error model.
    if (width == 0 || height == 0) {
        Fail("width and height must be > 0");
    }
    if (width > kMaxDim || height > kMaxDim) {
        Fail("width and height must fit in int32");
    }
    const std::uint64_t expected_input =
        static_cast<std::uint64_t>(width) *
        static_cast<std::uint64_t>(height) * 4ULL;
    if (rgba8.size() != expected_input) {
        Fail("rgba8.size() (" + std::to_string(rgba8.size()) +
             ") != width*height*4 (" +
             std::to_string(expected_input) + ")");
    }
    const bool is_bgra = options.color_type == ColorType::Bgra;
    if (!is_bgra && options.color_type != ColorType::Bgr) {
        Fail("unknown color_type");
    }

    // Layout computation.
    const std::uint32_t bytes_per_pixel = is_bgra ? 4 : 3;
    const std::uint32_t row_data_bytes = width * bytes_per_pixel;
    const std::uint32_t row_padding =
        (4 - (row_data_bytes % 4)) % 4;
    const std::uint32_t row_stride = row_data_bytes + row_padding;
    const std::uint32_t pixel_data_bytes = height * row_stride;
    const std::uint32_t bitfields_block =
        is_bgra ? kBitfieldsBlockSize : 0;
    const std::uint32_t bf_off_bits =
        kFileHeaderSize + kInfoHeaderSize + bitfields_block;
    const std::uint32_t bf_size = bf_off_bits + pixel_data_bytes;

    std::vector<std::byte> out;
    out.reserve(bf_size);

    // BITMAPFILEHEADER (14 bytes).
    out.push_back(std::byte{'B'});
    out.push_back(std::byte{'M'});
    WriteU32LE(out, bf_size);
    WriteU16LE(out, 0);  // bfReserved1
    WriteU16LE(out, 0);  // bfReserved2
    WriteU32LE(out, bf_off_bits);

    // BITMAPINFOHEADER (40 bytes).
    constexpr std::uint32_t kBiRgb = 0;
    constexpr std::uint32_t kBiBitfields = 3;
    // 2835 ≈ 72 DPI in pixels-per-meter; the de-facto default
    // emitted by mature BMP writers (zero is permissible per
    // the spec but 2835 maximises compatibility with viewers
    // that key off the resolution tags).
    constexpr std::int32_t kDefaultPpm = 2835;
    WriteU32LE(out, kInfoHeaderSize);
    WriteI32LE(out, static_cast<std::int32_t>(width));
    WriteI32LE(out, static_cast<std::int32_t>(height));  // +ve = bottom-up
    WriteU16LE(out, 1);  // biPlanes
    WriteU16LE(out, is_bgra ? 32 : 24);
    WriteU32LE(out, is_bgra ? kBiBitfields : kBiRgb);
    WriteU32LE(out, pixel_data_bytes);
    WriteI32LE(out, kDefaultPpm);
    WriteI32LE(out, kDefaultPpm);
    WriteU32LE(out, 0);  // biClrUsed
    WriteU32LE(out, 0);  // biClrImportant

    // BITFIELDS block (12 bytes, BGRA only).
    if (is_bgra) {
        WriteU32LE(out, 0x00FF0000);  // RedMask
        WriteU32LE(out, 0x0000FF00);  // GreenMask
        WriteU32LE(out, 0x000000FF);  // BlueMask
    }

    // Pixel data, bottom-up. Walk source rows in reverse order
    // (file row 0 = image bottom). Each pixel translates RGBA →
    // BGR (24-bit) or RGBA → BGRA (32-bit).
    const std::byte* src = rgba8.data();
    for (std::int64_t y = static_cast<std::int64_t>(height) - 1;
         y >= 0; --y) {
        const std::byte* row =
            src + static_cast<std::size_t>(y) *
                      static_cast<std::size_t>(width) * 4;
        if (is_bgra) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::byte* p = row + x * 4;
                out.push_back(p[2]);  // B
                out.push_back(p[1]);  // G
                out.push_back(p[0]);  // R
                out.push_back(p[3]);  // A
            }
        } else {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::byte* p = row + x * 4;
                out.push_back(p[2]);  // B
                out.push_back(p[1]);  // G
                out.push_back(p[0]);  // R
            }
            for (std::uint32_t k = 0; k < row_padding; ++k) {
                out.push_back(std::byte{0});
            }
        }
    }

    return out;
}

}  // namespace foundation::bmp_encoder
