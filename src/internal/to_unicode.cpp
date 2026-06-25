#include "to_unicode.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Helper forward declarations
std::vector<std::byte> StripComments(std::span<const std::byte> input);
std::vector<std::string> Tokenize(std::span<const std::byte> input);
std::string StripWhitespace(std::string s);
std::vector<std::string> ExtractBlock(const std::vector<std::string>& tokens,
                                      std::size_t start,
                                      const std::string& end);
std::uint32_t ParseHexInt(const std::string& hex, std::size_t width);
std::vector<std::uint32_t> ParseDstUnicode(const std::string& hex);
void ValidateCodepoint(std::uint32_t cp);
std::string ToUppercaseHex(std::uint32_t value, std::size_t width);

struct CMapParser {
    std::vector<std::string> tokens;
    std::size_t pos = 0;

    bool HasMore() const { return pos < tokens.size(); }
    std::string Peek() const { return pos < tokens.size() ? tokens[pos] : ""; }
    std::string Next() { return pos < tokens.size() ? tokens[pos++] : ""; }
    bool Match(const std::string& s) const { return pos < tokens.size() && tokens[pos] == s; }
};

// Strip PostScript-style % comments (to end of line)
std::vector<std::byte> StripComments(std::span<const std::byte> input) {
    std::vector<std::byte> out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::byte b = input[i];
        if (b == std::byte{'%'}) {
            // Skip to end of line
            while (i < input.size() && input[i] != std::byte{'\n'} && input[i] != std::byte{'\r'}) {
                ++i;
            }
            // Preserve the newline (or CR) if present
            if (i < input.size()) {
                out.push_back(input[i]);
            }
        } else {
            out.push_back(b);
        }
    }
    return out;
}

// Tokenize CMap: split on whitespace, preserve angle brackets as tokens
std::vector<std::string> Tokenize(std::span<const std::byte> input) {
    std::vector<std::string> tokens;
    std::string current;
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::byte b = input[i];
        char c = static_cast<char>(static_cast<unsigned char>(b));
        if (std::isspace(c)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else if (c == '<' || c == '>') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string StripWhitespace(std::string s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

// Collect the tokens of one keyword-delimited block, starting at `start`
// (the first token *after* the begin keyword the caller already consumed)
// and ending just before the next `end` keyword. Extracting from the
// caller's current position — rather than rescanning from the front — is
// what lets the parse loops pick up the second, third, ... block of a
// given kind; a CMap may legitimately split its bfchar/bfrange entries
// across several blocks (PDF 32000 §9.10.3).
std::vector<std::string> ExtractBlock(const std::vector<std::string>& tokens,
                                      std::size_t start,
                                      const std::string& end) {
    std::vector<std::string> block;
    for (std::size_t i = start; i < tokens.size() && tokens[i] != end; ++i) {
        block.push_back(tokens[i]);
    }
    return block;
}

// Parse hex string to uint32_t; width is number of hex digits expected
std::uint32_t ParseHexInt(const std::string& hex, std::size_t width) {
    if (hex.size() != width) {
        throw std::runtime_error("hex token width mismatch");
    }
    std::uint32_t value = 0;
    for (char c : hex) {
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            value |= (c - 'A' + 10);
        } else {
            throw std::runtime_error("invalid hex character");
        }
    }
    return value;
}

// Parse dst hex string to vector of codepoints (4 hex digits per codepoint)
std::vector<std::uint32_t> ParseDstUnicode(const std::string& hex) {
    if (hex.size() % 4 != 0) {
        throw std::runtime_error("dst hex length not multiple of 4");
    }
    std::vector<std::uint32_t> cps;
    for (std::size_t i = 0; i < hex.size(); i += 4) {
        std::uint32_t cp = ParseHexInt(hex.substr(i, 4), 4);
        cps.push_back(cp);
    }
    return cps;
}

void ValidateCodepoint(std::uint32_t cp) {
    if (cp > 0xFFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        throw std::runtime_error("codepoint outside BMP or is surrogate");
    }
}

std::string ToUppercaseHex(std::uint32_t value, std::size_t width) {
    std::string s(width, '0');
    for (std::size_t i = 0; i < width; ++i) {
        std::size_t idx = width - 1 - i;
        std::uint32_t digit = value & 0xF;
        s[idx] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value >>= 4;
    }
    return s;
}

// Parse a single hex string token (strip whitespace inside < >)
std::string ParseHexString(const std::string& token) {
    // token should be "<...>"
    if (token.size() < 2 || token.front() != '<' || token.back() != '>') {
        throw std::runtime_error("not a hex string token");
    }
    std::string inner = token.substr(1, token.size() - 2);
    std::string result;
    for (char c : inner) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result.push_back(c);
        }
    }
    return result;
}

// Parse hex string from token list (handles < > delimiters)
std::string ParseHexStringFromTokens(CMapParser& parser) {
    if (!parser.HasMore() || parser.Peek() != "<") {
        throw std::runtime_error("expected hex string");
    }
    parser.Next(); // consume '<'
    std::string result;
    while (parser.HasMore() && parser.Peek() != ">") {
        std::string token = parser.Next();
        for (char c : token) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                result.push_back(c);
            }
        }
    }
    if (!parser.HasMore() || parser.Next() != ">") {
        throw std::runtime_error("unclosed hex string");
    }
    return result;
}

// Parse array of hex strings: [<hex> <hex> ...]
std::vector<std::string> ParseHexStringArray(CMapParser& parser) {
    if (!parser.HasMore() || parser.Peek() != "[") {
        throw std::runtime_error("expected array");
    }
    parser.Next(); // consume '['
    std::vector<std::string> result;
    while (parser.HasMore() && parser.Peek() != "]") {
        if (parser.Peek() == "<") {
            result.push_back(ParseHexStringFromTokens(parser));
        } else {
            throw std::runtime_error("expected hex string in array");
        }
    }
    if (!parser.HasMore() || parser.Next() != "]") {
        throw std::runtime_error("unclosed array");
    }
    return result;
}

} // namespace

namespace foundation::to_unicode {

ToUnicode Parse(std::span<const std::byte> input) {
    // Strip comments first
    std::vector<std::byte> stripped = StripComments(input);
    
    // Tokenize
    std::vector<std::string> tokens = Tokenize(stripped);
    
    CMapParser parser;
    parser.tokens = tokens;
    
    ToUnicode cmap;
    
    // Parse codespace ranges
    while (parser.HasMore()) {
        if (parser.Match("begincodespacerange")) {
            parser.Next(); // consume 'begincodespacerange'
            std::vector<std::string> block = ExtractBlock(parser.tokens, parser.pos, "endcodespacerange");
            CMapParser blockParser;
            blockParser.tokens = block;
            blockParser.pos = 0;
            
            while (blockParser.HasMore()) {
                if (blockParser.Peek() == "endcodespacerange") {
                    break;
                }
                if (blockParser.Peek() != "<") {
                    throw std::runtime_error("expected hex string in codespacerange");
                }
                std::string startHex = ParseHexStringFromTokens(blockParser);
                std::string endHex = ParseHexStringFromTokens(blockParser);
                
                if (startHex.size() != endHex.size()) {
                    throw std::runtime_error("codespacerange start/end hex widths mismatch");
                }
                if (startHex.size() % 2 != 0) {
                    throw std::runtime_error("codespacerange hex width must be even");
                }
                
                std::uint8_t byte_count = static_cast<std::uint8_t>(startHex.size() / 2);
                std::uint32_t start = ParseHexInt(startHex, startHex.size());
                std::uint32_t end = ParseHexInt(endHex, endHex.size());
                
                cmap.codespace.push_back({byte_count, start, end});
            }
            
            // Advance parser past the block
            while (parser.HasMore() && parser.Peek() != "endcodespacerange") {
                parser.Next();
            }
            if (parser.HasMore()) {
                parser.Next(); // consume 'endcodespacerange'
            }
        } else {
            parser.Next();
        }
    }
    
    // Reset parser for second pass
    parser.tokens = tokens;
    parser.pos = 0;
    
    // Parse bfchar blocks
    while (parser.HasMore()) {
        if (parser.Match("beginbfchar")) {
            parser.Next(); // consume 'beginbfchar'
            std::vector<std::string> block = ExtractBlock(parser.tokens, parser.pos, "endbfchar");
            CMapParser blockParser;
            blockParser.tokens = block;
            blockParser.pos = 0;
            
            while (blockParser.HasMore()) {
                if (blockParser.Peek() == "endbfchar") {
                    break;
                }
                if (blockParser.Peek() != "<") {
                    throw std::runtime_error("expected hex string in bfchar");
                }
                std::string srcHex = ParseHexStringFromTokens(blockParser);
                std::string dstHex = ParseHexStringFromTokens(blockParser);
                
                if (srcHex.size() % 2 != 0) {
                    throw std::runtime_error("bfchar src hex width must be even");
                }
                
                std::uint8_t byte_count = static_cast<std::uint8_t>(srcHex.size() / 2);
                std::uint32_t char_code = ParseHexInt(srcHex, srcHex.size());
                std::vector<std::uint32_t> unicode = ParseDstUnicode(dstHex);
                
                for (std::uint32_t cp : unicode) {
                    ValidateCodepoint(cp);
                }
                
                // Insert or overwrite mapping
                Mapping mapping{byte_count, char_code, unicode};
                auto it = std::find_if(cmap.mappings.begin(), cmap.mappings.end(),
                                       [&](const Mapping& m) { return m.char_code == char_code; });
                if (it != cmap.mappings.end()) {
                    *it = mapping;
                } else {
                    cmap.mappings.push_back(mapping);
                }
            }
            
            // Advance parser past the block
            while (parser.HasMore() && parser.Peek() != "endbfchar") {
                parser.Next();
            }
            if (parser.HasMore()) {
                parser.Next(); // consume 'endbfchar'
            }
        } else {
            parser.Next();
        }
    }
    
    // Reset parser for third pass
    parser.tokens = tokens;
    parser.pos = 0;
    
    // Parse bfrange blocks
    while (parser.HasMore()) {
        if (parser.Match("beginbfrange")) {
            parser.Next(); // consume 'beginbfrange'
            std::vector<std::string> block = ExtractBlock(parser.tokens, parser.pos, "endbfrange");
            CMapParser blockParser;
            blockParser.tokens = block;
            blockParser.pos = 0;
            
            while (blockParser.HasMore()) {
                if (blockParser.Peek() == "endbfrange") {
                    break;
                }
                if (blockParser.Peek() != "<") {
                    throw std::runtime_error("expected hex string in bfrange");
                }
                std::string startHex = ParseHexStringFromTokens(blockParser);
                std::string endHex = ParseHexStringFromTokens(blockParser);
                
                if (startHex.size() != endHex.size()) {
                    throw std::runtime_error("bfrange start/end hex widths mismatch");
                }
                if (startHex.size() % 2 != 0) {
                    throw std::runtime_error("bfrange src hex width must be even");
                }
                
                std::uint8_t byte_count = static_cast<std::uint8_t>(startHex.size() / 2);
                std::uint32_t start = ParseHexInt(startHex, startHex.size());
                std::uint32_t end = ParseHexInt(endHex, endHex.size());
                
                if (end < start) {
                    throw std::runtime_error("bfrange end < start");
                }
                
                // Next token should be < or [
                if (!blockParser.HasMore()) {
                    throw std::runtime_error("expected dst in bfrange");
                }
                
                if (blockParser.Peek() == "<") {
                    // Single form
                    std::string dstHex = ParseHexStringFromTokens(blockParser);
                    std::vector<std::uint32_t> dst = ParseDstUnicode(dstHex);
                    
                    if (dst.empty()) {
                        throw std::runtime_error("bfrange dst empty");
                    }
                    
                    // Increment last code unit across range
                    for (std::uint32_t offset = 0; offset <= end - start; ++offset) {
                        std::vector<std::uint32_t> result = dst;
                        std::uint32_t& last = result.back();
                        last += offset;
                        if (last > 0xFFFF) {
                            throw std::runtime_error("bfrange single-dst overflow past 0xFFFF");
                        }
                        for (std::uint32_t cp : result) {
                            ValidateCodepoint(cp);
                        }
                        
                        Mapping mapping{byte_count, static_cast<std::uint32_t>(start + offset), result};
                        auto it = std::find_if(cmap.mappings.begin(), cmap.mappings.end(),
                                               [&](const Mapping& m) { return m.char_code == start + offset; });
                        if (it != cmap.mappings.end()) {
                            *it = mapping;
                        } else {
                            cmap.mappings.push_back(mapping);
                        }
                    }
                } else if (blockParser.Peek() == "[") {
                    // Array form
                    std::vector<std::string> dstArray = ParseHexStringArray(blockParser);
                    std::size_t expected_len = end - start + 1;
                    if (dstArray.size() != expected_len) {
                        throw std::runtime_error("bfrange array length != end - start + 1");
                    }
                    
                    for (std::size_t i = 0; i < expected_len; ++i) {
                        std::vector<std::uint32_t> unicode = ParseDstUnicode(dstArray[i]);
                        for (std::uint32_t cp : unicode) {
                            ValidateCodepoint(cp);
                        }
                        
                        Mapping mapping{byte_count, static_cast<std::uint32_t>(start + i), unicode};
                        auto it = std::find_if(cmap.mappings.begin(), cmap.mappings.end(),
                                               [&](const Mapping& m) { return m.char_code == start + i; });
                        if (it != cmap.mappings.end()) {
                            *it = mapping;
                        } else {
                            cmap.mappings.push_back(mapping);
                        }
                    }
                } else {
                    throw std::runtime_error("expected hex string or array in bfrange");
                }
            }
            
            // Advance parser past the block
            while (parser.HasMore() && parser.Peek() != "endbfrange") {
                parser.Next();
            }
            if (parser.HasMore()) {
                parser.Next(); // consume 'endbfrange'
            }
        } else {
            parser.Next();
        }
    }
    
    // Sort mappings by char_code
    std::sort(cmap.mappings.begin(), cmap.mappings.end(),
              [](const Mapping& a, const Mapping& b) {
                  return a.char_code < b.char_code;
              });
    
    return cmap;
}

std::string ToCanonical(const ToUnicode& cmap) {
    if (cmap.codespace.empty()) {
        // No codespace entries, use default width of 1
        throw std::runtime_error("no codespace entries found");
    }
    
    // Find max byte_count across all codespace entries
    std::uint8_t max_byte_count = 0;
    for (const auto& cs : cmap.codespace) {
        if (cs.byte_count > max_byte_count) {
            max_byte_count = cs.byte_count;
        }
    }
    
    std::string result;
    
    // Sort codespace entries by start
    std::vector<CodespaceRange> sorted_codespace = cmap.codespace;
    std::sort(sorted_codespace.begin(), sorted_codespace.end(),
              [](const CodespaceRange& a, const CodespaceRange& b) {
                  return a.start < b.start;
              });
    
    // Output codespace lines
    for (const auto& cs : sorted_codespace) {
        std::string startHex = ToUppercaseHex(cs.start, 2 * cs.byte_count);
        std::string endHex = ToUppercaseHex(cs.end, 2 * cs.byte_count);
        result += "codespace " + std::to_string(cs.byte_count) + " " + startHex + " " + endHex + "\n";
    }
    
    // Sort mappings by char_code (should already be sorted, but ensure)
    std::vector<Mapping> sorted_mappings = cmap.mappings;
    std::sort(sorted_mappings.begin(), sorted_mappings.end(),
              [](const Mapping& a, const Mapping& b) {
                  return a.char_code < b.char_code;
              });
    
    // Output bf lines
    for (const auto& m : sorted_mappings) {
        std::string charHex = ToUppercaseHex(m.char_code, 2 * max_byte_count);
        std::string line = "bf " + std::to_string(m.byte_count) + " " + charHex;
        for (std::uint32_t cp : m.unicode) {
            line += " " + ToUppercaseHex(cp, 4);
        }
        result += line + "\n";
    }
    
    return result;
}

} // namespace foundation::to_unicode
