#include "encoding.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace foundation::encoding {

namespace {

constexpr std::uint32_t kReplacementCp = 0xFFFDu;
constexpr std::uint8_t  kReplacementByte = 0x3Fu;  // '?'

// ---------------------------------------------------------------------
// UTF-8 codec helpers — these convert to / from Unicode codepoint
// vectors, the internal representation used by every charset's encode
// path. The primitive's input/output text strings are themselves UTF-8
// (cpp lib convention), so the UTF8 charset's GetBytes / GetString
// effectively pass-through after a validation pass that replaces
// malformed bytes with U+FFFD.

std::vector<std::uint32_t> _utf8_decode(std::string_view text) {
    std::vector<std::uint32_t> out;
    out.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        std::uint8_t b0 = static_cast<std::uint8_t>(text[i]);
        std::uint32_t cp = kReplacementCp;
        std::size_t consumed = 1;

        if (b0 < 0x80u) {
            cp = b0;
        } else if ((b0 & 0xE0u) == 0xC0u && i + 1 < text.size()) {
            std::uint8_t b1 = static_cast<std::uint8_t>(text[i + 1]);
            if ((b1 & 0xC0u) == 0x80u) {
                std::uint32_t v =
                    (static_cast<std::uint32_t>(b0 & 0x1Fu) << 6) |
                     static_cast<std::uint32_t>(b1 & 0x3Fu);
                if (v >= 0x80u) { cp = v; consumed = 2; }
            }
        } else if ((b0 & 0xF0u) == 0xE0u && i + 2 < text.size()) {
            std::uint8_t b1 = static_cast<std::uint8_t>(text[i + 1]);
            std::uint8_t b2 = static_cast<std::uint8_t>(text[i + 2]);
            if (((b1 & 0xC0u) == 0x80u) && ((b2 & 0xC0u) == 0x80u)) {
                std::uint32_t v =
                    (static_cast<std::uint32_t>(b0 & 0x0Fu) << 12) |
                    (static_cast<std::uint32_t>(b1 & 0x3Fu) << 6) |
                     static_cast<std::uint32_t>(b2 & 0x3Fu);
                if (v >= 0x800u && (v < 0xD800u || v > 0xDFFFu)) {
                    cp = v; consumed = 3;
                }
            }
        } else if ((b0 & 0xF8u) == 0xF0u && i + 3 < text.size()) {
            std::uint8_t b1 = static_cast<std::uint8_t>(text[i + 1]);
            std::uint8_t b2 = static_cast<std::uint8_t>(text[i + 2]);
            std::uint8_t b3 = static_cast<std::uint8_t>(text[i + 3]);
            if (((b1 & 0xC0u) == 0x80u) && ((b2 & 0xC0u) == 0x80u) &&
                ((b3 & 0xC0u) == 0x80u)) {
                std::uint32_t v =
                    (static_cast<std::uint32_t>(b0 & 0x07u) << 18) |
                    (static_cast<std::uint32_t>(b1 & 0x3Fu) << 12) |
                    (static_cast<std::uint32_t>(b2 & 0x3Fu) << 6) |
                     static_cast<std::uint32_t>(b3 & 0x3Fu);
                if (v >= 0x10000u && v <= 0x10FFFFu) {
                    cp = v; consumed = 4;
                }
            }
        }

        out.push_back(cp);
        i += consumed;
    }
    return out;
}

void _utf8_emit(std::string& out, std::uint32_t cp) {
    if (cp >= 0xD800u && cp <= 0xDFFFu) cp = kReplacementCp;
    if (cp > 0x10FFFFu)                  cp = kReplacementCp;
    if (cp < 0x80u) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800u) {
        out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else if (cp < 0x10000u) {
        out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
}

std::string _codepoints_to_utf8(std::span<const std::uint32_t> cps) {
    std::string out;
    out.reserve(cps.size());
    for (std::uint32_t cp : cps) _utf8_emit(out, cp);
    return out;
}

// ---------------------------------------------------------------------
// Windows-1252 slot map for 0x80..0x9F. 0xFFFD entries mark the five
// undefined byte slots (0x81, 0x8D, 0x8F, 0x90, 0x9D).

constexpr std::array<std::uint16_t, 32> kWin1252HighHalf = {
    /* 0x80 */ 0x20AC, /* 0x81 */ 0xFFFD, /* 0x82 */ 0x201A, /* 0x83 */ 0x0192,
    /* 0x84 */ 0x201E, /* 0x85 */ 0x2026, /* 0x86 */ 0x2020, /* 0x87 */ 0x2021,
    /* 0x88 */ 0x02C6, /* 0x89 */ 0x2030, /* 0x8A */ 0x0160, /* 0x8B */ 0x2039,
    /* 0x8C */ 0x0152, /* 0x8D */ 0xFFFD, /* 0x8E */ 0x017D, /* 0x8F */ 0xFFFD,
    /* 0x90 */ 0xFFFD, /* 0x91 */ 0x2018, /* 0x92 */ 0x2019, /* 0x93 */ 0x201C,
    /* 0x94 */ 0x201D, /* 0x95 */ 0x2022, /* 0x96 */ 0x2013, /* 0x97 */ 0x2014,
    /* 0x98 */ 0x02DC, /* 0x99 */ 0x2122, /* 0x9A */ 0x0161, /* 0x9B */ 0x203A,
    /* 0x9C */ 0x0153, /* 0x9D */ 0xFFFD, /* 0x9E */ 0x017E, /* 0x9F */ 0x0178,
};

std::uint8_t _win1252_encode_cp(std::uint32_t cp) {
    if (cp < 0x80u) return static_cast<std::uint8_t>(cp);
    if (cp >= 0xA0u && cp <= 0xFFu) return static_cast<std::uint8_t>(cp);
    for (std::size_t i = 0; i < kWin1252HighHalf.size(); ++i) {
        if (kWin1252HighHalf[i] == cp && cp != 0xFFFDu) {
            return static_cast<std::uint8_t>(0x80u + i);
        }
    }
    return kReplacementByte;
}

std::uint32_t _win1252_decode_byte(std::uint8_t b) {
    if (b < 0x80u || b >= 0xA0u) return b;  // ASCII + latin-1 high half
    return kWin1252HighHalf[b - 0x80u];     // 0xFFFD if undefined
}

std::uint8_t _latin1_encode_cp(std::uint32_t cp) {
    return cp <= 0xFFu ? static_cast<std::uint8_t>(cp) : kReplacementByte;
}

}  // namespace

// =====================================================================
// Static factories — each holds a private static-local Encoding instance
// constructed via the private ctor (legal because the factories are
// class members).

const Encoding& Encoding::UTF8() noexcept {
    static const Encoding inst{Kind::UTF8};
    return inst;
}
const Encoding& Encoding::Unicode() noexcept {
    static const Encoding inst{Kind::UTF16LE};
    return inst;
}
const Encoding& Encoding::BigEndianUnicode() noexcept {
    static const Encoding inst{Kind::UTF16BE};
    return inst;
}
const Encoding& Encoding::Latin1() noexcept {
    static const Encoding inst{Kind::LATIN1};
    return inst;
}
const Encoding& Encoding::Windows1252() noexcept {
    static const Encoding inst{Kind::WINDOWS1252};
    return inst;
}

const Encoding& Encoding::GetEncoding(std::string_view name) {
    std::string lower;
    lower.reserve(name.size());
    for (char c : name) {
        lower.push_back(static_cast<char>(std::tolower(
            static_cast<unsigned char>(c))));
    }
    if (lower == "utf-8" || lower == "utf8") return UTF8();
    if (lower == "utf-16" || lower == "utf-16le" || lower == "unicode")
        return Unicode();
    if (lower == "utf-16be" || lower == "unicodefffe")
        return BigEndianUnicode();
    if (lower == "iso-8859-1" || lower == "iso_8859-1" ||
        lower == "latin1") return Latin1();
    if (lower == "windows-1252" || lower == "cp1252") return Windows1252();
    throw std::invalid_argument(
        "foundation::encoding::GetEncoding: unknown name");
}

// =====================================================================
// Codec implementations — switch on private `kind_` member.

std::vector<std::byte> Encoding::GetBytes(std::string_view text) const {
    const auto cps = _utf8_decode(text);
    std::vector<std::byte> out;

    switch (kind_) {
    case Kind::UTF8: {
        std::string s = _codepoints_to_utf8(cps);
        out.reserve(s.size());
        for (char c : s) {
            out.push_back(std::byte{static_cast<std::uint8_t>(c)});
        }
        return out;
    }
    case Kind::UTF16LE: {
        out.reserve(cps.size() * 2);
        for (std::uint32_t cp : cps) {
            if (cp >= 0xD800u && cp <= 0xDFFFu) cp = kReplacementCp;
            if (cp <= 0xFFFFu) {
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(cp & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((cp >> 8) & 0xFFu)});
            } else {
                std::uint32_t s = cp - 0x10000u;
                std::uint16_t hi =
                    static_cast<std::uint16_t>(0xD800u | ((s >> 10) & 0x3FFu));
                std::uint16_t lo =
                    static_cast<std::uint16_t>(0xDC00u | (s & 0x3FFu));
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(hi & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((hi >> 8) & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(lo & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((lo >> 8) & 0xFFu)});
            }
        }
        return out;
    }
    case Kind::UTF16BE: {
        out.reserve(cps.size() * 2);
        for (std::uint32_t cp : cps) {
            if (cp >= 0xD800u && cp <= 0xDFFFu) cp = kReplacementCp;
            if (cp <= 0xFFFFu) {
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((cp >> 8) & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(cp & 0xFFu)});
            } else {
                std::uint32_t s = cp - 0x10000u;
                std::uint16_t hi =
                    static_cast<std::uint16_t>(0xD800u | ((s >> 10) & 0x3FFu));
                std::uint16_t lo =
                    static_cast<std::uint16_t>(0xDC00u | (s & 0x3FFu));
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((hi >> 8) & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(hi & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>((lo >> 8) & 0xFFu)});
                out.push_back(std::byte{
                    static_cast<std::uint8_t>(lo & 0xFFu)});
            }
        }
        return out;
    }
    case Kind::LATIN1: {
        out.reserve(cps.size());
        for (std::uint32_t cp : cps) {
            out.push_back(std::byte{_latin1_encode_cp(cp)});
        }
        return out;
    }
    case Kind::WINDOWS1252: {
        out.reserve(cps.size());
        for (std::uint32_t cp : cps) {
            out.push_back(std::byte{_win1252_encode_cp(cp)});
        }
        return out;
    }
    }
    return out;
}

std::string Encoding::GetString(
    std::span<const std::byte> bytes) const {
    std::vector<std::uint32_t> cps;

    switch (kind_) {
    case Kind::UTF8: {
        std::string_view sv(
            reinterpret_cast<const char*>(bytes.data()), bytes.size());
        cps = _utf8_decode(sv);
        break;
    }
    case Kind::UTF16LE:
    case Kind::UTF16BE: {
        const bool be = (kind_ == Kind::UTF16BE);
        std::size_t i = 0;
        while (i + 1 < bytes.size()) {
            std::uint8_t b0 =
                static_cast<std::uint8_t>(bytes[i]);
            std::uint8_t b1 =
                static_cast<std::uint8_t>(bytes[i + 1]);
            std::uint16_t u = be
                ? static_cast<std::uint16_t>((b0 << 8) | b1)
                : static_cast<std::uint16_t>(b0 | (b1 << 8));
            i += 2;

            if (u >= 0xD800u && u <= 0xDBFFu) {
                if (i + 1 < bytes.size()) {
                    std::uint8_t c0 =
                        static_cast<std::uint8_t>(bytes[i]);
                    std::uint8_t c1 =
                        static_cast<std::uint8_t>(bytes[i + 1]);
                    std::uint16_t lo = be
                        ? static_cast<std::uint16_t>((c0 << 8) | c1)
                        : static_cast<std::uint16_t>(c0 | (c1 << 8));
                    if (lo >= 0xDC00u && lo <= 0xDFFFu) {
                        std::uint32_t cp = 0x10000u +
                            (static_cast<std::uint32_t>(u - 0xD800u) << 10) +
                            (lo - 0xDC00u);
                        cps.push_back(cp);
                        i += 2;
                        continue;
                    }
                }
                cps.push_back(kReplacementCp);
            } else if (u >= 0xDC00u && u <= 0xDFFFu) {
                cps.push_back(kReplacementCp);
            } else {
                cps.push_back(u);
            }
        }
        if (i < bytes.size()) cps.push_back(kReplacementCp);
        break;
    }
    case Kind::LATIN1: {
        cps.reserve(bytes.size());
        for (std::byte b : bytes) {
            cps.push_back(static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(b)));
        }
        break;
    }
    case Kind::WINDOWS1252: {
        cps.reserve(bytes.size());
        for (std::byte b : bytes) {
            cps.push_back(_win1252_decode_byte(
                static_cast<std::uint8_t>(b)));
        }
        break;
    }
    }

    return _codepoints_to_utf8(cps);
}

int Encoding::GetByteCount(std::string_view text) const {
    return static_cast<int>(GetBytes(text).size());
}

int Encoding::GetCharCount(std::span<const std::byte> bytes) const {
    return static_cast<int>(GetString(bytes).size());
}

std::string_view Encoding::WebName() const noexcept {
    switch (kind_) {
    case Kind::UTF8:        return "utf-8";
    case Kind::UTF16LE:     return "utf-16";
    case Kind::UTF16BE:     return "utf-16BE";
    case Kind::LATIN1:      return "iso-8859-1";
    case Kind::WINDOWS1252: return "windows-1252";
    }
    return "";
}

int Encoding::CodePage() const noexcept {
    switch (kind_) {
    case Kind::UTF8:        return 65001;
    case Kind::UTF16LE:     return 1200;
    case Kind::UTF16BE:     return 1201;
    case Kind::LATIN1:      return 28591;
    case Kind::WINDOWS1252: return 1252;
    }
    return 0;
}

std::span<const std::byte> Encoding::Preamble() const noexcept {
    static constexpr std::array<std::byte, 3> kUtf8 = {
        std::byte{0xEFu}, std::byte{0xBBu}, std::byte{0xBFu}};
    static constexpr std::array<std::byte, 2> kUtf16Le = {
        std::byte{0xFFu}, std::byte{0xFEu}};
    static constexpr std::array<std::byte, 2> kUtf16Be = {
        std::byte{0xFEu}, std::byte{0xFFu}};
    switch (kind_) {
    case Kind::UTF8:
        return {kUtf8.data(), kUtf8.size()};
    case Kind::UTF16LE:
        return {kUtf16Le.data(), kUtf16Le.size()};
    case Kind::UTF16BE:
        return {kUtf16Be.data(), kUtf16Be.size()};
    case Kind::LATIN1:
    case Kind::WINDOWS1252:
        return {};
    }
    return {};
}

}  // namespace foundation::encoding
