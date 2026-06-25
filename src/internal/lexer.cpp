#include "lexer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

// PDF 32000-1:2008 §7.2.3 — whitespace characters
constexpr bool IsWhitespace(std::byte b) noexcept {
    switch (static_cast<unsigned char>(b)) {
        case 0x00: // NULL
        case 0x09: // TAB
        case 0x0A: // LF
        case 0x0C: // FF
        case 0x0D: // CR
        case 0x20: // SPACE
            return true;
        default:
            return false;
    }
}

// PDF 32000-1:2008 §7.2.3 — delimiter characters
constexpr bool IsDelimiter(std::byte b) noexcept {
    switch (static_cast<unsigned char>(b)) {
        case '(':
        case ')':
        case '<':
        case '>':
        case '[':
        case ']':
        case '{':
        case '}':
        case '/':
        case '%':
            return true;
        default:
            return false;
    }
}

// PDF 32000-1:2008 §7.2.3 — regular characters (neither whitespace nor delimiter)
constexpr bool IsRegular(std::byte b) noexcept {
    return !IsWhitespace(b) && !IsDelimiter(b);
}

// PDF 32000-1:2008 §7.2.4 — comment starts with '%' and runs to end of line
bool IsEol(std::byte b) noexcept {
    return b == std::byte{0x0A} || b == std::byte{0x0D};
}

// Helper: scan a line comment starting at pos (which points to '%')
std::size_t ScanComment(std::span<const std::byte> input, std::size_t pos) {
    // PDF 32000-1:2008 §7.2.4: comment is '%' followed by any sequence of
    // characters up to but not including the end-of-line marker.
    ++pos; // skip '%'
    while (pos < input.size() && !IsEol(input[pos])) {
        ++pos;
    }
    // Include the EOL if present
    if (pos < input.size() && IsEol(input[pos])) {
        // Preserve CR+LF as two tokens if present
        if (input[pos] == std::byte{0x0D} && pos + 1 < input.size() &&
            input[pos + 1] == std::byte{0x0A}) {
            pos += 2;
        } else {
            ++pos;
        }
    }
    return pos;
}

// Helper: scan a name starting at pos (which points to '/')
std::size_t ScanName(std::span<const std::byte> input, std::size_t pos) {
    // PDF 32000-1:2008 §7.3.5: Name objects are sequences of name characters.
    // Name characters are any non-whitespace, non-delimiter characters,
    // plus '#' followed by two hex digits.
    ++pos; // skip '/'
    while (pos < input.size()) {
        std::byte b = input[pos];
        if (IsWhitespace(b) || IsDelimiter(b)) {
            break;
        }
        if (b == std::byte{'#'} && pos + 3 < input.size()) {
            // Check for #XX hex escape
            unsigned char c1 = static_cast<unsigned char>(input[pos + 1]);
            unsigned char c2 = static_cast<unsigned char>(input[pos + 2]);
            if ((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F') || (c1 >= 'a' && c1 <= 'f')) {
                if ((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F') || (c2 >= 'a' && c2 <= 'f')) {
                    pos += 3;
                    continue;
                }
            }
        }
        ++pos;
    }
    return pos;
}

// Helper: scan a number (integer or real)
std::size_t ScanNumber(std::span<const std::byte> input, std::size_t pos) {
    // PDF 32000-1:2008 §7.3.3: numbers may be integers or real.
    // Integers: optional sign followed by one or more digits.
    // Reals: optional sign, digits, '.', digits, optional exponent.
    if (pos >= input.size()) return pos;

    std::byte b = input[pos];
    if (b == std::byte{'+'} || b == std::byte{'-'}) {
        ++pos;
    }

    // Digits before decimal point
    while (pos < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[pos]);
        if (c < '0' || c > '9') break;
        ++pos;
    }

    // Optional fractional part
    if (pos < input.size() && input[pos] == std::byte{'.'}) {
        ++pos;
        // Digits after decimal point
        while (pos < input.size()) {
            unsigned char c = static_cast<unsigned char>(input[pos]);
            if (c < '0' || c > '9') break;
            ++pos;
        }
    }

    // Optional exponent
    if (pos < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[pos]);
        if (c == 'e' || c == 'E') {
            ++pos;
            if (pos < input.size()) {
                c = static_cast<unsigned char>(input[pos]);
                if (c == '+' || c == '-') {
                    ++pos;
                }
                // Digits in exponent
                while (pos < input.size()) {
                    c = static_cast<unsigned char>(input[pos]);
                    if (c < '0' || c > '9') break;
                    ++pos;
                }
            }
        }
    }

    return pos;
}

// Helper: scan a literal string starting at pos (which points to '(')
std::size_t ScanLiteralString(std::span<const std::byte> input, std::size_t pos) {
    // PDF 32000-1:2008 §7.3.4.1: literal strings are enclosed in parentheses.
    // Backslash escapes are supported; parentheses must be balanced.
    ++pos; // skip '('
    int depth = 1;
    while (pos < input.size() && depth > 0) {
        std::byte b = input[pos];
        if (b == std::byte{'\\'}) {
            // Escape sequence: skip next char (even if not a valid escape)
            pos += 2;
            continue;
        }
        if (b == std::byte{'('}) {
            ++depth;
        } else if (b == std::byte{')'}) {
            --depth;
        }
        // Advance past every non-escape byte, including a nested '(' or
        // ')'. Balanced parentheses inside a literal string are legal
        // and need no escaping (§7.3.4.1); not advancing here re-read
        // the same paren forever. The closing ')' that drops depth to 0
        // is consumed by this increment, so the token span includes it.
        ++pos;
    }
    return pos;
}

// Helper: scan a hex string starting at pos (which points to '<')
std::size_t ScanHexString(std::span<const std::byte> input, std::size_t pos) {
    // PDF 32000-1:2008 §7.3.4.3: hex strings are enclosed in angle brackets.
    // Hex digits may be separated by whitespace.
    ++pos; // skip '<'
    while (pos < input.size()) {
        std::byte b = input[pos];
        if (IsWhitespace(b)) {
            ++pos;
            continue;
        }
        if (b == std::byte{'>'}) {
            ++pos;
            break;
        }
        // Consume hex digit
        unsigned char c = static_cast<unsigned char>(b);
        bool is_hex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
        if (is_hex) {
            ++pos;
            continue;
        }
        // Invalid char in hex string — stop at this char
        break;
    }
    return pos;
}

// Helper: scan whitespace (including EOLs) and return end position
std::size_t ScanWhitespace(std::span<const std::byte> input, std::size_t pos) {
    while (pos < input.size() && IsWhitespace(input[pos])) {
        ++pos;
    }
    return pos;
}

// Helper: determine if a keyword token is a known keyword
bool IsKnownKeyword(std::span<const std::byte> bytes) {
    // PDF 32000-1:2008 §7.3.2: boolean keywords
    static const std::string true_str = "true";
    static const std::string false_str = "false";
    // PDF 32000-1:2008 §7.3.1: null keyword
    static const std::string null_str = "null";
    // PDF 32000-1:2008 §7.3.10: stream/endstream keywords
    static const std::string stream_str = "stream";
    static const std::string endstream_str = "endstream";
    // PDF 32000-1:2008 §7.3.10: obj/endobj keywords
    static const std::string obj_str = "obj";
    static const std::string endobj_str = "endobj";
    // PDF 32000-1:2008 §7.3.10: xref, trailer, startxref keywords
    static const std::string xref_str = "xref";
    static const std::string trailer_str = "trailer";
    static const std::string startxref_str = "startxref";

    if (bytes.size() == 0) return false;

    std::string_view sv(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return sv == true_str || sv == false_str || sv == null_str ||
           sv == stream_str || sv == endstream_str ||
           sv == obj_str || sv == endobj_str ||
           sv == xref_str || sv == trailer_str || sv == startxref_str;
}

} // namespace

namespace foundation {

Lexer::Lexer(std::span<const std::byte> input)
    : input_(input), pos_(0), at_end_(false) {}

Token Lexer::Next() {
    // PDF 32000-1:2008 §7.2: lexical conventions
    if (at_end_) {
        return {TokenKind::Eof, {}, input_.size()};
    }

    if (pos_ >= input_.size()) {
        at_end_ = true;
        return {TokenKind::Eof, {}, input_.size()};
    }

    std::size_t start = pos_;
    std::byte b = input_[pos_];
    TokenKind kind = TokenKind::Whitespace;

    // Dispatch based on first byte
    if (IsWhitespace(b)) {
        // Scan all whitespace (including EOLs) as one token
        pos_ = ScanWhitespace(input_, pos_);
        kind = TokenKind::Whitespace;
    } else if (b == std::byte{'%'}) {
        // Comment
        pos_ = ScanComment(input_, pos_);
        kind = TokenKind::Comment;
    } else if (b == std::byte{'('}) {
        // Literal string
        pos_ = ScanLiteralString(input_, pos_);
        kind = TokenKind::StringLiteral;
    } else if (b == std::byte{'<'}) {
        // Hex string or dict open
        if (pos_ + 1 < input_.size() && input_[pos_ + 1] == std::byte{'<'}) {
            // Dict open
            pos_ += 2;
            kind = TokenKind::DictOpen;
        } else {
            // Hex string
            pos_ = ScanHexString(input_, pos_);
            kind = TokenKind::StringHex;
        }
    } else if (b == std::byte{'>'}) {
        // Dict close or array close
        if (pos_ + 1 < input_.size() && input_[pos_ + 1] == std::byte{'>'}) {
            pos_ += 2;
            kind = TokenKind::DictClose;
        } else {
            pos_ += 1;
            kind = TokenKind::ArrayClose;
        }
    } else if (b == std::byte{'['}) {
        pos_ += 1;
        kind = TokenKind::ArrayOpen;
    } else if (b == std::byte{']'}) {
        pos_ += 1;
        kind = TokenKind::ArrayClose;
    } else if (b == std::byte{'/'}) {
        // Name
        pos_ = ScanName(input_, pos_);
        kind = TokenKind::Name;
    } else if (b == std::byte{'+'} || b == std::byte{'-'} ||
               b == std::byte{'.'} ||
               (b >= std::byte{'0'} && b <= std::byte{'9'})) {
        // Number (integer or real). PDF 32000-1:2008 §7.3.3 admits
        // a real with no integer part, written with a leading
        // decimal point (e.g. `.5`, `.000009`); dispatch on '.' so
        // it tokenises as a Number rather than a Keyword.
        pos_ = ScanNumber(input_, pos_);
        kind = TokenKind::Number;
    } else if (IsDelimiter(b)) {
        // Delimiter not handled above: treat as keyword
        // PDF 32000-1:2008 §7.2.3: delimiters are: ( ) < > [ ] { } / %
        // We've handled most above; remaining are: ( ) [ ] { }
        // But ( and ) are handled as strings, [ and ] as arrays, { } as dicts.
        // So this branch should not be reached for standard delimiters.
        // However, per the byte_roundtrip rules, we must handle *any* byte.
        // Treat as a single-byte keyword.
        pos_ += 1;
        kind = TokenKind::Keyword;
    } else {
        // Regular character: could be a keyword or part of one.
        // Scan until whitespace/delimiter to form a keyword candidate.
        ++pos_;
        while (pos_ < input_.size()) {
            std::byte next = input_[pos_];
            if (IsWhitespace(next) || IsDelimiter(next)) {
                break;
            }
            ++pos_;
        }
        kind = TokenKind::Keyword;
    }

    // Construct token with zero-copy view
    std::size_t end = pos_;
    std::span<const std::byte> view = input_.subspan(start, end - start);

    // If we didn't advance, force one-byte token to avoid infinite loop
    if (end == start) {
        end = start + 1;
        view = input_.subspan(start, 1);
        // Adjust kind: use Whitespace for unknown bytes (PDF spec §7.2.3)
        kind = TokenKind::Whitespace;
    }

    Token t{kind, view, start};
    if (t.kind == TokenKind::Eof) {
        at_end_ = true;
    }
    return t;
}

bool Lexer::AtEnd() const {
    return at_end_;
}

std::vector<Token> Lexer::Lex() {
    std::vector<Token> tokens;
    // PDF 32000-1:2008 §7.2: Lexical conventions — tokens are scanned until EOF
    while (!at_end_) {
        Token t = Next();
        if (t.kind == TokenKind::Eof) {
            break;
        }
        tokens.push_back(t);
    }
    return tokens;
}

}
