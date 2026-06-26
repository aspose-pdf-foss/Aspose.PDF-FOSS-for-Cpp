#include "content_stream.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "lexer.hpp"
#include "objects.hpp"

namespace foundation::content_stream {

namespace {

// Expected helper list — fill bodies in this order.
// Keep each helper ≤ 40 lines. If a body exceeds that, split it further.

// 1. Trivia-skipping cursor over the lexer's token vector.
//    Peek / Next / AtEnd skip Whitespace + Comment transparently.
//    Returns a synthetic Eof when pos_ reaches tokens_.size().
class TokenCursor {
 public:
    explicit TokenCursor(std::vector<Token> tokens)
        : tokens_(std::move(tokens)), pos_(0) {}

    // Peek and Next skip Whitespace + Comment tokens
    // transparently so helpers never see trivia. The lexer
    // is trivia-preserving; content_stream treats those
    // kinds as noise between operands and operators.
    Token Peek() {
        SkipTrivia();
        if (pos_ >= tokens_.size()) {
            return {TokenKind::Eof, {}, tokens_.size()};
        }
        return tokens_[pos_];
    }

    Token Next() {
        SkipTrivia();
        if (pos_ >= tokens_.size()) {
            return {TokenKind::Eof, {}, tokens_.size()};
        }
        return tokens_[pos_++];
    }

    bool AtEnd() {
        SkipTrivia();
        return pos_ >= tokens_.size();
    }

 private:
    void SkipTrivia() {
        while (pos_ < tokens_.size()
               && (tokens_[pos_].kind == TokenKind::Whitespace
                   || tokens_[pos_].kind == TokenKind::Comment)) {
            ++pos_;
        }
    }

    std::vector<Token> tokens_;
    std::size_t pos_;
};

// 2. One decoder per TokenKind that produces a value. Each
//    takes the token's bytes view (no cursor needed) and
//    returns one Value.
objects::Value DecodeNumber(std::span<const std::byte> bytes);
objects::Value DecodeName(std::span<const std::byte> bytes);
objects::Value DecodeLiteralString(std::span<const std::byte> bytes);
objects::Value DecodeHexString(std::span<const std::byte> bytes);

// 3. Container helpers — recursive value grammar. Each takes a
//    cursor positioned just past the opener and consumes
//    through the closer.
objects::Array ParseArray(TokenCursor& cursor);
objects::Dict ParseDict(TokenCursor& cursor);

// 4. Value dispatcher — chooses the right decoder/container
//    helper based on the next token kind. Also handles the
//    keyword literals null / true / false when they appear
//    in value position.
objects::Value ParseValue(TokenCursor& cursor);

// 5. Per-value-kind canonicaliser. One function per variant
//    alternative. Avoid writing one large `if`/`else if` chain;
//    use std::visit or a switch on the discriminator.
std::string CanonicalName(const std::string& name);
std::string CanonicalString(const objects::String& s);
std::string CanonicalReal(double d);
std::string CanonicalArray(const objects::Array& arr);
std::string CanonicalDict(const objects::Dict& dict);
std::string CanonicalValue(const objects::Value& v);

// 6. Helper: skip trivia tokens (Whitespace, Comment)
//    and return the next non-trivia token.
Token NextSignificant(TokenCursor& cursor);

// 7. Helper: parse indirect reference (N M R) when present.
//    Returns std::nullopt if not an indirect reference.
std::optional<objects::Ref> TryParseIndirectRef(TokenCursor& cursor);

// 8. Helper: decode #XX escapes in a name body (without leading '/').
std::string DecodeNameBody(std::span<const std::byte> bytes);

// 2. Decoders

objects::Value DecodeNumber(std::span<const std::byte> bytes) {
    std::string_view sv(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    bool has_dot = sv.find('.') != std::string_view::npos;
    if (!has_dot) {
        std::int64_t i = 0;
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), i);
        if (ec == std::errc() && ptr == sv.data() + sv.size()) {
            return objects::Value{i};
        }
    }
    double d = 0.0;
    char* end = nullptr;
    d = std::strtod(sv.data(), &end);
    if (end == sv.data() + sv.size()) {
        return objects::Value{d};
    }
    // Fallback: treat as real even if parsing failed (malformed)
    return objects::Value{0.0};
}

objects::Value DecodeName(std::span<const std::byte> bytes) {
    if (bytes.size() == 0) return objects::Value{std::string{}};
    // Skip leading '/'
    if (bytes.size() > 0 && bytes[0] == std::byte{'/'}) {
        bytes = bytes.subspan(1);
    }
    return objects::Value{DecodeNameBody(bytes)};
}

std::string DecodeNameBody(std::span<const std::byte> bytes) {
    std::string result;
    result.reserve(bytes.size());
    for (std::size_t i = 0; i < bytes.size(); ) {
        if (bytes[i] == std::byte{'#'} && i + 2 < bytes.size()) {
            auto hex_to_byte = [](char c) -> std::uint8_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return 10 + c - 'A';
                if (c >= 'a' && c <= 'f') return 10 + c - 'a';
                return 0;
            };
            std::uint8_t hi = hex_to_byte(static_cast<char>(bytes[i + 1]));
            std::uint8_t lo = hex_to_byte(static_cast<char>(bytes[i + 2]));
            result.push_back(static_cast<char>(static_cast<std::uint8_t>((hi << 4) | lo)));
            i += 3;
        } else {
            result.push_back(static_cast<char>(static_cast<std::uint8_t>(bytes[i])));
            ++i;
        }
    }
    return result;
}

objects::Value DecodeLiteralString(std::span<const std::byte> bytes) {
    if (bytes.size() < 2) {
        return objects::Value{objects::String{{}, objects::StringKind::Literal}};
    }
    std::vector<std::byte> body;
    body.reserve(bytes.size() - 2);
    for (std::size_t i = 1; i + 1 < bytes.size(); ) {
        if (bytes[i] == std::byte{'\\'}) {
            ++i;
            if (i >= bytes.size()) break;
            switch (static_cast<char>(bytes[i])) {
                case 'n': body.push_back(std::byte{'\n'}); break;
                case 'r': body.push_back(std::byte{'\r'}); break;
                case 't': body.push_back(std::byte{'\t'}); break;
                case 'b': body.push_back(std::byte{'\b'}); break;
                case 'f': body.push_back(std::byte{'\f'}); break;
                case '(': body.push_back(std::byte{'('}); break;
                case ')': body.push_back(std::byte{')'}); break;
                case '\\': body.push_back(std::byte{'\\'}); break;
                case '\n':
                    // Line continuation — skip the EOL
                    ++i;
                    continue;
                case '\r':
                    if (i + 1 < bytes.size() && bytes[i + 1] == std::byte{'\n'}) {
                        ++i; // skip \r\n
                    }
                    ++i;
                    continue;
                default:
                    // Octal escape: up to 3 octal digits
                    std::uint8_t val = 0;
                    int count = 0;
                    while (count < 3 && i < bytes.size() &&
                           static_cast<char>(bytes[i]) >= '0' &&
                           static_cast<char>(bytes[i]) <= '7') {
                        val = (val << 3) | (static_cast<std::uint8_t>(bytes[i]) - '0');
                        ++i;
                        ++count;
                    }
                    body.push_back(std::byte{val});
                    continue;
            }
            ++i;
        } else {
            body.push_back(bytes[i]);
            ++i;
        }
    }
    return objects::Value{objects::String{std::move(body), objects::StringKind::Literal}};
}

objects::Value DecodeHexString(std::span<const std::byte> bytes) {
    if (bytes.size() < 2) {
        return objects::Value{objects::String{{}, objects::StringKind::Hex}};
    }
    std::vector<std::byte> body;
    body.reserve((bytes.size() - 2) / 2);
    bool high_nibble = true;
    std::uint8_t current = 0;
    for (std::size_t i = 1; i + 1 < bytes.size(); ++i) {
        char c = static_cast<char>(bytes[i]);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\0') {
            continue; // skip whitespace in hex string
        }
        auto hex_val = [](char ch) -> std::uint8_t {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
            if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
            return 0;
        };
        std::uint8_t v = hex_val(c);
        if (high_nibble) {
            current = static_cast<std::uint8_t>(v << 4);
            high_nibble = false;
        } else {
            current |= v;
            body.push_back(std::byte{current});
            high_nibble = true;
        }
    }
    return objects::Value{objects::String{std::move(body), objects::StringKind::Hex}};
}

// Inline-image preprocessing (§8.9.7). Raw image samples after the `ID`
// operator are NOT lexable PDF syntax; if the lexer sees them it mangles
// the rest of the stream (an unbalanced '(' in the samples swallows
// everything up to the next ')' as a literal string; a stray '[' / '<<'
// opens a container that then chokes on binary). We therefore strip the
// sample bytes from the raw input *before* lexing, leaving `ID EI` so the
// operators survive. The scan is string/comment-aware so an "ID" inside a
// text-show string is never mistaken for the operator.

// Whitespace per §7.2.3.
bool IsContentWs(std::byte b) {
    switch (std::to_integer<unsigned char>(b)) {
        case 0: case 9: case 10: case 12: case 13: case 32: return true;
        default: return false;
    }
}

// Find the offset just past the `EI` delimiter that ends inline-image
// data beginning at `pos`. `EI` is accepted only when bracketed by
// whitespace / a delimiter / the ends of input, so a stray `EI` inside
// the samples rarely false-matches. Returns input.size() if none found.
std::size_t FindInlineImageEnd(std::span<const std::byte> input,
                               std::size_t pos) {
    auto is_delim = [](std::byte b) {
        switch (std::to_integer<unsigned char>(b)) {
            case '(': case ')': case '<': case '>': case '[':
            case ']': case '{': case '}': case '/': case '%':
                return true;
            default: return false;
        }
    };
    for (std::size_t i = pos; i + 1 < input.size(); ++i) {
        if (input[i] != std::byte{'E'} || input[i + 1] != std::byte{'I'}) {
            continue;
        }
        const bool left_ok = (i == pos) || IsContentWs(input[i - 1]);
        const bool right_ok = (i + 2 >= input.size()) ||
                              IsContentWs(input[i + 2]) || is_delim(input[i + 2]);
        if (left_ok && right_ok) return i + 2;
    }
    return input.size();
}

// Copy `input`, dropping the raw sample bytes between every inline-image
// `ID` operator and its `EI` delimiter. The result lexes cleanly. Each
// dropped sample run is also recorded in `ranges` as a [start, end) byte
// offset into the ORIGINAL `in` (the §8.9.7 single-whitespace separators
// after `ID` and before `EI` trimmed), in stream order, so Parse can
// re-attach the samples as an inline-image Stream body without re-lexing
// the binary.
std::vector<std::byte> StripInlineImageData(
        std::span<const std::byte> in,
        std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
    std::vector<std::byte> out;
    out.reserve(in.size());
    enum { kNormal, kString, kHex, kComment } state = kNormal;
    int paren_depth = 0;
    std::size_t i = 0;
    while (i < in.size()) {
        const std::byte b = in[i];
        const unsigned char c = std::to_integer<unsigned char>(b);
        if (state == kComment) {
            out.push_back(b);
            if (c == '\n' || c == '\r') state = kNormal;
            ++i;
        } else if (state == kString) {
            out.push_back(b);
            if (c == '\\' && i + 1 < in.size()) {
                out.push_back(in[i + 1]);  // escaped byte, verbatim
                i += 2;
            } else {
                if (c == '(') ++paren_depth;
                else if (c == ')' && --paren_depth == 0) state = kNormal;
                ++i;
            }
        } else if (state == kHex) {
            out.push_back(b);
            if (c == '>') state = kNormal;
            ++i;
        } else {  // kNormal
            if (c == '%') {
                state = kComment; out.push_back(b); ++i;
            } else if (c == '(') {
                state = kString; paren_depth = 1; out.push_back(b); ++i;
            } else if (c == '<') {
                if (i + 1 < in.size() && in[i + 1] == std::byte{'<'}) {
                    out.push_back(b); out.push_back(in[i + 1]); i += 2;  // dict
                } else {
                    state = kHex; out.push_back(b); ++i;  // hex string
                }
            } else if (c == 'I' && i + 1 < in.size()
                       && in[i + 1] == std::byte{'D'}
                       && (i == 0 || IsContentWs(in[i - 1])
                           || in[i - 1] == std::byte{'>'})
                       && (i + 2 >= in.size() || IsContentWs(in[i + 2]))) {
                // Inline-image data operator. Keep `ID`, drop the samples,
                // re-emit a clean `EI` so both lex as operators. Record the
                // dropped sample run [data_start, data_end) into `in` so
                // Parse can re-attach it as the image's Stream body.
                out.push_back(in[i]); out.push_back(in[i + 1]);
                const std::size_t ei_end = FindInlineImageEnd(in, i + 2);
                std::size_t data_start = i + 2;
                // One mandatory whitespace separates `ID` from the samples.
                if (data_start < in.size() && IsContentWs(in[data_start]))
                    ++data_start;
                // ei_end is just past `EI`; back up to the `E`, then drop the
                // single whitespace the delimiter rule requires before it.
                std::size_t data_end =
                    (ei_end >= 2) ? ei_end - 2 : data_start;
                if (data_end > data_start && IsContentWs(in[data_end - 1]))
                    --data_end;
                if (data_end < data_start) data_end = data_start;
                ranges.emplace_back(data_start, data_end);
                out.push_back(std::byte{' '});
                out.push_back(std::byte{'E'});
                out.push_back(std::byte{'I'});
                i = ei_end;
            } else {
                out.push_back(b); ++i;
            }
        }
    }
    return out;
}

// 3. Container helpers

objects::Array ParseArray(TokenCursor& cursor) {
    objects::Array arr;
    for (;;) {
        Token t = cursor.Peek();
        if (t.kind == TokenKind::ArrayClose) {
            cursor.Next(); // consume ']'
            break;
        }
        if (t.kind == TokenKind::Eof) {
            break;  // lenient: unterminated array — return what parsed
        }
        arr.items.push_back(ParseValue(cursor));
    }
    return arr;
}

objects::Dict ParseDict(TokenCursor& cursor) {
    objects::Dict dict;
    for (;;) {
        Token t = cursor.Peek();
        if (t.kind == TokenKind::DictClose) {
            cursor.Next(); // consume '>>'
            break;
        }
        if (t.kind == TokenKind::Eof) {
            break;  // lenient: unterminated dict — return what parsed
        }
        // Key must be a Name; a stray non-name token is skipped rather
        // than failing the page.
        if (t.kind != TokenKind::Name) {
            cursor.Next();
            continue;
        }
        Token key_token = cursor.Next();
        std::string key_str;
        if (key_token.bytes.size() > 0 && key_token.bytes[0] == std::byte{'/'}) {
            key_str = std::string(reinterpret_cast<const char*>(key_token.bytes.data() + 1),
                                  key_token.bytes.size() - 1);
        } else {
            key_str = std::string(reinterpret_cast<const char*>(key_token.bytes.data()),
                                  key_token.bytes.size());
        }
        // Decode #XX escapes in key
        std::string decoded_key = DecodeNameBody(
            std::span<const std::byte>(
                reinterpret_cast<const std::byte*>(key_str.data()),
                key_str.size()));
        // ParseValue consumes the value's first token itself —
        // any cursor.Next() here would drop it.
        objects::Value val = ParseValue(cursor);
        dict.entries.emplace_back(std::move(decoded_key), std::move(val));
    }
    return dict;
}

// 4. Value dispatcher

objects::Value ParseValue(TokenCursor& cursor) {
    Token t = cursor.Next();
    switch (t.kind) {
        case TokenKind::Number:
            return DecodeNumber(t.bytes);
        case TokenKind::Name:
            return DecodeName(t.bytes);
        case TokenKind::StringLiteral:
            return DecodeLiteralString(t.bytes);
        case TokenKind::StringHex:
            return DecodeHexString(t.bytes);
        case TokenKind::ArrayOpen:
            // '[' already consumed by the Next() above — ParseArray
            // only walks the body + closing ']'.
            return objects::Value{ParseArray(cursor)};
        case TokenKind::DictOpen:
            // '<<' already consumed by the Next() above.
            return objects::Value{ParseDict(cursor)};
        case TokenKind::Keyword: {
            std::string_view kw(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
            if (kw == "true") return objects::Value{true};
            if (kw == "false") return objects::Value{false};
            if (kw == "null") return objects::Value{std::monostate{}};
            // Try indirect reference: N M R
            std::optional<objects::Ref> ref = TryParseIndirectRef(cursor);
            if (ref) return objects::Value{*ref};
            // A keyword (operator) in value position — e.g. inside a
            // malformed array/dict operand. Lenient: drop it as a null
            // placeholder and carry on instead of failing the whole page.
            // (Content-stream parsing is non-semantic; validation is a
            // separate concern — see the intent section of the spec.)
            return objects::Value{std::monostate{}};
        }
        default:
            // Eof must still terminate (a value-context loop would spin
            // otherwise); any other stray token (a lone '>' / ']' / '>>'
            // in operand position) is skipped as null — field reproducer
            // 31458 has a stray '>' early in the content stream.
            if (t.kind == TokenKind::Eof)
                throw std::runtime_error(
                    "unexpected end of input in value position");
            return objects::Value{std::monostate{}};
    }
}

// 6. Helper: skip trivia tokens (Whitespace, Comment)
Token NextSignificant(TokenCursor& cursor) {
    for (;;) {
        Token t = cursor.Next();
        if (t.kind != TokenKind::Whitespace &&
            t.kind != TokenKind::Comment) {
            return t;
        }
    }
}

// 7. Helper: parse indirect reference (N M R) when present.
std::optional<objects::Ref> TryParseIndirectRef(TokenCursor& cursor) {
    // We already consumed the 'true'/'false'/'null' keyword.
    // Now expect two numbers followed by 'R'.
    Token n1 = cursor.Peek();
    if (n1.kind != TokenKind::Number) return std::nullopt;
    cursor.Next(); // consume n1

    Token n2 = cursor.Peek();
    if (n2.kind != TokenKind::Number) {
        // Not an indirect reference (n1 was a bare operand) — lenient.
        return std::nullopt;
    }
    cursor.Next(); // consume n2

    Token r = cursor.Peek();
    if (r.kind != TokenKind::Keyword) return std::nullopt;
    std::string_view rv(reinterpret_cast<const char*>(r.bytes.data()), r.bytes.size());
    if (rv != "R") return std::nullopt;
    cursor.Next(); // consume 'R'

    // Parse numbers
    std::int64_t id = 0, gen = 0;
    {
        std::string_view n1s(reinterpret_cast<const char*>(n1.bytes.data()), n1.bytes.size());
        auto [p, ec] = std::from_chars(n1s.data(), n1s.data() + n1s.size(), id);
        if (ec != std::errc() || p != n1s.data() + n1s.size()) {
            return std::nullopt;  // lenient: malformed id
        }
    }
    {
        std::string_view n2s(reinterpret_cast<const char*>(n2.bytes.data()), n2.bytes.size());
        auto [p, ec] = std::from_chars(n2s.data(), n2s.data() + n2s.size(), gen);
        if (ec != std::errc() || p != n2s.data() + n2s.size()) {
            return std::nullopt;  // lenient: malformed gen
        }
    }
    return objects::Ref{static_cast<std::uint32_t>(id), static_cast<std::uint16_t>(gen)};
}

// 5. Canonical helpers

std::string CanonicalName(const std::string& name) {
    std::string result = "/";
    for (unsigned char c : name) {
        bool regular = (c >= 0x21 && c <= 0x7E) &&
                       (c != '(') && (c != ')') && (c != '<') && (c != '>') &&
                       (c != '[') && (c != ']') && (c != '{') && (c != '}') &&
                       (c != '/') && (c != '#');
        if (regular) {
            result.push_back(static_cast<char>(c));
        } else {
            static const char hex[] = "0123456789ABCDEF";
            result.push_back('#');
            result.push_back(hex[(c >> 4) & 0xF]);
            result.push_back(hex[c & 0xF]);
        }
    }
    return result;
}

std::string CanonicalString(const objects::String& s) {
    std::string result = "<";
    for (std::byte b : s.bytes) {
        static const char hex[] = "0123456789ABCDEF";
        result.push_back(hex[static_cast<unsigned char>(b) >> 4]);
        result.push_back(hex[static_cast<unsigned char>(b) & 0xF]);
    }
    result.push_back('>');
    return result;
}

std::string CanonicalReal(double d) {
    // Shortest round-trip decimal with a '.'; integer-valued reals keep ".0"
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%.1f", d);
    if (n > 0 && static_cast<std::size_t>(n) < sizeof(buf)) {
        std::string s(buf, static_cast<std::size_t>(n));
        // Remove trailing zeros after decimal point, but keep at least one digit after '.'
        size_t dot_pos = s.find('.');
        if (dot_pos != std::string::npos) {
            size_t last_non_zero = s.find_last_not_of('0');
            if (last_non_zero != std::string::npos && last_non_zero > dot_pos) {
                s.erase(last_non_zero + 1);
            }
            if (s.back() == '.') {
                s += '0';
            }
        }
        return s;
    }
    return "0.0";
}

std::string CanonicalArray(const objects::Array& arr) {
    std::string result = "[ ";
    for (std::size_t i = 0; i < arr.items.size(); ++i) {
        if (i > 0) result += " ";
        result += CanonicalValue(arr.items[i]);
    }
    result += " ]";
    return result;
}

std::string CanonicalDict(const objects::Dict& dict) {
    // Sort entries by decoded key bytes (ASCII ascending)
    std::vector<std::pair<std::string, objects::Value>> entries = dict.entries;
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    std::string result = "<< ";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) result += " ";
        result += "/" + entries[i].first;
        result += " ";
        result += CanonicalValue(entries[i].second);
    }
    result += " >>";
    return result;
}

std::string CanonicalValue(const objects::Value& v) {
    using objects::Value;
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return CanonicalReal(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return CanonicalName(arg);
        } else if constexpr (std::is_same_v<T, objects::String>) {
            return CanonicalString(arg);
        } else if constexpr (std::is_same_v<T, objects::Array>) {
            return CanonicalArray(arg);
        } else if constexpr (std::is_same_v<T, objects::Dict>) {
            return CanonicalDict(arg);
        } else if constexpr (std::is_same_v<T, objects::Ref>) {
            return std::to_string(arg.id) + " " + std::to_string(arg.generation) + " R";
        } else {
            return "";
        }
    }, v.v);
}

}  // namespace

// Public API

Stream Parse(std::span<const std::byte> input) {
    // Strip inline-image sample bytes (§8.9.7) before lexing — they are
    // raw binary, not PDF tokens, and would otherwise derail the lexer
    // (and hence the parser). `cleaned` outlives the parse; operand
    // values and operator names are copied out, so nothing dangles.
    std::vector<std::pair<std::size_t, std::size_t>> inline_ranges;
    const std::vector<std::byte> cleaned =
        StripInlineImageData(input, inline_ranges);
    Lexer lex(std::span<const std::byte>(cleaned.data(), cleaned.size()));
    TokenCursor cursor(lex.Lex());

    Stream stream;
    std::vector<objects::Value> operands;
    // Index into `inline_ranges`, advanced as each `BI` is consumed; the
    // ranges are in the same stream order the loop encounters the images.
    std::size_t inline_idx = 0;

    // Pattern-A cursor ownership: top-level loop PEEKs to
    // classify; ParseValue owns Next() for operand tokens.
    // Mixing Peek+Next here with a Next() inside ParseValue
    // would drop every first operand.
    while (!cursor.AtEnd()) {
        Token t = cursor.Peek();
        if (t.kind == TokenKind::Eof) break;

        if (t.kind == TokenKind::Keyword) {
            std::string_view kw(
                reinterpret_cast<const char*>(t.bytes.data()),
                t.bytes.size());
            // null / true / false are operand-literals; delegate
            // to ParseValue (which handles them as Keyword-case).
            if (kw == "null" || kw == "true" || kw == "false") {
                operands.push_back(ParseValue(cursor));
                continue;
            }
            // "R" after two non-negative ints is an indirect
            // reference per §7.3.10 — consume, pop + push Ref.
            if (kw == "R" && operands.size() >= 2) {
                const auto* gen = std::get_if<std::int64_t>(
                    &operands.back().v);
                const auto* id = std::get_if<std::int64_t>(
                    &operands[operands.size() - 2].v);
                if (id && gen && *id >= 0 && *gen >= 0
                    && *id <= 0xffffffffLL
                    && *gen <= 0xffffLL) {
                    cursor.Next(); // consume 'R'
                    objects::Ref ref{
                        static_cast<std::uint32_t>(*id),
                        static_cast<std::uint16_t>(*gen)};
                    operands.pop_back();
                    operands.pop_back();
                    operands.push_back(objects::Value{ref});
                    continue;
                }
            }
            // Inline images (§8.9.7). StripInlineImageData removed the raw
            // sample bytes before lexing, so the token stream is
            // `BI <name/value pairs> ID EI`. Collect the parameter dict and
            // re-attach the dropped samples (recorded in `inline_ranges`,
            // spans into `input`) as a single INLINE_IMAGE operation whose
            // lone operand is a Stream {header = inline dict, body = samples}.
            // Abbreviation expansion (W→Width, /Fl→FlateDecode, …) and
            // decoding/rendering are the consumer's job — the parser carries
            // the inline image verbatim.
            if (kw == "BI") {
                cursor.Next();  // consume BI
                objects::Dict idict;
                bool saw_id = false;
                while (!cursor.AtEnd()) {
                    Token p = cursor.Peek();
                    if (p.kind == TokenKind::Eof) break;
                    if (p.kind == TokenKind::Keyword) {
                        std::string_view k(
                            reinterpret_cast<const char*>(p.bytes.data()),
                            p.bytes.size());
                        cursor.Next();  // consume the keyword
                        if (k == "ID") { saw_id = true; break; }
                        if (k == "EI") break;  // ID-less / malformed
                        continue;  // stray keyword in dict position — skip
                    }
                    if (p.kind != TokenKind::Name) {
                        ParseValue(cursor);  // skip a stray value
                        continue;
                    }
                    // Key (Name) — strip the leading '/' and decode #XX.
                    Token key_token = cursor.Next();
                    std::string key_str;
                    if (!key_token.bytes.empty()
                        && key_token.bytes[0] == std::byte{'/'}) {
                        key_str = std::string(
                            reinterpret_cast<const char*>(
                                key_token.bytes.data() + 1),
                            key_token.bytes.size() - 1);
                    } else {
                        key_str = std::string(
                            reinterpret_cast<const char*>(
                                key_token.bytes.data()),
                            key_token.bytes.size());
                    }
                    std::string decoded_key = DecodeNameBody(
                        std::span<const std::byte>(
                            reinterpret_cast<const std::byte*>(
                                key_str.data()),
                            key_str.size()));
                    objects::Value val = ParseValue(cursor);
                    idict.entries.emplace_back(std::move(decoded_key),
                                               std::move(val));
                }
                objects::Stream img;
                img.header = std::move(idict);
                if (inline_idx < inline_ranges.size()) {
                    const auto [s, e] = inline_ranges[inline_idx++];
                    img.body = input.subspan(s, e - s);
                }
                std::vector<objects::Value> img_ops;
                img_ops.push_back(objects::Value{std::move(img)});
                stream.operations.push_back(
                    {std::string("INLINE_IMAGE"), std::move(img_ops)});
                operands.clear();
                // Consume the trailing EI that StripInlineImageData re-emitted.
                if (saw_id && !cursor.AtEnd()) {
                    Token p = cursor.Peek();
                    if (p.kind == TokenKind::Keyword) {
                        std::string_view k(
                            reinterpret_cast<const char*>(p.bytes.data()),
                            p.bytes.size());
                        if (k == "EI") cursor.Next();
                    }
                }
                continue;
            }
            // Regular operator — consume keyword, flush operands.
            cursor.Next();
            std::string op(kw.begin(), kw.end());
            stream.operations.push_back(
                {std::move(op), std::move(operands)});
            operands.clear();
            continue;
        }
        // Operand token — ParseValue consumes it from the cursor.
        operands.push_back(ParseValue(cursor));
    }

    // A non-empty operand stack at end-of-stream means a trailing run of
    // operands had no terminating operator — malformed but tolerable. Drop
    // it (Acrobat / poppler do) instead of failing the page; the partial
    // operands carry no operation to emit.
    return stream;
}

std::string ToCanonical(const Stream& stream) {
    if (stream.operations.empty()) {
        return "";
    }
    std::string result;
    for (const auto& op : stream.operations) {
        if (!op.operands.empty()) {
            for (std::size_t i = 0; i < op.operands.size(); ++i) {
                if (i > 0) result += " ";
                result += CanonicalValue(op.operands[i]);
            }
            result += " ";
        }
        result += op.op;
        result += "\n";
    }
    return result;
}

} // namespace foundation::content_stream
