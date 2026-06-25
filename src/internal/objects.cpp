#include "objects.hpp"

#include "flate.hpp"

#include <algorithm>
#include <utility>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
// std::to_string — MSVC's STL doesn't pull <string> in
// transitively (libstdc++/libc++ happen to via <sstream>).
// Spell it explicitly for toolchain portability.
#include <string>
#include <string_view>
#include <vector>

namespace foundation::objects {

// Out-of-line special members for Value (declared in objects.hpp). Defined
// here, where Value/Dict/Array are complete, so the recursive-type trait
// instantiation that clang + libstdc++ reject is never forced on an
// incomplete Value at the header-include sites.
Value::Value() = default;
Value::Value(const Value&) = default;
Value::Value(Value&&) noexcept = default;
Value& Value::operator=(const Value&) = default;
Value& Value::operator=(Value&&) noexcept = default;
Value::~Value() = default;

namespace {

// Forward declarations for mutually recursive helpers
struct ParserState;
Value ParseValue(ParserState& state);
Array ParseArray(ParserState& state);
Dict ParseDict(ParserState& state);

// Locate a stream body's length by scanning forward for the
// `endstream` keyword. Used when the stream dict's /Length is
// absent or an indirect reference (`N M R`) — neither resolvable
// inside this positional, single-object parse (refs stay opaque).
// A single EOL (CRLF, LF, or lone CR) immediately preceding
// `endstream` is trimmed per §7.3.8.1. Throws when no `endstream`
// keyword follows the body.
std::size_t FindStreamBodyLengthByScan(std::span<const std::byte> input,
                                       std::size_t body_offset) {
    static constexpr std::string_view kEnd{"endstream"};
    for (std::size_t i = body_offset; i + kEnd.size() <= input.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < kEnd.size(); ++j) {
            if (input[i + j] != static_cast<std::byte>(kEnd[j])) {
                match = false;
                break;
            }
        }
        if (!match) {
            continue;
        }
        std::size_t end = i;
        if (end > body_offset && input[end - 1] == std::byte{0x0A}) {
            --end;  // LF
            if (end > body_offset && input[end - 1] == std::byte{0x0D}) {
                --end;  // CR of a CRLF pair
            }
        } else if (end > body_offset && input[end - 1] == std::byte{0x0D}) {
            --end;  // lone CR
        }
        return end - body_offset;
    }
    throw std::runtime_error(
        "stream with indirect or absent /Length has no 'endstream' keyword");
}

// Parser state: lexer, full input, base offset, pushback slot
// ParserState carries a pushback STACK rather than a single slot.
// The 3-token lookahead for indirect references (`N M R`) needs to
// be able to push two peeked tokens back when the pattern doesn't
// match, and we'd also like deeper lookaheads to be trivially
// available if future primitive-body grammar requires them. The
// back of the vector is the "top" of the stack — the next token
// Consume will return. When multiple tokens are pushed in the
// order (t1, t2, t3), Consume will yield them in the reverse order
// (t3, t2, t1); callers that want t1 to come out first must push
// t3 first, t2 second, t1 third.
struct ParserState {
    Lexer& lex;
    std::span<const std::byte> input;
    std::size_t base;
    std::vector<Token> pushback;
};

// Helper: consume next token, respecting pushback stack
Token Consume(ParserState& s) {
    if (!s.pushback.empty()) {
        Token t = s.pushback.back();
        s.pushback.pop_back();
        return t;
    }
    return s.lex.Next();
}

// Helper: skip trivia tokens (whitespace, comment)
void SkipTrivia(ParserState& state) {
    for (;;) {
        Token t = Consume(state);
        if (t.kind != TokenKind::Whitespace &&
            t.kind != TokenKind::Comment) {
            state.pushback.push_back(t);
            break;
        }
    }
}

// Helper: consume next non-trivia token
Token NextSignificant(ParserState& state) {
    SkipTrivia(state);
    return Consume(state);
}

// Helper: check if token matches expected keyword
bool IsKeyword(const Token& t, std::string_view kw) {
    if (t.kind != TokenKind::Keyword) return false;
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    return sv == kw;
}

// Helper: decode name from raw token bytes (includes leading '/')
std::string DecodeName(std::span<const std::byte> raw) {
    if (raw.size() < 1 || raw[0] != std::byte('/')) {
        throw std::runtime_error("invalid name token");
    }
    // §7.3.5: `#XX` is a two-hex-digit escape. A `#` that is NOT
    // followed by two hex digits is non-conforming; rather than reject
    // the whole document, treat that `#` as a literal byte (matching
    // poppler / mutool tolerance — e.g. a name like `/grd0#_`).
    auto hex_val = [](std::byte b) -> int {
        char c = static_cast<char>(b);
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        return -1;
    };
    std::string result;
    result.reserve(raw.size() - 1);
    for (size_t i = 1; i < raw.size(); ++i) {
        int high = -1, low = -1;
        if (raw[i] == std::byte('#') && i + 2 < raw.size()
                && (high = hex_val(raw[i + 1])) >= 0
                && (low = hex_val(raw[i + 2])) >= 0) {
            result.push_back(static_cast<char>((high << 4) | low));
            i += 2;
        } else {
            result.push_back(static_cast<char>(static_cast<unsigned char>(raw[i])));
        }
    }
    return result;
}

// Helper: decode literal string (parenthesized) from raw token bytes
std::vector<std::byte> DecodeLiteralString(std::span<const std::byte> raw) {
    if (raw.size() < 2 || raw[0] != std::byte('(') || raw[raw.size() - 1] != std::byte(')')) {
        throw std::runtime_error("invalid literal string token");
    }
    std::vector<std::byte> result;
    result.reserve(raw.size() - 2);
    for (size_t i = 1; i < raw.size() - 1; ++i) {
        if (raw[i] == std::byte('\\')) {
            if (++i >= raw.size() - 1) break;
            switch (static_cast<char>(raw[i])) {
                case 'n': result.push_back(std::byte(0x0A)); break;
                case 'r': result.push_back(std::byte(0x0D)); break;
                case 't': result.push_back(std::byte(0x09)); break;
                case 'b': result.push_back(std::byte(0x08)); break;
                case 'f': result.push_back(std::byte(0x0C)); break;
                case '\\': result.push_back(std::byte('\\')); break;
                case '(': result.push_back(std::byte('(')); break;
                case ')': result.push_back(std::byte(')')); break;
                case '\r':
                    // CRLF line continuation: skip LF if present
                    if (i + 1 < raw.size() - 1 && raw[i + 1] == std::byte('\n')) ++i;
                    break;
                case '\n':
                    // LF line continuation
                    break;
                default:
                    // Octal escape (1-3 digits)
                    int val = 0;
                    int count = 0;
                    while (count < 3 && i < raw.size() - 1 &&
                           raw[i] >= std::byte('0') && raw[i] <= std::byte('7')) {
                        val = val * 8 + (static_cast<char>(raw[i]) - '0');
                        ++i;
                        ++count;
                    }
                    result.push_back(std::byte(val));
                    --i; // loop will increment
                    break;
            }
        } else {
            result.push_back(raw[i]);
        }
    }
    return result;
}

// Helper: decode hex string from raw token bytes
std::vector<std::byte> DecodeHexString(std::span<const std::byte> raw) {
    if (raw.size() < 2 || raw[0] != std::byte('<') || raw[raw.size() - 1] != std::byte('>')) {
        throw std::runtime_error("invalid hex string token");
    }
    std::vector<std::byte> result;
    result.reserve((raw.size() - 2 + 1) / 2);
    int high_nibble = -1;
    for (size_t i = 1; i < raw.size() - 1; ++i) {
        char c = static_cast<char>(raw[i]);
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        int nibble;
        if (c >= '0' && c <= '9') nibble = c - '0';
        else if (c >= 'A' && c <= 'F') nibble = 10 + c - 'A';
        else if (c >= 'a' && c <= 'f') nibble = 10 + c - 'a';
        else continue; // skip invalid chars

        if (high_nibble == -1) {
            high_nibble = nibble;
        } else {
            result.push_back(std::byte((high_nibble << 4) | nibble));
            high_nibble = -1;
        }
    }
    if (high_nibble != -1) {
        result.push_back(std::byte(high_nibble << 4));
    }
    return result;
}

// Helper: parse integer from Number token
std::int64_t ParseInteger(const Token& t) {
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    std::size_t pos = 0;
    try {
        return std::stoll(std::string(sv), &pos);
    } catch (...) {
        throw std::runtime_error("invalid integer literal");
    }
    if (pos != sv.size()) {
        throw std::runtime_error("invalid integer literal");
    }
    return 0;
}

// Helper: parse real from Number token
double ParseReal(const Token& t) {
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    std::size_t pos = 0;
    try {
        return std::stod(std::string(sv), &pos);
    } catch (...) {
        throw std::runtime_error("invalid real literal");
    }
    if (pos != sv.size()) {
        throw std::runtime_error("invalid real literal");
    }
    return 0.0;
}

// Helper: check if token is a Number that looks like an integer (no '.')
bool IsIntegerToken(const Token& t) {
    if (t.kind != TokenKind::Number) return false;
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    return sv.find('.') == std::string_view::npos;
}

// Helper: check if token is a Number that looks like a real (has '.')
bool IsRealToken(const Token& t) {
    if (t.kind != TokenKind::Number) return false;
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    return sv.find('.') != std::string_view::npos;
}

// Parse a single value expression
Value ParseValue(ParserState& state) {
    Token t = NextSignificant(state);

    switch (t.kind) {
        case TokenKind::Keyword: {
            std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
            if (sv == "null") {
                return Value{std::monostate{}};
            } else if (sv == "true") {
                return Value{true};
            } else if (sv == "false") {
                return Value{false};
            } else if (sv == "obj") {
                throw std::runtime_error("unexpected 'obj' keyword in value context");
            } else if (sv == "endobj") {
                throw std::runtime_error("unexpected 'endobj' keyword in value context");
            } else if (sv == "stream") {
                throw std::runtime_error("unexpected 'stream' keyword in value context");
            } else if (sv == "endstream") {
                throw std::runtime_error("unexpected 'endstream' keyword in value context");
            } else {
                // Unknown keyword — treat as name
                return Value{std::string(sv)};
            }
        }

        case TokenKind::Number: {
            if (IsRealToken(t)) {
                // A real (has '.') is never the first token of an
                // indirect reference — refs are int-int-R. Dispatch
                // directly.
                return Value{ParseReal(t)};
            }
            if (!IsIntegerToken(t)) {
                throw std::runtime_error("invalid numeric literal");
            }
            std::int64_t first_int;
            try {
                first_int = ParseInteger(t);
            } catch (const std::exception&) {
                // An integer literal too large for int64 (e.g. a
                // ~2^128 value in a malformed numeric array) — readers
                // fall back to a real rather than rejecting the file.
                // Such a value cannot be an object id, so it is never
                // the head of an indirect reference.
                return Value{ParseReal(t)};
            }
            // why: indirect-reference lookahead (§7.3.10). `N M R`
            // with two non-negative integers followed by the keyword
            // `R` is an indirect reference; anything else is just
            // the first integer followed by independent values. We
            // peek two tokens ahead and push them BOTH back (via the
            // multi-slot pushback stack) when the pattern fails, so
            // the caller sees every token in source order.
            if (first_int < 0) {
                return Value{first_int};
            }
            Token t2 = NextSignificant(state);
            if (t2.kind != TokenKind::Number || !IsIntegerToken(t2)) {
                state.pushback.push_back(t2);
                return Value{first_int};
            }
            std::int64_t second_int = ParseInteger(t2);
            if (second_int < 0) {
                state.pushback.push_back(t2);
                return Value{first_int};
            }
            Token t3 = NextSignificant(state);
            if (IsKeyword(t3, "R")) {
                return Value{Ref{
                    static_cast<std::uint32_t>(first_int),
                    static_cast<std::uint16_t>(second_int)}};
            }
            // Not a ref — restore t2 and t3 to the stream. Push t3
            // first so t2 pops first (LIFO): caller's next Consume
            // yields t2, the one after that yields t3.
            state.pushback.push_back(t3);
            state.pushback.push_back(t2);
            return Value{first_int};
        }

        case TokenKind::Name: {
            return Value{DecodeName(t.bytes)};
        }

        case TokenKind::StringLiteral: {
            return Value{String{DecodeLiteralString(t.bytes), StringKind::Literal}};
        }

        case TokenKind::StringHex: {
            return Value{String{DecodeHexString(t.bytes), StringKind::Hex}};
        }

        case TokenKind::ArrayOpen: {
            return Value{ParseArray(state)};
        }

        case TokenKind::DictOpen: {
            Dict d = ParseDict(state);
            // Check if this dict is followed by 'stream'
            Token next = NextSignificant(state);
            if (IsKeyword(next, "stream")) {
                // The body starts after EXACTLY ONE end-of-line marker
                // (CRLF or LF) following the `stream` keyword, per
                // §7.3.8.1 — never a greedy whitespace run. A stream
                // body is arbitrary binary and PDF classes NUL (0x00),
                // among others, as whitespace; consuming a lexer
                // whitespace token here would wrongly swallow leading
                // body bytes (e.g. a JPXDecode/JP2 image that opens
                // with 0x00 0x00 0x00 0x0C), corrupting body_offset.
                std::size_t body_offset =
                    state.base + next.offset + next.bytes.size();
                if (body_offset < state.input.size() &&
                    state.input[body_offset] == std::byte{0x0D}) {
                    ++body_offset;  // CR
                    if (body_offset < state.input.size() &&
                        state.input[body_offset] == std::byte{0x0A}) {
                        ++body_offset;  // LF of a CRLF pair
                    }
                } else if (body_offset < state.input.size() &&
                           state.input[body_offset] == std::byte{0x0A}) {
                    ++body_offset;  // lone LF
                }
                // Determine the body length. The /Length entry is a
                // direct non-negative integer in spec-canonical PDFs;
                // skip forward by exactly that many bytes. But real
                // PDFs commonly express /Length as an indirect
                // reference (`N M R`) resolved via xref, or omit it —
                // neither resolvable in this positional parse. In that
                // case locate the body end by scanning for `endstream`
                // (refs stay opaque). /Length 0 (a legitimately empty
                // body) is honoured via the direct-integer path.
                const Value* length_val = nullptr;
                for (const auto& entry : d.entries) {
                    if (entry.first == "Length") {
                        length_val = &entry.second;
                        break;
                    }
                }
                if (length_val == nullptr) {
                    throw std::runtime_error("stream missing /Length");
                }
                std::size_t length = 0;
                bool have_direct_length = false;
                if (std::holds_alternative<std::int64_t>(length_val->v)) {
                    std::int64_t raw = std::get<std::int64_t>(length_val->v);
                    if (raw < 0) {
                        throw std::runtime_error("/Length must be non-negative");
                    }
                    length = static_cast<std::size_t>(raw);
                    have_direct_length = true;
                } else {
                    // /Length present but not a direct integer — an
                    // indirect reference (`N M R`) resolved via xref in
                    // real PDFs, unresolvable in this positional parse.
                    // Recover the body end by scanning for `endstream`.
                    length = FindStreamBodyLengthByScan(state.input, body_offset);
                }
                // Helper: position the lexer just past `length` body bytes
                // and report whether `endstream` follows.
                auto endstream_follows = [&](std::size_t len) {
                    if (body_offset + len > state.input.size()) return false;
                    state.lex = Lexer(state.input.subspan(body_offset + len));
                    state.base = body_offset + len;
                    state.pushback.clear();
                    return IsKeyword(NextSignificant(state), "endstream");
                };
                // A direct /Length is non-authoritative in the wild — when
                // it does not land exactly on `endstream` (off by a few
                // bytes, or plain wrong), recover the true body length by
                // scanning, as poppler/mutool do. The scan result is the
                // fallback for both a misaligned direct /Length and an
                // indirect one.
                if (!endstream_follows(length)) {
                    if (have_direct_length) {
                        length = FindStreamBodyLengthByScan(state.input,
                                                            body_offset);
                    }
                    if (!endstream_follows(length)) {
                        throw std::runtime_error(
                            "expected 'endstream' after stream body");
                    }
                }
                // Body span points into the parsed input buffer
                // (no copy). The Stream value must not outlive
                // the source bytes that backed `Parse`.
                std::span<const std::byte> body_span =
                    state.input.subspan(body_offset, length);
                return Value{Stream{d, body_span}};
            } else {
                state.pushback.push_back(next);
                return Value{d};
            }
        }

        default:
            throw std::runtime_error("unexpected token in value context");
    }
}

// Parse array
Array ParseArray(ParserState& state) {
    Array arr;
    for (;;) {
        Token t = NextSignificant(state);
        if (t.kind == TokenKind::ArrayClose) {
            break;
        }
        state.pushback.push_back(t);
        arr.items.push_back(ParseValue(state));
    }
    return arr;
}

// Parse dictionary
Dict ParseDict(ParserState& state) {
    Dict dict;
    std::vector<std::pair<std::string, Value>> entries;
    for (;;) {
        Token key_token = NextSignificant(state);
        if (key_token.kind == TokenKind::DictClose) {
            break;
        }
        if (key_token.kind != TokenKind::Name) {
            throw std::runtime_error("dictionary key must be a name");
        }
        std::string key = DecodeName(key_token.bytes);
        Value val = ParseValue(state);
        // A duplicate key is non-conforming (§7.3.7), but real files
        // carry them; readers keep the last occurrence rather than
        // rejecting the dict. Overwrite an existing entry in place.
        bool replaced = false;
        for (auto& entry : entries) {
            if (entry.first == key) {
                entry.second = val;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            entries.emplace_back(key, val);
        }
    }
    dict.entries = std::move(entries);
    return dict;
}

// Parse indirect object starting at offset pointing to "N M obj"
IndirectObject ParseIndirectObject(ParserState& state) {
    Token t1 = NextSignificant(state);
    if (!IsIntegerToken(t1)) {
        throw std::runtime_error("expected object number");
    }
    std::uint32_t id = static_cast<std::uint32_t>(ParseInteger(t1));

    Token t2 = NextSignificant(state);
    if (!IsIntegerToken(t2)) {
        throw std::runtime_error("expected generation number");
    }
    std::uint16_t gen = static_cast<std::uint16_t>(ParseInteger(t2));

    Token t3 = NextSignificant(state);
    if (!IsKeyword(t3, "obj")) {
        throw std::runtime_error("expected 'obj' keyword");
    }

    Value val = ParseValue(state);

    Token t4 = NextSignificant(state);
    if (!IsKeyword(t4, "endobj")) {
        throw std::runtime_error("expected 'endobj' keyword");
    }

    return IndirectObject{id, gen, val};
}

// Canonical helpers
std::string CanonicalValue(const Value& v);

std::string CanonicalArray(const Array& arr) {
    if (arr.items.empty()) return "[ ]";
    std::string result = "[ ";
    for (size_t i = 0; i < arr.items.size(); ++i) {
        if (i > 0) result += " ";
        result += CanonicalValue(arr.items[i]);
    }
    result += " ]";
    return result;
}

std::string CanonicalDict(const Dict& dict) {
    if (dict.entries.empty()) return "<< >>";
    // Sort by decoded key bytes (ASCII ascending)
    std::vector<std::pair<std::string, Value>> sorted = dict.entries;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });
    std::string result = "<< ";
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) result += " ";
        // Escape key for canonical form
        const std::string& key = sorted[i].first;
        std::string escaped_key;
        for (unsigned char c : key) {
            if ((c >= 0x21 && c <= 0x7E) && c != '#' && c != '<' && c != '>' &&
                c != '(' && c != ')' && c != '[' && c != ']' && c != '{' && c != '}' &&
                c != '/' && c != '%') {
                escaped_key.push_back(static_cast<char>(c));
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "#%02X", c);
                escaped_key += buf;
            }
        }
        result += "/" + escaped_key + " " + CanonicalValue(sorted[i].second);
    }
    result += " >>";
    return result;
}

std::string CanonicalValue(const Value& v) {
    if (std::holds_alternative<std::monostate>(v.v)) {
        return "null";
    } else if (std::holds_alternative<bool>(v.v)) {
        return std::get<bool>(v.v) ? "true" : "false";
    } else if (std::holds_alternative<std::int64_t>(v.v)) {
        return std::to_string(std::get<std::int64_t>(v.v));
    } else if (std::holds_alternative<double>(v.v)) {
        double d = std::get<double>(v.v);
        std::ostringstream oss;
        // Use fixed with enough precision for round-trip
        oss << std::fixed << std::setprecision(15) << d;
        std::string s = oss.str();
        // Remove trailing zeros after decimal point, but keep at least one digit after decimal
        size_t pos = s.find('.');
        if (pos != std::string::npos) {
            // Remove trailing zeros
            while (s.size() > pos + 1 && s.back() == '0') {
                s.pop_back();
            }
            // Ensure at least one digit after decimal point
            if (s.back() == '.') {
                s += '0';
            }
        }
        return s;
    } else if (std::holds_alternative<std::string>(v.v)) {
        const std::string& name = std::get<std::string>(v.v);
        std::string escaped;
        escaped.reserve(name.size() + 2);
        escaped.push_back('/');
        for (unsigned char c : name) {
            if ((c >= 0x21 && c <= 0x7E) && c != '#' && c != '<' && c != '>' &&
                c != '(' && c != ')' && c != '[' && c != ']' && c != '{' && c != '}' &&
                c != '/' && c != '%') {
                escaped.push_back(static_cast<char>(c));
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "#%02X", c);
                escaped += buf;
            }
        }
        return escaped;
    } else if (std::holds_alternative<String>(v.v)) {
        const String& str = std::get<String>(v.v);
        std::ostringstream oss;
        oss << '<';
        for (std::byte b : str.bytes) {
            unsigned char c = static_cast<unsigned char>(b);
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", c);
            oss << buf;
        }
        oss << '>';
        return oss.str();
    } else if (std::holds_alternative<Array>(v.v)) {
        return CanonicalArray(std::get<Array>(v.v));
    } else if (std::holds_alternative<Dict>(v.v)) {
        return CanonicalDict(std::get<Dict>(v.v));
    } else if (std::holds_alternative<Stream>(v.v)) {
        const Stream& stream = std::get<Stream>(v.v);
        return "stream " + CanonicalDict(stream.header);
    } else if (std::holds_alternative<Ref>(v.v)) {
        const Ref& ref = std::get<Ref>(v.v);
        return std::to_string(ref.id) + " " + std::to_string(ref.generation) + " R";
    }
    return "";
}

// Decode an object stream's body (§7.5.7). Object streams carry
// /Filter /FlateDecode (a name or a one-element array) or no filter.
// A /DecodeParms predictor is not used by object streams in practice;
// reject it rather than silently mis-decode.
std::vector<std::byte> DecodeObjStmBody(std::span<const std::byte> body,
                                        const Value* filter_val,
                                        const Value* parms_val) {
    auto is_flate = [](const Value& v) {
        return std::holds_alternative<std::string>(v.v) &&
               std::get<std::string>(v.v) == "FlateDecode";
    };
    bool flate = false;
    if (filter_val != nullptr) {
        if (is_flate(*filter_val)) {
            flate = true;
        } else if (std::holds_alternative<Array>(filter_val->v)) {
            const auto& arr = std::get<Array>(filter_val->v).items;
            if (arr.size() == 1 && is_flate(arr[0])) {
                flate = true;
            }
        }
        if (!flate) {
            throw std::runtime_error("object stream: unsupported /Filter");
        }
    }
    if (parms_val != nullptr && std::holds_alternative<Dict>(parms_val->v)) {
        for (const auto& kv : std::get<Dict>(parms_val->v).entries) {
            if (kv.first == "Predictor" &&
                std::holds_alternative<std::int64_t>(kv.second.v) &&
                std::get<std::int64_t>(kv.second.v) >= 2) {
                throw std::runtime_error("object stream: /Predictor unsupported");
            }
        }
    }
    if (!flate) {
        return std::vector<std::byte>(body.begin(), body.end());
    }
    return foundation::flate::Decode(body);
}

// Object-stream extraction (§7.5.7). For each /ObjStm container named
// by a type-2 (compressed) xref entry, decode the container stream,
// read its N (object-number, byte-offset) header pairs, and parse each
// contained object's value. Objects in an object stream are direct
// values — never streams, never generation != 0 — so the parsed
// Values own their bytes and stay valid after the decoded buffer is
// released. Extracted objects are appended to `dump` with generation 0
// (skipping any id already present from the verbatim type-1 pass).
void ExtractObjStmObjects(Dump& dump,
                          const std::vector<xref::Entry>& compressed) {
    std::vector<std::uint32_t> stream_ids;
    for (const auto& e : compressed) {
        auto sid = static_cast<std::uint32_t>(e.offset_or_stream_id);
        if (std::find(stream_ids.begin(), stream_ids.end(), sid) ==
            stream_ids.end()) {
            stream_ids.push_back(sid);
        }
    }

    for (std::uint32_t sid : stream_ids) {
        const IndirectObject* container = nullptr;
        for (const auto& o : dump.objects) {
            if (o.id == sid) {
                container = &o;
                break;
            }
        }
        if (container == nullptr) {
            throw std::runtime_error("object stream container not found");
        }
        const Stream* stm = std::get_if<Stream>(&container->value.v);
        if (stm == nullptr) {
            throw std::runtime_error("object stream container is not a stream");
        }

        std::int64_t n = -1;
        std::int64_t first = -1;
        const Value* filter_val = nullptr;
        const Value* parms_val = nullptr;
        for (const auto& kv : stm->header.entries) {
            if (kv.first == "N" &&
                std::holds_alternative<std::int64_t>(kv.second.v)) {
                n = std::get<std::int64_t>(kv.second.v);
            } else if (kv.first == "First" &&
                       std::holds_alternative<std::int64_t>(kv.second.v)) {
                first = std::get<std::int64_t>(kv.second.v);
            } else if (kv.first == "Filter") {
                filter_val = &kv.second;
            } else if (kv.first == "DecodeParms") {
                parms_val = &kv.second;
            }
        }
        if (n < 0 || first < 0) {
            throw std::runtime_error("object stream missing /N or /First");
        }

        std::vector<std::byte> decoded =
            DecodeObjStmBody(stm->body, filter_val, parms_val);
        std::span<const std::byte> dspan(decoded);

        // Header: N pairs of (object-number, offset-relative-to-/First).
        Lexer hlex(dspan);
        ParserState hstate{hlex, dspan, 0, {}};
        std::vector<std::pair<std::uint32_t, std::size_t>> entries;
        entries.reserve(static_cast<std::size_t>(n));
        for (std::int64_t i = 0; i < n; ++i) {
            Token num = NextSignificant(hstate);
            Token off = NextSignificant(hstate);
            if (num.kind != TokenKind::Number ||
                off.kind != TokenKind::Number) {
                throw std::runtime_error("object stream header malformed");
            }
            auto objnum = static_cast<std::uint32_t>(ParseInteger(num));
            auto rel = static_cast<std::size_t>(ParseInteger(off));
            entries.emplace_back(objnum,
                                 static_cast<std::size_t>(first) + rel);
        }

        for (const auto& [objnum, abs_off] : entries) {
            bool exists = false;
            for (const auto& o : dump.objects) {
                if (o.id == objnum) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                continue;
            }
            if (abs_off > decoded.size()) {
                throw std::runtime_error(
                    "object stream entry offset out of range");
            }
            Lexer olex(dspan.subspan(abs_off));
            ParserState ostate{olex, dspan, abs_off, {}};
            Value v = ParseValue(ostate);
            dump.objects.push_back(IndirectObject{objnum, 0, std::move(v)});
        }
    }
}

} // namespace

Dump Parse(std::span<const std::byte> input) {
    if (input.empty()) {
        throw std::runtime_error("empty input");
    }

    // Parse xref table
    xref::Table xref_table = xref::Parse(input);

    // Split entries: type-1 (InUse, verbatim at a file offset) and
    // type-2 (Compressed, packed inside an /ObjStm container).
    std::vector<xref::Entry> inuse_entries;
    std::vector<xref::Entry> compressed_entries;
    for (const auto& entry : xref_table.entries) {
        if (entry.kind == xref::EntryKind::InUse) {
            inuse_entries.push_back(entry);
        } else if (entry.kind == xref::EntryKind::Compressed) {
            compressed_entries.push_back(entry);
        }
    }

    // Sort by (id, generation)
    std::sort(inuse_entries.begin(), inuse_entries.end(),
              [](const auto& a, const auto& b) {
                  if (a.id != b.id) return a.id < b.id;
                  return a.generation < b.generation;
              });

    Dump dump;
    dump.objects.reserve(inuse_entries.size());

    // Parse each object. A single malformed object (corrupt body,
    // wrong /Length, binary garbage where `endobj` should be) is
    // skipped rather than failing the whole document — the rest of the
    // object graph still loads, matching poppler/mutool recovery. A
    // dropped object simply resolves to "missing" for its consumers.
    for (const auto& entry : inuse_entries) {
        std::size_t offset = static_cast<std::size_t>(entry.offset_or_stream_id);
        if (offset >= input.size()) {
            continue;
        }
        try {
            Lexer lex(input.subspan(offset));
            ParserState state{lex, input, offset, {}};
            IndirectObject obj = ParseIndirectObject(state);
            dump.objects.push_back(obj);
        } catch (const std::exception&) {
            continue;
        }
    }

    // Extract objects packed inside /ObjStm containers (§7.5.7). The
    // containers themselves are type-1 objects parsed above, so this
    // runs after the verbatim pass.
    if (!compressed_entries.empty()) {
        ExtractObjStmObjects(dump, compressed_entries);
        // Keep the dump sorted by (id, generation) — the verbatim pass
        // emitted in sorted order, but extracted objects are appended.
        std::sort(dump.objects.begin(), dump.objects.end(),
                  [](const auto& a, const auto& b) {
                      if (a.id != b.id) return a.id < b.id;
                      return a.generation < b.generation;
                  });
    }

    return dump;
}

std::string ToCanonical(const Dump& dump) {
    std::string result;
    for (const auto& obj : dump.objects) {
        result += std::to_string(obj.id) + " " +
                  std::to_string(obj.generation) + " " +
                  CanonicalValue(obj.value) + "\n";
    }
    return result;
}

} // namespace foundation::objects
