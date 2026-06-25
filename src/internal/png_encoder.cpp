#include "png_encoder.hpp"

#include "flate.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

// PNG 1.2 baseline write. Spec authority: ISO/IEC 15948:2003 /
// W3C PNG 1.2 §3 (file structure), §4 (chunk specifications:
// IHDR, IDAT, IEND), §6 (filter algorithms — encode emits
// filter type 0 only), §10 (compression: Deflate via zlib),
// §13 / Annex E (CRC algorithm — IEEE 802.3 reversed polynomial
// 0xEDB88320). IDAT compression delegates to
// foundation::flate::Encode (zlib-wrapped Deflate per
// RFC 1950 / 1951). CRC-32 is computed inline via a 256-entry
// table; csharp ships the same inline table for the same
// reason (no other primitive in the lib currently needs CRC-32,
// so pulling it through foundation::digest would add ceremony
// without enabling reuse).

namespace foundation::png_encoder {

namespace {

constexpr std::array<std::byte, 8> kSignature = {
    std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
    std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
};

constexpr std::uint32_t kMaxDim = 0x7FFFFFFF;  // int32 max per PNG §11.2.2

[[noreturn]] void Fail(const std::string& msg) {
    throw std::runtime_error("png_encoder: " + msg);
}

// CRC-32 (IEEE 802.3 reversed polynomial 0xEDB88320). Built
// once at first use via constexpr-initialised function-local
// static. Same algorithm PNG §13 + Annex E specifies.
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

// CRC-32 over an arbitrary byte sequence; PNG callers chain
// this over (chunk type | chunk data) to produce the trailing
// 4-byte CRC of every chunk.
std::uint32_t Crc32(std::span<const std::byte> data) {
    const auto& table = CrcTable();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (auto b : data) {
        crc = table[(crc ^ static_cast<std::uint8_t>(b)) & 0xFF]
              ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void WriteU32BE(std::vector<std::byte>& out, std::uint32_t v) {
    out.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::byte>(v & 0xFF));
}

// Append one PNG chunk: length(4) + type(4) + data + CRC(4)
// over (type || data). PNG §3.2.
void AppendChunk(std::vector<std::byte>& out,
                 const char type[4],
                 std::span<const std::byte> data) {
    if (data.size() > kMaxDim) {
        Fail("chunk data length > 2^31 - 1");
    }
    WriteU32BE(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t crc_start = out.size();
    out.push_back(static_cast<std::byte>(type[0]));
    out.push_back(static_cast<std::byte>(type[1]));
    out.push_back(static_cast<std::byte>(type[2]));
    out.push_back(static_cast<std::byte>(type[3]));
    out.insert(out.end(), data.begin(), data.end());
    const std::uint32_t crc = Crc32(std::span<const std::byte>(
        out.data() + crc_start, out.size() - crc_start));
    WriteU32BE(out, crc);
}

// Sample-reduction: project the input RGBA8 buffer into the
// per-color-type sample stream prepended with the per-scanline
// filter byte (always 0 — filter type None per spec yaml's
// scope: "encode emits filter type 0 only"). Total emitted
// length is height * (1 + width * bpp).
std::vector<std::byte> BuildFilteredStream(
        std::uint32_t width, std::uint32_t height,
        std::span<const std::byte> rgba8, ColorType color_type) {
    std::uint32_t bpp;
    switch (color_type) {
        case ColorType::Grayscale: bpp = 1; break;
        case ColorType::Rgb:       bpp = 3; break;
        case ColorType::Rgba:      bpp = 4; break;
        default: Fail("unknown color_type");
    }
    const std::size_t row_data_bytes =
        static_cast<std::size_t>(width) * bpp;
    std::vector<std::byte> filtered;
    filtered.reserve(static_cast<std::size_t>(height) *
                     (1 + row_data_bytes));
    for (std::uint32_t y = 0; y < height; ++y) {
        filtered.push_back(std::byte{0});  // filter type 0 (None)
        const std::byte* row =
            rgba8.data() + static_cast<std::size_t>(y) *
                               static_cast<std::size_t>(width) * 4;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::byte* p = row + x * 4;
            switch (color_type) {
                case ColorType::Grayscale: {
                    // Spec contract: R == G == B required.
                    if (p[0] != p[1] || p[1] != p[2]) {
                        Fail("color_type Grayscale requires R == G == B "
                             "at every pixel; mismatch at (" +
                             std::to_string(x) + ", " +
                             std::to_string(y) + ")");
                    }
                    filtered.push_back(p[0]);
                    break;
                }
                case ColorType::Rgb:
                    filtered.push_back(p[0]);
                    filtered.push_back(p[1]);
                    filtered.push_back(p[2]);
                    break;
                case ColorType::Rgba:
                    filtered.push_back(p[0]);
                    filtered.push_back(p[1]);
                    filtered.push_back(p[2]);
                    filtered.push_back(p[3]);
                    break;
                default:
                    Fail("unknown color_type");
            }
        }
    }
    return filtered;
}

}  // namespace

std::vector<std::byte> Encode(std::uint32_t width,
                              std::uint32_t height,
                              std::span<const std::byte> rgba8,
                              const Options& options) {
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
    if (options.color_type != ColorType::Grayscale &&
        options.color_type != ColorType::Rgb &&
        options.color_type != ColorType::Rgba) {
        Fail("unknown color_type");
    }

    std::vector<std::byte> out;

    // PNG signature.
    out.insert(out.end(), kSignature.begin(), kSignature.end());

    // IHDR chunk (13-byte payload). PNG §11.2.2.
    std::array<std::byte, 13> ihdr{};
    ihdr[0] = static_cast<std::byte>((width >> 24) & 0xFF);
    ihdr[1] = static_cast<std::byte>((width >> 16) & 0xFF);
    ihdr[2] = static_cast<std::byte>((width >> 8) & 0xFF);
    ihdr[3] = static_cast<std::byte>(width & 0xFF);
    ihdr[4] = static_cast<std::byte>((height >> 24) & 0xFF);
    ihdr[5] = static_cast<std::byte>((height >> 16) & 0xFF);
    ihdr[6] = static_cast<std::byte>((height >> 8) & 0xFF);
    ihdr[7] = static_cast<std::byte>(height & 0xFF);
    ihdr[8] = std::byte{8};  // bit depth
    ihdr[9] = static_cast<std::byte>(
        static_cast<std::uint8_t>(options.color_type));
    ihdr[10] = std::byte{0};  // compression — Deflate
    ihdr[11] = std::byte{0};  // filter — set 0
    ihdr[12] = std::byte{0};  // interlace — none
    AppendChunk(out, "IHDR", std::span<const std::byte>(ihdr));

    // Build the filtered byte stream + flate-compress for IDAT.
    const auto filtered = BuildFilteredStream(
        width, height, rgba8, options.color_type);
    const auto idat = foundation::flate::Encode(
        std::span<const std::byte>(filtered));
    AppendChunk(out, "IDAT", std::span<const std::byte>(idat));

    // IEND chunk (empty payload).
    AppendChunk(out, "IEND", std::span<const std::byte>{});

    return out;
}

}  // namespace foundation::png_encoder
