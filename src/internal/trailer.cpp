#include "trailer.hpp"
#include "lexer.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::trailer {

namespace {

// Helper: locate last 4 KiB of input, scan for "startxref"
std::size_t FindStartXrefOffset(std::span<const std::byte> input) {
    constexpr std::size_t kMaxLookback = 4096;
    std::size_t start = (input.size() > kMaxLookback) ? input.size() - kMaxLookback : 0;

    std::string_view keyword{"startxref"};
    for (std::size_t i = start; i + keyword.size() <= input.size(); ++i) {
        if (std::memcmp(&input[i], keyword.data(), keyword.size()) == 0) {
            // Verify it's a standalone keyword (preceded by whitespace or start)
            bool ok_start = (i == 0);
            if (!ok_start) {
                std::byte prev = input[i - 1];
                ok_start = (prev == std::byte{0x00} || prev == std::byte{0x09} ||
                            prev == std::byte{0x0A} || prev == std::byte{0x0C} ||
                            prev == std::byte{0x0D} || prev == std::byte{0x20});
            }
            if (!ok_start) continue;

            // Verify followed by whitespace or end
            std::size_t end_pos = i + keyword.size();
            bool ok_end = (end_pos >= input.size());
            if (!ok_end) {
                std::byte next = input[end_pos];
                ok_end = (next == std::byte{0x00} || next == std::byte{0x09} ||
                          next == std::byte{0x0A} || next == std::byte{0x0C} ||
                          next == std::byte{0x0D} || next == std::byte{0x20});
            }
            if (!ok_end) continue;

            return i;
        }
    }
    throw std::runtime_error("no \"startxref\" keyword in the last 4 KiB");
}

// Helper: scan backwards from startxref for "trailer". Returns the
// offset of the keyword's first byte when found, or nullopt when the
// file has no classical trailer (a PDF 1.5+ XRef-stream file — the
// caller then takes the XRef-stream path).
std::optional<std::size_t> FindTrailerKeywordOffset(std::span<const std::byte> input, std::size_t startxref_offset) {
    std::string_view keyword{"trailer"};
    for (std::size_t i = startxref_offset; i >= keyword.size(); --i) {
        if (std::memcmp(&input[i - keyword.size()], keyword.data(), keyword.size()) == 0) {
            // Verify it's a standalone keyword (preceded by whitespace or start)
            bool ok_start = (i == keyword.size());
            if (!ok_start) {
                std::byte prev = input[i - keyword.size() - 1];
                ok_start = (prev == std::byte{0x00} || prev == std::byte{0x09} ||
                            prev == std::byte{0x0A} || prev == std::byte{0x0C} ||
                            prev == std::byte{0x0D} || prev == std::byte{0x20});
            }
            if (!ok_start) continue;

            // Verify followed by whitespace or end
            std::size_t end_pos = i;
            bool ok_end = (end_pos >= input.size());
            if (!ok_end) {
                std::byte next = input[end_pos];
                ok_end = (next == std::byte{0x00} || next == std::byte{0x09} ||
                          next == std::byte{0x0A} || next == std::byte{0x0C} ||
                          next == std::byte{0x0D} || next == std::byte{0x20});
            }
            if (!ok_end) continue;

            return i - keyword.size();
        }
    }
    return std::nullopt;
}

// Helper: skip whitespace and comments, return next non-trivia token
Token NextSignificant(Lexer& lex) {
    for (;;) {
        Token t = lex.Next();
        if (t.kind != TokenKind::Whitespace &&
            t.kind != TokenKind::Comment) {
            return t;
        }
    }
}

// Helper: parse an integer from a Number token's bytes
std::uint32_t ParseUint32FromNumber(std::span<const std::byte> bytes) {
    std::string s;
    s.reserve(bytes.size());
    for (std::byte b : bytes) {
        s.push_back(static_cast<char>(static_cast<unsigned char>(b)));
    }
    std::size_t pos = 0;
    int sign = 1;
    if (!s.empty() && s[0] == '-') {
        sign = -1;
        ++pos;
    } else if (!s.empty() && s[0] == '+') {
        ++pos;
    }
    std::uint32_t value = 0;
    for (; pos < s.size(); ++pos) {
        char c = s[pos];
        if (c < '0' || c > '9') break;
        value = value * 10 + (c - '0');
    }
    if (sign < 0) {
        throw std::runtime_error("/Size must be a non-negative integer");
    }
    return value;
}

// Helper: parse a trailer dict body (<< ... >>) and extract the four keys
Trailer ParseTrailerDict(Lexer& lex) {
    Token open = NextSignificant(lex);
    if (open.kind != TokenKind::DictOpen) {
        throw std::runtime_error("Trailer dict must start with '<<'");
    }

    Trailer result{0, 0, 0, 0};
    bool has_size = false;
    bool has_root = false;

    for (;;) {
        Token key = NextSignificant(lex);
        if (key.kind == TokenKind::DictClose) {
            break;
        }
        if (key.kind != TokenKind::Name) {
            throw std::runtime_error("Trailer dict key must be a Name");
        }

        // Decode name (strip leading '/', resolve #XX escapes)
        std::string_view key_view(reinterpret_cast<const char*>(key.bytes.data()), key.bytes.size());
        if (key_view.size() < 2 || key_view[0] != '/') {
            throw std::runtime_error("Trailer dict key must start with '/'");
        }
        std::string decoded_name;
        for (std::size_t i = 1; i < key_view.size(); ++i) {
            if (key_view[i] == '#' && i + 2 < key_view.size()) {
                char hi = key_view[i + 1];
                char lo = key_view[i + 2];
                int v = 0;
                if (hi >= '0' && hi <= '9') v = (hi - '0') << 4;
                else if (hi >= 'A' && hi <= 'F') v = (hi - 'A' + 10) << 4;
                else if (hi >= 'a' && hi <= 'f') v = (hi - 'a' + 10) << 4;
                else throw std::runtime_error("Invalid hex escape in name");
                if (lo >= '0' && lo <= '9') v |= (lo - '0');
                else if (lo >= 'A' && lo <= 'F') v |= (lo - 'A' + 10);
                else if (lo >= 'a' && lo <= 'f') v |= (lo - 'a' + 10);
                else throw std::runtime_error("Invalid hex escape in name");
                decoded_name.push_back(static_cast<char>(v));
                i += 2;
            } else {
                decoded_name.push_back(key_view[i]);
            }
        }

        Token val = NextSignificant(lex);

        if (decoded_name == "Size") {
            if (val.kind != TokenKind::Number) {
                throw std::runtime_error("/Size must be an integer");
            }
            result.size = ParseUint32FromNumber(val.bytes);
            has_size = true;
        } else if (decoded_name == "Root") {
            // /Root is written as "id gen R"
            if (val.kind != TokenKind::Number) {
                throw std::runtime_error("/Root: expected object id");
            }
            std::uint32_t root_id = ParseUint32FromNumber(val.bytes);
            Token gen_token = NextSignificant(lex);
            if (gen_token.kind != TokenKind::Number) {
                throw std::runtime_error("/Root: expected generation");
            }
            std::uint32_t root_gen = ParseUint32FromNumber(gen_token.bytes);
            Token r_token = NextSignificant(lex);
            if (r_token.kind != TokenKind::Keyword ||
                std::string_view(reinterpret_cast<const char*>(r_token.bytes.data()), r_token.bytes.size()) != "R") {
                throw std::runtime_error("/Root: expected 'R' keyword");
            }
            result.root_id = root_id;
            has_root = true;
        } else if (decoded_name == "Info") {
            if (val.kind != TokenKind::Number) {
                throw std::runtime_error("/Info: expected object id");
            }
            std::uint32_t info_id = ParseUint32FromNumber(val.bytes);
            Token gen_token = NextSignificant(lex);
            if (gen_token.kind != TokenKind::Number) {
                throw std::runtime_error("/Info: expected generation");
            }
            std::uint32_t info_gen = ParseUint32FromNumber(gen_token.bytes);
            Token r_token = NextSignificant(lex);
            if (r_token.kind != TokenKind::Keyword ||
                std::string_view(reinterpret_cast<const char*>(r_token.bytes.data()), r_token.bytes.size()) != "R") {
                throw std::runtime_error("/Info: expected 'R' keyword");
            }
            result.info_id = info_id;
        } else if (decoded_name == "Encrypt") {
            if (val.kind != TokenKind::Number) {
                throw std::runtime_error("/Encrypt: expected object id");
            }
            std::uint32_t enc_id = ParseUint32FromNumber(val.bytes);
            Token gen_token = NextSignificant(lex);
            if (gen_token.kind != TokenKind::Number) {
                throw std::runtime_error("/Encrypt: expected generation");
            }
            std::uint32_t enc_gen = ParseUint32FromNumber(gen_token.bytes);
            Token r_token = NextSignificant(lex);
            if (r_token.kind != TokenKind::Keyword ||
                std::string_view(reinterpret_cast<const char*>(r_token.bytes.data()), r_token.bytes.size()) != "R") {
                throw std::runtime_error("/Encrypt: expected 'R' keyword");
            }
            result.encrypt_id = enc_id;
        } else {
            // Unknown key — skip the value. Simple-token values
            // are already consumed (val is the read-ahead). For
            // compound values (Array, Dict) we have to walk
            // nested opens/closes; the most common case is /ID's
            // 2-element string array, which v1 (single-token
            // skip) tripped over.
            if (val.kind == TokenKind::ArrayOpen
                    || val.kind == TokenKind::DictOpen) {
                int depth = 1;
                while (depth > 0) {
                    Token t = NextSignificant(lex);
                    if (t.kind == TokenKind::Eof) {
                        throw std::runtime_error(
                            "Trailer dict: unbalanced compound "
                            "value (eof inside [...] / <<...>>)");
                    }
                    if (t.kind == TokenKind::ArrayOpen
                            || t.kind == TokenKind::DictOpen) {
                        ++depth;
                    } else if (t.kind == TokenKind::ArrayClose
                            || t.kind == TokenKind::DictClose) {
                        --depth;
                    }
                }
            }
            // Note: indirect-ref values (`<id> <gen> R`) for
            // unknown keys would leave 2 extra tokens unread.
            // No real-world trailer uses such; if we ever see
            // one, extend this branch with a 3-token lookahead.
        }
    }

    if (!has_size) {
        throw std::runtime_error("trailer missing required /Size");
    }
    if (!has_root) {
        throw std::runtime_error("trailer missing required /Root");
    }

    return result;
}

// Helper: read the byte offset that follows the ``startxref`` keyword.
// For an XRef-stream file this offset points at the cross-reference
// stream object whose dict carries the trailer fields.
std::size_t ReadStartXrefTargetOffset(std::span<const std::byte> input,
                                      std::size_t startxref_offset) {
    std::size_t after = startxref_offset + std::string_view{"startxref"}.size();
    Lexer lex(input.subspan(after));
    Token num = NextSignificant(lex);
    if (num.kind != TokenKind::Number) {
        throw std::runtime_error("startxref not followed by an offset");
    }
    return static_cast<std::size_t>(ParseUint32FromNumber(num.bytes));
}

// Helper: consume the ``N M obj`` header of an indirect object,
// leaving the lexer positioned at the object's value (the ``<<`` of
// its dict for an XRef stream).
void SkipObjHeader(Lexer& lex) {
    Token id = NextSignificant(lex);
    Token gen = NextSignificant(lex);
    Token obj = NextSignificant(lex);
    bool ok = id.kind == TokenKind::Number && gen.kind == TokenKind::Number &&
              obj.kind == TokenKind::Keyword &&
              std::string_view(reinterpret_cast<const char*>(obj.bytes.data()),
                               obj.bytes.size()) == "obj";
    if (!ok) {
        throw std::runtime_error(
            "startxref offset does not point at an 'N M obj' header "
            "(expected XRef stream object)");
    }
}

} // namespace

Trailer Parse(std::span<const std::byte> input) {
    // Find startxref in last 4 KiB
    std::size_t startxref_offset = FindStartXrefOffset(input);

    // Find "trailer" keyword before startxref (absent for XRef streams)
    std::optional<std::size_t> trailer_keyword_offset =
        FindTrailerKeywordOffset(input, startxref_offset);

    if (trailer_keyword_offset.has_value()) {
        // Classical trailer (§7.5.5): dict starts right after the keyword.
        std::size_t dict_start = *trailer_keyword_offset + 7; // "trailer".size()
        std::span<const std::byte> trailer_region = input.subspan(dict_start);
        Lexer lex(trailer_region);
        return ParseTrailerDict(lex);
    }

    // XRef-stream trailer (§7.5.8): no "trailer" keyword. startxref
    // points at the cross-reference stream object; its dict carries the
    // trailer fields. Seek there, consume the `N M obj` header, and
    // parse the dict (stopping at `>>`, never reading the stream body).
    std::size_t xref_obj_offset = ReadStartXrefTargetOffset(input, startxref_offset);
    if (xref_obj_offset >= input.size()) {
        throw std::runtime_error("startxref offset out of range");
    }
    Lexer lex(input.subspan(xref_obj_offset));
    SkipObjHeader(lex);
    return ParseTrailerDict(lex);
}

std::string ToCanonical(const Trailer& trailer) {
    std::string out;
    out.reserve(64);

    out += "size ";
    out += std::to_string(trailer.size);
    out += '\n';

    out += "root ";
    out += std::to_string(trailer.root_id);
    out += '\n';

    if (trailer.info_id != 0) {
        out += "info ";
        out += std::to_string(trailer.info_id);
        out += '\n';
    }

    if (trailer.encrypt_id != 0) {
        out += "encrypt ";
        out += std::to_string(trailer.encrypt_id);
        out += '\n';
    }

    return out;
}

} // namespace foundation::trailer
