// =============================================================================
// lexer_leading_dot_smoke_test — regression coverage for PDF
// 32000-1:2008 §7.3.3 reals written with a leading decimal point
// and no integer part (e.g. `.5`, `.000009`).
//
// The lexer must classify a token starting with '.' as a Number,
// not a Keyword. The downstream symptom of the old misclassification
// was a content stream like `.5 0 0 .5 0 0 cm` losing all of its
// operands (the bare fractional was treated as an operator that
// flushed the operand stack), so `cm` ran with arity 0 and the image
// it positioned rendered at a degenerate unit scale (blank page).
// =============================================================================

#include "content_stream.hpp"
#include "lexer.hpp"

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

std::string_view View(const foundation::Token& t) {
    return std::string_view(
        reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
}

// `.5` lexes to exactly one Number token spanning both bytes.
TEST(LexerLeadingDotSmoke, BareFractionalIsNumber) {
    const auto in = Bytes(".5");
    foundation::Lexer lex{std::span<const std::byte>(in.data(), in.size())};
    const auto toks = lex.Lex();
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, foundation::TokenKind::Number);
    EXPECT_EQ(View(toks[0]), ".5");
}

// `.000009` (the shape seen in real cm matrices) is a single Number.
TEST(LexerLeadingDotSmoke, LongBareFractionalIsNumber) {
    const auto in = Bytes(".000009");
    foundation::Lexer lex{std::span<const std::byte>(in.data(), in.size())};
    const auto toks = lex.Lex();
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, foundation::TokenKind::Number);
    EXPECT_EQ(View(toks[0]), ".000009");
}

// A `cm` with leading-dot reals keeps all six operands through the
// content-stream parser.
TEST(LexerLeadingDotSmoke, CmKeepsSixOperands) {
    const auto in = Bytes("q\n.5 0 0 .5 0 0 cm\n/Im Do\nQ\n");
    const auto cs = foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
    const foundation::content_stream::Operation* cm = nullptr;
    for (const auto& op : cs.operations) {
        if (op.op == "cm") cm = &op;
    }
    ASSERT_NE(cm, nullptr);
    EXPECT_EQ(cm->operands.size(), 6u);
}

}  // namespace
