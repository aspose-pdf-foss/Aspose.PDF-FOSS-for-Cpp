// =============================================================================
// content_stream_inline_image_smoke_test — regression coverage for PDF
// 32000-1:2008 §8.9.7 inline images (`BI <params> ID <raw bytes> EI`).
//
// The raw image samples after `ID` are not lexable PDF tokens; the lexer
// turns them into garbage tokens. Without inline-image handling the parser
// fed those tokens to the value grammar and threw "unexpected keyword /
// token in value position" whenever the binary happened to contain a '['
// or '<<' (field reproducers 29501, 33343). Parse now skips from `ID` to
// the `EI` keyword token so the rest of the stream parses.
// =============================================================================

#include "content_stream.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> Bytes(std::string_view s) {
    std::vector<std::byte> out;
    out.reserve(s.size());
    for (char c : s) out.push_back(static_cast<std::byte>(c));
    return out;
}

bool HasOp(const foundation::content_stream::Stream& s,
           std::string_view op) {
    for (const auto& o : s.operations) {
        if (o.op == op) return true;
    }
    return false;
}

}  // namespace

// An inline image whose binary contains '<', '[' and keyword-like bytes
// (which previously opened containers and threw) is skipped, and the
// operators before and after it survive.
TEST(ContentStreamInlineImageSmoke, BinaryWithDelimitersIsSkipped) {
    // Binary payload deliberately includes '<' '[' '(' and letters.
    const auto in = Bytes(
        "q 1 0 0 1 0 0 cm\n"
        "BI /W 2 /H 2 /BPC 8 /CS /RGB ID <[(garbage Tf BT >>]\x01\x02 EI\n"
        "Q\n"
        "BT /F1 12 Tf (after) Tj ET\n");
    const auto cs = foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
    // Reaching here means no throw. The trailing text operators parsed.
    EXPECT_TRUE(HasOp(cs, "q"));
    EXPECT_TRUE(HasOp(cs, "Q"));
    EXPECT_TRUE(HasOp(cs, "Tf"));
    EXPECT_TRUE(HasOp(cs, "Tj"));
    EXPECT_TRUE(HasOp(cs, "ET"));
}

// Two consecutive inline images both skip cleanly (a single mis-synced
// skip would derail the second).
TEST(ContentStreamInlineImageSmoke, TwoInlineImagesBothSkip) {
    const auto in = Bytes(
        "BI /W 1 /H 1 /BPC 8 /CS /G ID \xAA\xBB EI\n"
        "q\n"
        "BI /W 1 /H 1 /BPC 8 /CS /G ID \xCC\xDD EI\n"
        "Q\n");
    const auto cs = foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
    EXPECT_TRUE(HasOp(cs, "q"));
    EXPECT_TRUE(HasOp(cs, "Q"));
}

// An inline image with a /DP dict param (the common filtered-image shape)
// parses: the dict before ID is ordinary, only the post-ID bytes skip.
TEST(ContentStreamInlineImageSmoke, FilteredInlineImageWithDictParam) {
    const auto in = Bytes(
        "BI /CS /RGB /W 4 /H 1 /BPC 8 /F /Fl "
        "/DP << /Predictor 15 /Columns 4 /Colors 3 >> ID "
        "\x78\x9c\x01\x02\x03 EI\n"
        "re\n");
    const auto cs = foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
    EXPECT_TRUE(HasOp(cs, "re"));  // operator after EI survived
}
