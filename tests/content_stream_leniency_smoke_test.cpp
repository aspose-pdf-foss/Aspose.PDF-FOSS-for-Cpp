// =============================================================================
// content_stream_leniency_smoke_test — a malformed content stream must parse
// to completion (dropping the bad bytes) instead of throwing and failing the
// whole page. Field reproducers: a stray '>' in operand position (31458), a
// keyword in an array/dict operand (34328, 34162-1-crop*), a trailing operand
// run with no terminating operator (34832, 24558-2, 33150), an unterminated
// array, and a malformed indirect reference (34749-1).
//
// Content-stream parsing is non-semantic (it already accepts unknown
// operators); leniency on malformed tokens is consistent with that — validation
// is a separate concern.
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

foundation::content_stream::Stream Parse(std::string_view s) {
    const auto in = Bytes(s);
    return foundation::content_stream::Parse(
        std::span<const std::byte>(in.data(), in.size()));
}

bool HasOp(const foundation::content_stream::Stream& st, std::string_view op) {
    for (const auto& o : st.operations) if (o.op == op) return true;
    return false;
}

}  // namespace

// A stray '>' (and '>>' / ']') in operand position is skipped; surrounding
// operators survive.
TEST(ContentStreamLeniency, StrayDelimiterInOperandPosition) {
    const auto st = Parse("q 1 0 0 1 0 0 cm\n> ]\n0 0 1 rg\n0 0 10 10 re f\nQ\n");
    EXPECT_TRUE(HasOp(st, "q"));
    EXPECT_TRUE(HasOp(st, "cm"));
    EXPECT_TRUE(HasOp(st, "rg"));
    EXPECT_TRUE(HasOp(st, "f"));
    EXPECT_TRUE(HasOp(st, "Q"));
}

// A keyword inside an array operand is dropped, not fatal.
TEST(ContentStreamLeniency, KeywordInsideArray) {
    const auto st = Parse("[ (a) -20 Tf (b) ] TJ\nET\n");
    EXPECT_TRUE(HasOp(st, "TJ"));
    EXPECT_TRUE(HasOp(st, "ET"));
}

// A trailing operand run with no terminating operator is discarded.
TEST(ContentStreamLeniency, TrailingOperandStackDropped) {
    const auto st = Parse("q\nQ\n1 2 3 4 5\n");
    EXPECT_TRUE(HasOp(st, "q"));
    EXPECT_TRUE(HasOp(st, "Q"));
}

// An unterminated array does not hang or throw.
TEST(ContentStreamLeniency, UnterminatedArray) {
    const auto st = Parse("BT\n[ 1 2 3\n");
    EXPECT_TRUE(HasOp(st, "BT"));  // reaching here means no throw / hang
}

// A malformed indirect-reference-like sequence is treated as bare operands.
TEST(ContentStreamLeniency, MalformedIndirectRef) {
    const auto st = Parse("q 1 0 0 1 0 0 cm\n5 x R\n0 0 1 rg\nf\nQ\n");
    EXPECT_TRUE(HasOp(st, "cm"));
    EXPECT_TRUE(HasOp(st, "Q"));
}
