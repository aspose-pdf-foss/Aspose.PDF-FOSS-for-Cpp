// =============================================================================
// lexer_nested_paren_smoke_test — regression coverage for PDF
// 32000-1:2008 §7.3.4.1 literal strings containing balanced nested
// parentheses (e.g. `(a(b)c)`, `(outer (inner) text)`).
//
// A balanced inner paren pair is legal string content and needs no
// escaping. The old ScanLiteralString adjusted the paren-depth counter
// on a nested '(' / ')' but did not advance the read cursor, so it
// re-read the same paren forever — an infinite loop that hung the
// renderer on any content stream carrying such a Tj/TJ operand
// (field reproducers 29501, 29753, 31458, 31840, 33319, 33343).
//
// These assertions must simply RETURN — if the lexer regresses they
// hang rather than fail, which the test runner reports as a timeout.
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

}  // namespace

// `(a(b)c)` lexes to exactly one StringLiteral spanning every byte,
// including the closing paren — and terminates.
TEST(LexerNestedParenSmoke, SingleNestedPairIsOneString) {
    const auto in = Bytes("(a(b)c)");
    foundation::Lexer lex{std::span<const std::byte>(in.data(), in.size())};
    const auto toks = lex.Lex();
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, foundation::TokenKind::StringLiteral);
    EXPECT_EQ(View(toks[0]), "(a(b)c)");
}

// Deeper nesting plus a trailing token: the string ends at the matching
// outer ')', the following name lexes separately.
TEST(LexerNestedParenSmoke, DeepNestingClosesAtOuterParen) {
    const auto in = Bytes("(a((b))c) /Next");
    foundation::Lexer lex{std::span<const std::byte>(in.data(), in.size())};
    const auto toks = lex.Lex();
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, foundation::TokenKind::StringLiteral);
    EXPECT_EQ(View(toks[0]), "(a((b))c)");
}

// An escaped paren does not change depth (so the string still closes at
// the real matching paren) and the scanner still terminates.
TEST(LexerNestedParenSmoke, EscapedParenDoesNotUnbalance) {
    const auto in = Bytes("(a\\)b(c)d)");
    foundation::Lexer lex{std::span<const std::byte>(in.data(), in.size())};
    const auto toks = lex.Lex();
    ASSERT_GE(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, foundation::TokenKind::StringLiteral);
    EXPECT_EQ(View(toks[0]), "(a\\)b(c)d)");
}

// The field symptom: a content stream whose Tj operand has nested parens
// parses to completion instead of hanging.
TEST(LexerNestedParenSmoke, ContentStreamWithNestedParenTjParses) {
    const auto in = Bytes("BT /F1 12 Tf (a(b)c) Tj ET");
    const auto cs = foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
    EXPECT_GE(cs.operations.size(), 1u);  // reaching here means no hang
}
