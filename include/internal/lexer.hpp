#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation {

enum class TokenKind {
    Name,
    Number,
    StringLiteral,
    StringHex,
    ArrayOpen,
    ArrayClose,
    DictOpen,
    DictClose,
    Keyword,
    Comment,
    Whitespace,
    Eol,
    Eof,
};

// Token = (kind, byte view into input, absolute offset).
// bytes is a non-owning view — the underlying buffer belongs to
// the caller of Lexer::Lex. offset is the byte offset in the
// original input where this token starts.
struct Token {
    TokenKind kind;
    std::span<const std::byte> bytes;
    std::size_t offset;
};

// Trivia-preserving PDF lexer.
// Lex consumes the entire input and returns every token — no
// whitespace skipping, no comment elision. The concatenation of
// token.bytes in order equals the input byte-for-byte.
class Lexer {
 public:
    explicit Lexer(std::span<const std::byte> input);

    // Returns one token per call; final call returns a
    // TokenKind::Eof token at input.size() offset.
    Token Next();

    // True once Next() has returned the Eof token.
    bool AtEnd() const;

    // Convenience: consume the whole input in one call.
    std::vector<Token> Lex();

 private:
    std::span<const std::byte> input_;
    std::size_t pos_;
    bool at_end_;
};

}
