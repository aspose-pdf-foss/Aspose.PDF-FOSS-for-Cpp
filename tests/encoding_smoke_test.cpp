// Hand-authored gtest smoke test for foundation::encoding.
// Covers basic API surface: factories, GetBytes/GetString roundtrip
// for representative inputs across the five v1 charsets, BCL-shape
// properties (WebName / CodePage / Preamble), GetEncoding aliases,
// and replacement-char policy on unmappable codepoints.

#include "encoding.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

using foundation::encoding::Encoding;

namespace {

std::span<const std::byte> SpanOfString(const std::string& s) {
    return std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(s.data()), s.size());
}

}  // namespace

TEST(EncodingSmoke, FactoriesReturnSingletons) {
    EXPECT_EQ(&Encoding::UTF8(), &Encoding::UTF8());
    EXPECT_EQ(&Encoding::Unicode(), &Encoding::Unicode());
    EXPECT_NE(&Encoding::UTF8(), &Encoding::Unicode());
    EXPECT_NE(&Encoding::Latin1(), &Encoding::Windows1252());
}

TEST(EncodingSmoke, WebNamesMatchBcl) {
    EXPECT_EQ(Encoding::UTF8().WebName(), "utf-8");
    EXPECT_EQ(Encoding::Unicode().WebName(), "utf-16");
    EXPECT_EQ(Encoding::BigEndianUnicode().WebName(), "utf-16BE");
    EXPECT_EQ(Encoding::Latin1().WebName(), "iso-8859-1");
    EXPECT_EQ(Encoding::Windows1252().WebName(), "windows-1252");
}

TEST(EncodingSmoke, CodePagesMatchBcl) {
    EXPECT_EQ(Encoding::UTF8().CodePage(), 65001);
    EXPECT_EQ(Encoding::Unicode().CodePage(), 1200);
    EXPECT_EQ(Encoding::BigEndianUnicode().CodePage(), 1201);
    EXPECT_EQ(Encoding::Latin1().CodePage(), 28591);
    EXPECT_EQ(Encoding::Windows1252().CodePage(), 1252);
}

TEST(EncodingSmoke, PreambleBytes) {
    auto utf8 = Encoding::UTF8().Preamble();
    ASSERT_EQ(utf8.size(), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(utf8[0]), 0xEFu);
    EXPECT_EQ(static_cast<std::uint8_t>(utf8[1]), 0xBBu);
    EXPECT_EQ(static_cast<std::uint8_t>(utf8[2]), 0xBFu);

    auto le = Encoding::Unicode().Preamble();
    ASSERT_EQ(le.size(), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(le[0]), 0xFFu);
    EXPECT_EQ(static_cast<std::uint8_t>(le[1]), 0xFEu);

    auto be = Encoding::BigEndianUnicode().Preamble();
    ASSERT_EQ(be.size(), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(be[0]), 0xFEu);
    EXPECT_EQ(static_cast<std::uint8_t>(be[1]), 0xFFu);

    EXPECT_EQ(Encoding::Latin1().Preamble().size(), 0u);
    EXPECT_EQ(Encoding::Windows1252().Preamble().size(), 0u);
}

TEST(EncodingSmoke, AsciiRoundtripAcrossAllCharsets) {
    const std::string input = "Hello";
    const Encoding* charsets[] = {
        &Encoding::UTF8(),
        &Encoding::Unicode(),
        &Encoding::BigEndianUnicode(),
        &Encoding::Latin1(),
        &Encoding::Windows1252(),
    };
    for (const auto* enc : charsets) {
        const auto bytes = enc->GetBytes(input);
        const auto round =
            enc->GetString(std::span<const std::byte>(bytes));
        EXPECT_EQ(round, input)
            << "roundtrip failed for " << enc->WebName();
    }
}

TEST(EncodingSmoke, Latin1OneByteRoundtrip) {
    const std::string input = "café";  // U+00E9
    const auto bytes = Encoding::Latin1().GetBytes(input);
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[0]), 'c');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[1]), 'a');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[2]), 'f');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0xE9u);
    EXPECT_EQ(Encoding::Latin1().GetString(
        std::span<const std::byte>(bytes)), input);
}

TEST(EncodingSmoke, Windows1252EncodesEuroSign) {
    const std::string input = "€";  // U+20AC
    const auto bytes = Encoding::Windows1252().GetBytes(input);
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[0]), 0x80u);
    EXPECT_EQ(Encoding::Windows1252().GetString(
        std::span<const std::byte>(bytes)), input);
}

TEST(EncodingSmoke, Latin1ReplacesUnmappableWithQuestionMark) {
    const std::string input = "中";  // not in latin-1
    const auto bytes = Encoding::Latin1().GetBytes(input);
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[0]), 0x3Fu);
}

TEST(EncodingSmoke, Windows1252UndefinedByteDecodesToReplacement) {
    // 0x81 is undefined in win-1252 — decode produces U+FFFD.
    const std::byte b[] = {std::byte{0x81u}};
    const std::string out = Encoding::Windows1252().GetString(
        std::span<const std::byte>(b, 1));
    // U+FFFD encoded as UTF-8 is 0xEF 0xBF 0xBD.
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(out[0]), 0xEFu);
    EXPECT_EQ(static_cast<std::uint8_t>(out[1]), 0xBFu);
    EXPECT_EQ(static_cast<std::uint8_t>(out[2]), 0xBDu);
}

TEST(EncodingSmoke, Utf16LeEncodesAscii) {
    const std::string input = "Hi";
    const auto bytes = Encoding::Unicode().GetBytes(input);
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[0]), 'H');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[1]), 0x00u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[2]), 'i');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 0x00u);
}

TEST(EncodingSmoke, Utf16BeEncodesAscii) {
    const std::string input = "Hi";
    const auto bytes = Encoding::BigEndianUnicode().GetBytes(input);
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[0]), 0x00u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[1]), 'H');
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[2]), 0x00u);
    EXPECT_EQ(static_cast<std::uint8_t>(bytes[3]), 'i');
}

TEST(EncodingSmoke, Utf16HandlesSurrogatePair) {
    // U+1F600 = 😀 — encodes as surrogate pair in utf-16.
    const std::string input = "\xF0\x9F\x98\x80";  // utf-8 of U+1F600
    const auto le = Encoding::Unicode().GetBytes(input);
    ASSERT_EQ(le.size(), 4u);
    // Surrogate pair: D83D DE00 — LE: 3D D8 00 DE
    EXPECT_EQ(static_cast<std::uint8_t>(le[0]), 0x3Du);
    EXPECT_EQ(static_cast<std::uint8_t>(le[1]), 0xD8u);
    EXPECT_EQ(static_cast<std::uint8_t>(le[2]), 0x00u);
    EXPECT_EQ(static_cast<std::uint8_t>(le[3]), 0xDEu);
    // Roundtrip
    EXPECT_EQ(Encoding::Unicode().GetString(
        std::span<const std::byte>(le)), input);
}

TEST(EncodingSmoke, GetEncodingResolvesAliases) {
    EXPECT_EQ(&Encoding::GetEncoding("utf-8"), &Encoding::UTF8());
    EXPECT_EQ(&Encoding::GetEncoding("UTF-8"), &Encoding::UTF8());
    EXPECT_EQ(&Encoding::GetEncoding("utf8"), &Encoding::UTF8());
    EXPECT_EQ(&Encoding::GetEncoding("unicode"),
              &Encoding::Unicode());
    EXPECT_EQ(&Encoding::GetEncoding("UTF-16BE"),
              &Encoding::BigEndianUnicode());
    EXPECT_EQ(&Encoding::GetEncoding("iso-8859-1"),
              &Encoding::Latin1());
    EXPECT_EQ(&Encoding::GetEncoding("Latin1"), &Encoding::Latin1());
    EXPECT_EQ(&Encoding::GetEncoding("Windows-1252"),
              &Encoding::Windows1252());
    EXPECT_EQ(&Encoding::GetEncoding("cp1252"),
              &Encoding::Windows1252());
}

TEST(EncodingSmoke, GetEncodingThrowsOnUnknown) {
    EXPECT_THROW(Encoding::GetEncoding("unsupported-charset"),
                 std::invalid_argument);
}

TEST(EncodingSmoke, EmptyInputProducesEmptyBytes) {
    const std::string input;
    EXPECT_EQ(Encoding::UTF8().GetBytes(input).size(), 0u);
    EXPECT_EQ(Encoding::Unicode().GetBytes(input).size(), 0u);
    EXPECT_EQ(Encoding::Latin1().GetBytes(input).size(), 0u);
}
