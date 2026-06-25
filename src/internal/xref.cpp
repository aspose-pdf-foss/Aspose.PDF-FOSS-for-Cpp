#include "xref.hpp"

#include "flate.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <optional>
#include <stdexcept>
// std::to_string — MSVC's STL doesn't pull <string> in
// transitively (libstdc++/libc++ happen to). Spell it
// explicitly for toolchain portability.
#include <string>
#include <string_view>
#include <vector>

namespace {

using foundation::Token;
using foundation::TokenKind;

// Helper: skip whitespace tokens, return next non-whitespace token or Eof.
Token SkipWhitespace(foundation::Lexer& lex) {
    Token t = lex.Next();
    while (t.kind == TokenKind::Whitespace || t.kind == TokenKind::Eol) {
        t = lex.Next();
    }
    return t;
}

// Helper: parse unsigned 32-bit integer from token bytes. Throws on parse error.
std::uint32_t ParseUint32(const Token& t) {
    if (t.kind != TokenKind::Number) {
        throw std::runtime_error("xref: expected number token");
    }
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    std::uint32_t out = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        throw std::runtime_error("xref: invalid integer");
    }
    return out;
}

// Helper: parse unsigned 64-bit integer from token bytes. Throws on parse error.
std::uint64_t ParseUint64(const Token& t) {
    if (t.kind != TokenKind::Number) {
        throw std::runtime_error("xref: expected number token");
    }
    std::string_view sv(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
    std::uint64_t out = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
        throw std::runtime_error("xref: invalid integer");
    }
    return out;
}

// Helper: find last occurrence of 'startxref' keyword in last ~1 KiB of input.
// Returns offset of the integer that follows 'startxref', or throws.
std::size_t FindStartXrefOffset(std::span<const std::byte> input) {
    constexpr std::size_t kScanWindow = 1024;
    std::size_t scan_start = (input.size() > kScanWindow) ? (input.size() - kScanWindow) : 0;

    // Look for "startxref" (case-sensitive per spec)
    std::string_view keyword = "startxref";
    std::size_t last_pos = std::string::npos;
    for (std::size_t i = scan_start; i + keyword.size() <= input.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < keyword.size(); ++j) {
            if (static_cast<char>(input[i + j]) != keyword[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            last_pos = i;
        }
    }

    if (last_pos == std::string::npos) {
        throw std::runtime_error("xref: startxref not found");
    }

    // Advance past keyword
    std::size_t pos = last_pos + keyword.size();

    // Skip whitespace until we hit a digit (start of integer)
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++pos;
        } else {
            break;
        }
    }

    if (pos >= input.size()) {
        throw std::runtime_error("xref: startxref followed by no integer");
    }

    // Scan integer
    std::size_t start = pos;
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c >= '0' && c <= '9') {
            ++pos;
        } else {
            break;
        }
    }

    if (pos == start) {
        throw std::runtime_error("xref: startxref followed by no integer");
    }

    // Return the offset of the integer (where parsing should resume)
    return start;
}

// Helper: parse one xref entry line. The PDF 32000-1:2008 §7.5.4
// entry format is fixed-width: 10-digit offset (or next-free id for
// free entries), 5-digit generation, single-char status 'n' or 'f',
// two trailing bytes (space + EOL). There is NO per-row object id —
// the id is ``base_id`` supplied by the caller (subsection start_id
// + row index).
//
// A prior attempt read a leading "object number" field; that field
// does not exist in the spec. The mistake manifested as the entry
// parser consuming the offset as an id, the generation as an offset,
// the status as a generation, and then failing on the trailing
// whitespace.
std::optional<foundation::xref::Entry> ParseXrefEntry(foundation::Lexer& lex, std::uint32_t base_id) {
    // First token: byte offset (10 digits).
    Token offset_tok = SkipWhitespace(lex);
    if (offset_tok.kind != TokenKind::Number) {
        throw std::runtime_error("xref: entry: expected byte offset");
    }
    std::uint64_t offset = ParseUint64(offset_tok);

    // Second token: generation number (5 digits).
    Token gen_tok = SkipWhitespace(lex);
    if (gen_tok.kind != TokenKind::Number) {
        throw std::runtime_error("xref: entry: expected generation number");
    }
    std::uint16_t generation = static_cast<std::uint16_t>(ParseUint32(gen_tok));

    // Third token: single-char status 'n' or 'f'. The lexer
    // emits it as a one-byte Keyword token.
    Token status_tok = SkipWhitespace(lex);
    if (status_tok.kind != TokenKind::Keyword) {
        throw std::runtime_error("xref: entry: expected 'n' or 'f'");
    }
    if (status_tok.bytes.size() != 1) {
        throw std::runtime_error("xref: entry: status token must be single char");
    }
    char status = static_cast<char>(status_tok.bytes[0]);
    std::uint32_t id = base_id;

    // Consume EOL
    Token eol = lex.Next();
    // The lexer merges EOLs into its Whitespace token class
    // — TokenKind::Eol is declared in the header but never produced
    // by the current implementation. Accept either so this code
    // keeps working whether a future lexer revision splits them.
    if (eol.kind != TokenKind::Eol && eol.kind != TokenKind::Whitespace) {
        throw std::runtime_error("xref: expected EOL after xref entry");
    }

    if (status == 'f') {
        // Free entry — caller drops these via std::nullopt. A prior
        // attempt returned a default-constructed Entry here, but
        // EntryKind::InUse is the first enum value and therefore
        // the zero-init default — the "skip frees" caller check
        // (kind != InUse) was always false and free entries leaked
        // straight through to the canonical output.
        return std::nullopt;
    } else if (status == 'n') {
        return foundation::xref::Entry{
            id, generation,
            foundation::xref::EntryKind::InUse, offset, 0};
    } else {
        throw std::runtime_error("xref: entry: invalid status char");
    }
}

// Helper: parse a classical xref section starting at current lex position.
// Assumes lex is positioned after the 'xref' keyword.
//
// A prior approach tried to peek-then-parse by calling SkipWhitespace
// to look at the first token and then lex.Next() twice more. The
// lexer has no peek/unnext, so those two extra Next() calls dropped
// the start_id and the first entry on the floor. This rewrite treats
// the token returned by SkipWhitespace as authoritative: if it's a
// Keyword "trailer", we're done; otherwise it IS the subsection's
// start_id and we go straight to reading the count and entries.
foundation::xref::Table ParseClassicalXref(foundation::Lexer& lex) {
    foundation::xref::Table table;

    while (true) {
        Token first = SkipWhitespace(lex);
        if (first.kind == TokenKind::Keyword) {
            std::string_view kw(reinterpret_cast<const char*>(first.bytes.data()),
                                first.bytes.size());
            if (kw == "trailer") {
                break;
            }
            throw std::runtime_error(
                "xref: classical section: unexpected keyword");
        }
        if (first.kind != TokenKind::Number) {
            throw std::runtime_error(
                "xref: classical section: expected subsection start_id");
        }

        std::uint32_t start_id = ParseUint32(first);

        Token count_tok = SkipWhitespace(lex);
        if (count_tok.kind != TokenKind::Number) {
            throw std::runtime_error(
                "xref: classical section: expected subsection count");
        }
        std::uint32_t count = ParseUint32(count_tok);

        for (std::uint32_t i = 0; i < count; ++i) {
            auto entry = ParseXrefEntry(lex, start_id + i);
            if (entry.has_value()) {
                table.entries.push_back(*entry);
            }
        }
    }

    return table;
}

// Parse a classical trailer dictionary: capture /Root, /Info, /Size and return
// /Prev (0 if absent). Values may be compound (`1 0 R`, arrays); we read only
// the direct forms the trailer uses for these keys.
std::uint64_t ParseTrailerFields(foundation::Lexer& lex,
                                 foundation::xref::Table& table) {
    Token t = SkipWhitespace(lex);
    if (t.kind != TokenKind::DictOpen) {
        throw std::runtime_error("xref: trailer: expected dictionary");
    }
    int depth = 1;
    std::uint64_t prev = 0;
    while (depth > 0) {
        Token tok = SkipWhitespace(lex);
        if (tok.kind == TokenKind::Eof) {
            throw std::runtime_error("xref: trailer: unterminated dictionary");
        }
        if (tok.kind == TokenKind::DictOpen)  { ++depth; continue; }
        if (tok.kind == TokenKind::DictClose) { --depth; continue; }
        if (depth != 1 || tok.kind != TokenKind::Name) continue;
        if (tok.bytes.size() < 2 || tok.bytes[0] != std::byte{'/'}) continue;
        std::string_view name(
            reinterpret_cast<const char*>(tok.bytes.data()) + 1,
            tok.bytes.size() - 1);

        if (name == "Prev") {
            Token v = SkipWhitespace(lex);
            if (v.kind == TokenKind::Number) prev = ParseUint64(v);
        } else if (name == "Size") {
            Token v = SkipWhitespace(lex);
            if (v.kind == TokenKind::Number) {
                std::uint64_t s = ParseUint64(v);
                if (s > table.size) table.size = s;
            }
        } else if (name == "Root" || name == "Info") {
            // <id> <gen> R
            Token id = SkipWhitespace(lex);
            Token gen = SkipWhitespace(lex);
            Token r = SkipWhitespace(lex);
            const bool ok = id.kind == TokenKind::Number &&
                            gen.kind == TokenKind::Number &&
                            r.kind == TokenKind::Keyword &&
                            std::string_view(reinterpret_cast<const char*>(r.bytes.data()),
                                             r.bytes.size()) == "R";
            if (ok) {
                std::uint32_t oid = ParseUint32(id);
                std::uint16_t ogen = static_cast<std::uint16_t>(ParseUint64(gen));
                if (name == "Root" && table.root_id == 0) {
                    table.root_id = oid; table.root_gen = ogen;
                } else if (name == "Info" && table.info_id == 0) {
                    table.info_id = oid; table.info_gen = ogen;
                }
            }
        }
    }
    return prev;
}

// Helper: detect if section at offset is classical xref or /XRef stream.
// Returns true if /XRef stream, false if classical.
bool DetectXrefStream(std::span<const std::byte> input, std::size_t offset) {
    // Skip whitespace and comments until we hit a non-trivia token.
    std::size_t pos = offset;
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '%') {
            if (c == '%') {
                // Skip comment to EOL
                while (pos < input.size() && static_cast<char>(input[pos]) != '\n') {
                    ++pos;
                }
                if (pos < input.size()) ++pos;
            } else {
                ++pos;
            }
        } else {
            break;
        }
    }

    // Check for "xref\n" or "xref\r\n"
    if (pos + 4 <= input.size()) {
        if (input[pos] == std::byte{'x'} &&
            input[pos + 1] == std::byte{'r'} &&
            input[pos + 2] == std::byte{'e'} &&
            input[pos + 3] == std::byte{'f'}) {
            return false; // classical xref
        }
    }

    // Check for object declaration: "<digits> <digits> obj"
    // Read first number
    std::size_t start = pos;
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c >= '0' && c <= '9') {
            ++pos;
        } else {
            break;
        }
    }
    if (pos == start || pos >= input.size()) return true; // assume stream if no number

    // Skip whitespace
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++pos;
        } else {
            break;
        }
    }

    // Read second number
    start = pos;
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c >= '0' && c <= '9') {
            ++pos;
        } else {
            break;
        }
    }
    if (pos == start || pos >= input.size()) return true;

    // Skip whitespace
    while (pos < input.size()) {
        char c = static_cast<char>(input[pos]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++pos;
        } else {
            break;
        }
    }

    // Check for "obj"
    if (pos + 3 <= input.size()) {
        if (input[pos] == std::byte{'o'} &&
            input[pos + 1] == std::byte{'b'} &&
            input[pos + 2] == std::byte{'j'}) {
            return true; // /XRef stream object
        }
    }

    return false;
}

// Byte-level /XRef stream parser. Returns (Table, prev_offset).
//
// Why a hand-rolled byte scanner instead of the lexer path: an xref
// stream object interleaves a PDF dictionary (which the lexer
// handles fine) with a raw binary payload (which the lexer
// misclassifies — most binary bytes fall into the lexer's
// Whitespace/Keyword branches and get fragmented). Byte scanning
// sidesteps the mismatch and is only ~80 lines; the alternative
// was patching the dictionary parser to handle compound values
// AND somehow switching the lexer off for the stream body, which
// is strictly more code for no correctness benefit.
std::pair<foundation::xref::Table, std::uint64_t>
ParseXrefStreamByBytes(std::span<const std::byte> input, std::size_t offset);

// Helper: parse a section at given offset (classical or stream).
// Returns (Table, prev_offset).
std::pair<foundation::xref::Table, std::uint64_t> ParseSectionAtOffset(std::span<const std::byte> input, std::size_t offset) {
    // Start the lexer at exactly the section offset via a subspan.
    // A previous approach used ``while (lex.Next().offset < offset)``
    // to advance a full-file lexer, but Next() has already consumed
    // whichever token sits at ``offset`` by the time the loop exits
    // — the caller's first ``lex.Next()`` then returns the token
    // AFTER the section header (whitespace or the next keyword).
    // Subspan is the cleaner fix: the resulting lexer's token
    // offsets are relative to the subspan, which is fine because
    // nothing downstream compares them to file-absolute offsets.
    if (offset > input.size()) {
        throw std::runtime_error("xref: section offset beyond input");
    }
    foundation::Lexer lex(input.subspan(offset));

    if (DetectXrefStream(input, offset)) {
        // /XRef streams are byte-scanned, not lexer-tokenised. The
        // stream body is arbitrary binary (compressed xref entries)
        // and the lexer's token classes (Name, Number, …)
        // cut through it unpredictably — a zero byte is
        // whitespace per §7.2.3, so a FlateDecode payload with
        // any NUL splits into dozens of "tokens." Parsing the
        // dictionary inline by bytes is also simpler than patching
        // the LLM-written ParseDict to handle compound values like
        // ``/Root 1 0 R`` and ``/W [1 3 1]``.
        return ParseXrefStreamByBytes(input, offset);
    } else {
        // Classical xref
        Token t = lex.Next();
        if (t.kind != TokenKind::Keyword) {
            throw std::runtime_error("xref: classical xref: expected 'xref' keyword");
        }
        std::string_view kw(reinterpret_cast<const char*>(t.bytes.data()), t.bytes.size());
        if (kw != "xref") {
            throw std::runtime_error("xref: classical xref: expected 'xref'");
        }

        foundation::xref::Table table = ParseClassicalXref(lex);

        // Now parse trailer dictionary to get /Prev and capture trailer fields
        std::uint64_t prev = ParseTrailerFields(lex, table);

        return {table, prev};
    }
}

// Byte-level /XRef stream implementation. The caller guarantees
// ``offset`` points at an object declaration of the form
// ``<id> <gen> obj ... stream ... endstream endobj`` whose
// dictionary has /Type /XRef.
std::pair<foundation::xref::Table, std::uint64_t>
ParseXrefStreamByBytes(std::span<const std::byte> input, std::size_t offset) {
    auto as_char = [](std::byte b) {
        return static_cast<char>(b);
    };
    auto is_digit = [&](std::size_t p) {
        return p < input.size() && as_char(input[p]) >= '0'
               && as_char(input[p]) <= '9';
    };
    auto is_ws = [&](std::size_t p) {
        if (p >= input.size()) return false;
        char c = as_char(input[p]);
        return c == ' ' || c == '\t' || c == '\r' || c == '\n'
               || c == '\f' || c == '\0';
    };
    auto skip_ws = [&](std::size_t p) {
        while (p < input.size() && is_ws(p)) ++p;
        return p;
    };
    auto read_uint = [&](std::size_t p, std::uint64_t& out) {
        std::size_t start = p;
        out = 0;
        while (is_digit(p)) {
            out = out * 10
                  + static_cast<std::uint64_t>(as_char(input[p]) - '0');
            ++p;
        }
        return p > start ? p : std::string::npos;
    };
    auto match = [&](std::size_t p, std::string_view s) {
        if (p + s.size() > input.size()) return false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (as_char(input[p + i]) != s[i]) return false;
        }
        return true;
    };
    auto find = [&](std::size_t start, std::string_view needle,
                    std::size_t end) {
        if (end > input.size()) end = input.size();
        if (needle.empty() || needle.size() > end - start) {
            return std::string::npos;
        }
        for (std::size_t p = start; p + needle.size() <= end; ++p) {
            if (match(p, needle)) return p;
        }
        return std::string::npos;
    };

    // Object header: "<id> <gen> obj". We don't need id/gen — just
    // advance past the header.
    std::size_t pos = skip_ws(offset);
    std::uint64_t dummy = 0;
    pos = read_uint(pos, dummy);
    if (pos == std::string::npos) {
        throw std::runtime_error("xref: /XRef stream: bad object id");
    }
    pos = skip_ws(pos);
    pos = read_uint(pos, dummy);
    if (pos == std::string::npos) {
        throw std::runtime_error("xref: /XRef stream: bad generation");
    }
    pos = skip_ws(pos);
    if (!match(pos, "obj")) {
        throw std::runtime_error("xref: /XRef stream: expected 'obj'");
    }
    pos += 3;
    pos = skip_ws(pos);

    // Dictionary: find the outer <<...>> with nesting tracking. The
    // dict body runs from dict_start (just after the outer '<<')
    // to dict_end (just before the matching '>>').
    if (!match(pos, "<<")) {
        throw std::runtime_error("xref: /XRef stream: expected '<<'");
    }
    std::size_t dict_start = pos + 2;
    std::size_t dict_end = std::string::npos;
    int depth = 1;
    std::size_t p = dict_start;
    while (p + 1 < input.size() && depth > 0) {
        if (match(p, "<<")) { ++depth; p += 2; continue; }
        if (match(p, ">>")) {
            --depth;
            if (depth == 0) { dict_end = p; break; }
            p += 2; continue;
        }
        ++p;
    }
    if (dict_end == std::string::npos) {
        throw std::runtime_error("xref: /XRef stream: unterminated dict");
    }

    // Extract simple key/value pairs by bytescan.
    auto dict_find_int = [&](std::string_view key,
                             std::uint64_t& out) {
        std::string search = std::string("/") + std::string(key);
        std::size_t kp = find(dict_start, search, dict_end);
        if (kp == std::string::npos) return false;
        std::size_t ap = kp + search.size();
        // Key must be terminated (not a prefix of another name).
        if (ap < input.size()) {
            char c = as_char(input[ap]);
            if (!(is_ws(ap) || c == '/' || c == '[' || c == '<'
                  || c == '(')) {
                return false;
            }
        }
        ap = skip_ws(ap);
        return read_uint(ap, out) != std::string::npos;
    };

    auto dict_find_name = [&](std::string_view key,
                              std::string& out) {
        std::string search = std::string("/") + std::string(key);
        std::size_t kp = find(dict_start, search, dict_end);
        if (kp == std::string::npos) return false;
        std::size_t ap = skip_ws(kp + search.size());
        if (ap >= input.size() || as_char(input[ap]) != '/') return false;
        ++ap;
        std::size_t start_name = ap;
        while (ap < input.size()) {
            char c = as_char(input[ap]);
            if (is_ws(ap) || c == '/' || c == '[' || c == '<'
                || c == '>' || c == '(' || c == ')') break;
            ++ap;
        }
        out.assign(reinterpret_cast<const char*>(input.data()) + start_name,
                   ap - start_name);
        return true;
    };

    // /W array: find ``/W [a b c]``.
    std::array<std::uint64_t, 3> w = {0, 0, 0};
    {
        std::size_t kp = find(dict_start, "/W", dict_end);
        if (kp == std::string::npos) {
            throw std::runtime_error("xref: /XRef stream: missing /W");
        }
        std::size_t ap = skip_ws(kp + 2);
        if (ap >= input.size() || as_char(input[ap]) != '[') {
            throw std::runtime_error("xref: /XRef stream: /W not an array");
        }
        ++ap;
        for (int i = 0; i < 3; ++i) {
            ap = skip_ws(ap);
            ap = read_uint(ap, w[i]);
            if (ap == std::string::npos) {
                throw std::runtime_error("xref: /XRef stream: /W needs 3 ints");
            }
        }
    }

    std::uint64_t size = 0;
    if (!dict_find_int("Size", size)) {
        throw std::runtime_error("xref: /XRef stream: missing /Size");
    }
    std::uint64_t prev = 0;
    dict_find_int("Prev", prev);

    // Indirect-ref finder: /Key <id> <gen> R  (mirrors dict_find_int).
    auto dict_find_ref = [&](std::string_view key,
                             std::uint32_t& id, std::uint16_t& gen) -> bool {
        std::string search = std::string("/") + std::string(key);
        std::size_t kp = find(dict_start, search, dict_end);
        if (kp == std::string::npos) return false;
        std::size_t kend = kp + search.size();
        // Key must be terminated (not a prefix of another name like /RootXxx),
        // mirroring dict_find_int.
        if (kend < input.size()) {
            char c = as_char(input[kend]);
            if (!(is_ws(kend) || c == '/' || c == '[' || c == '<' || c == '(')) {
                return false;
            }
        }
        std::uint64_t a = 0, b = 0;
        std::size_t ap = read_uint(skip_ws(kend), a);
        if (ap == std::string::npos) return false;
        ap = read_uint(skip_ws(ap), b);
        if (ap == std::string::npos) return false;
        std::size_t rp = skip_ws(ap);
        if (rp >= input.size() || as_char(input[rp]) != 'R') return false;
        id = static_cast<std::uint32_t>(a);
        gen = static_cast<std::uint16_t>(b);
        return true;
    };

    std::string filter;
    dict_find_name("Filter", filter);

    std::uint64_t length = 0;
    const bool has_length = dict_find_int("Length", length);

    // Stream body: between "stream\n" and "endstream". Per
    // §7.3.8.1, the "stream" keyword is followed by a single EOL
    // (either LF or CR+LF).
    std::size_t stream_kw = find(dict_end + 2, "stream", input.size());
    if (stream_kw == std::string::npos) {
        throw std::runtime_error("xref: /XRef stream: no 'stream'");
    }
    std::size_t body_start = stream_kw + 6;
    if (body_start < input.size() && as_char(input[body_start]) == '\r') {
        ++body_start;
    }
    if (body_start < input.size() && as_char(input[body_start]) == '\n') {
        ++body_start;
    }
    std::size_t end_kw = find(body_start, "endstream", input.size());
    if (end_kw == std::string::npos) {
        throw std::runtime_error("xref: /XRef stream: no 'endstream'");
    }
    // Body end: /Length (§7.3.8.2) is authoritative. Scanning to
    // "endstream" and trimming trailing whitespace is wrong when the
    // encoded data legitimately ends in a whitespace-valued byte (a
    // FlateDecode stream commonly ends in 0x0A) — the trim then eats a
    // real byte and truncates the stream. Trust a direct /Length when
    // it lands at body_start..body_end with "endstream" right after
    // (allowing one optional EOL); otherwise fall back to the
    // scan-and-trim heuristic.
    std::size_t body_end = end_kw;
    bool bounded_by_length = false;
    if (has_length && length > 0
            && body_start + length <= end_kw) {
        std::size_t cand_end = body_start + length;
        std::size_t gap = cand_end;
        if (gap < input.size() && as_char(input[gap]) == '\r') ++gap;
        if (gap < input.size() && as_char(input[gap]) == '\n') ++gap;
        if (gap == end_kw) {
            body_end = cand_end;
            bounded_by_length = true;
        }
    }
    if (!bounded_by_length) {
        while (body_end > body_start && is_ws(body_end - 1)) {
            --body_end;
        }
    }

    std::vector<std::byte> raw(
        input.begin() + body_start, input.begin() + body_end);

    if (filter == "FlateDecode") {
        try {
            raw = foundation::flate::Decode(raw);
        } catch (const std::exception&) {
            throw std::runtime_error(
                "xref: /XRef stream: flate decode failed");
        }
    } else if (!filter.empty()) {
        throw std::runtime_error(
            "xref: /XRef stream: unsupported filter");
    }

    // Reverse a PNG predictor if /DecodeParms declared one. Cross-
    // reference streams commonly carry /Predictor 12 (PNG "Up") with
    // /Columns == w0+w1+w2, so each FlateDecoded row is prefixed by a
    // 1-byte PNG filter tag (§7.4.4.4 / RFC 2083 §6) that must be
    // undone before the rows are fixed-width entries. xref streams use
    // the default Colors=1 / BitsPerComponent=8, i.e. 1 byte/pixel.
    std::uint64_t predictor = 0;
    dict_find_int("Predictor", predictor);
    if (predictor >= 2) {
        if (predictor < 10) {
            throw std::runtime_error(
                "xref: /XRef stream: unsupported /Predictor");
        }
        std::uint64_t columns = 0;
        if (!dict_find_int("Columns", columns) || columns == 0) {
            columns = w[0] + w[1] + w[2];
        }
        const std::size_t cols = static_cast<std::size_t>(columns);
        const std::size_t stride = cols + 1;  // +1 PNG filter-tag byte
        const std::size_t bpp = 1;            // Colors=1, BPC=8
        auto paeth = [](int a, int b, int c) {
            int p = a + b - c;
            int pa = p > a ? p - a : a - p;
            int pb = p > b ? p - b : b - p;
            int pc = p > c ? p - c : c - p;
            if (pa <= pb && pa <= pc) return a;
            if (pb <= pc) return b;
            return c;
        };
        std::vector<std::byte> out;
        out.reserve(raw.size());
        std::vector<std::uint8_t> prev(cols, 0);
        std::vector<std::uint8_t> cur(cols, 0);
        for (std::size_t r = 0; r + stride <= raw.size(); r += stride) {
            std::uint8_t ft = static_cast<std::uint8_t>(raw[r]);
            for (std::size_t j = 0; j < cols; ++j) {
                int x = static_cast<std::uint8_t>(raw[r + 1 + j]);
                int a = (j >= bpp) ? cur[j - bpp] : 0;
                int b = prev[j];
                int c = (j >= bpp) ? prev[j - bpp] : 0;
                int recon;
                switch (ft) {
                    case 0: recon = x; break;
                    case 1: recon = x + a; break;
                    case 2: recon = x + b; break;
                    case 3: recon = x + ((a + b) >> 1); break;
                    case 4: recon = x + paeth(a, b, c); break;
                    default:
                        throw std::runtime_error(
                            "xref: /XRef stream: bad PNG filter tag");
                }
                cur[j] = static_cast<std::uint8_t>(recon & 0xFF);
            }
            for (std::size_t j = 0; j < cols; ++j) {
                out.push_back(static_cast<std::byte>(cur[j]));
            }
            prev.swap(cur);
        }
        raw.swap(out);
    }

    // /Index [start count ...] selects which object ids the rows map
    // to (§7.5.8.3). Absent → a single subsection covering [0, Size).
    // Without this, every revision's rows would be misread as ids
    // 0..Size-1 — wrong for any incremental / linearised file whose
    // latest section refreshes only a slice of the id space.
    std::vector<std::pair<std::uint64_t, std::uint64_t>> index;
    {
        std::size_t kp = find(dict_start, "/Index", dict_end);
        if (kp != std::string::npos) {
            std::size_t ap = skip_ws(kp + 6);
            if (ap >= input.size() || as_char(input[ap]) != '[') {
                throw std::runtime_error("xref: /XRef stream: /Index not an array");
            }
            ++ap;
            for (;;) {
                ap = skip_ws(ap);
                if (ap < input.size() && as_char(input[ap]) == ']') break;
                std::uint64_t start = 0;
                std::uint64_t count = 0;
                ap = read_uint(ap, start);
                if (ap == std::string::npos) {
                    throw std::runtime_error("xref: /XRef stream: /Index malformed");
                }
                ap = skip_ws(ap);
                ap = read_uint(ap, count);
                if (ap == std::string::npos) {
                    throw std::runtime_error("xref: /XRef stream: /Index malformed");
                }
                index.emplace_back(start, count);
            }
        } else {
            index.emplace_back(static_cast<std::uint64_t>(0), size);
        }
    }

    // Decode fixed-width entries. Each row is w[0]+w[1]+w[2] bytes,
    // big-endian. Default entry type is 1 (§7.5.8.3) if w[0]==0 —
    // the spec allows it, though none of our fixtures exercise it.
    // Rows are consumed in /Index subsection order, each row mapped to
    // id = subsection_start + offset_within_subsection.
    foundation::xref::Table table;
    const std::size_t row = static_cast<std::size_t>(w[0] + w[1] + w[2]);
    if (row == 0) {
        throw std::runtime_error("xref: /XRef stream: /W rows are zero-width");
    }
    std::size_t rp = 0;
    auto read_be = [&](std::size_t off, std::size_t width) {
        std::uint64_t v = 0;
        for (std::size_t i = 0; i < width; ++i) {
            v = (v << 8) | static_cast<std::uint64_t>(raw[rp + off + i]);
        }
        return v;
    };
    bool out_of_rows = false;
    for (const auto& sub : index) {
        for (std::uint64_t k = 0; k < sub.second; ++k) {
            if (rp + row > raw.size()) { out_of_rows = true; break; }
            std::uint64_t id = sub.first + k;
            std::uint64_t etype = w[0] == 0 ? 1 : read_be(0, w[0]);
            std::uint64_t f2 = read_be(w[0], w[1]);
            std::uint64_t f3 = read_be(w[0] + w[1], w[2]);
            rp += row;

            if (etype == 0) {
                // Free — dropped to match oracle canonical form.
            } else if (etype == 1) {
                table.entries.push_back(foundation::xref::Entry{
                    static_cast<std::uint32_t>(id),
                    static_cast<std::uint16_t>(f3),
                    foundation::xref::EntryKind::InUse, f2, 0});
            } else if (etype == 2) {
                // §7.5.8.2: compressed-object generations are always 0.
                table.entries.push_back(foundation::xref::Entry{
                    static_cast<std::uint32_t>(id),
                    0,
                    foundation::xref::EntryKind::Compressed,
                    f2, static_cast<std::uint32_t>(f3)});
            } else {
                // Unknown type — §7.5.8.3 says "ignore the entry."
            }
        }
        if (out_of_rows) break;
    }

    table.size = size;
    { std::uint32_t id = 0; std::uint16_t g = 0;
      if (dict_find_ref("Root", id, g)) { table.root_id = id; table.root_gen = g; } }
    { std::uint32_t id = 0; std::uint16_t g = 0;
      if (dict_find_ref("Info", id, g)) { table.info_id = id; table.info_gen = g; } }

    return {table, prev};
}

} // namespace

namespace foundation::xref {

// Reconstruct a cross-reference table by scanning the whole file for
// object headers `N G obj` plus the trailer dictionary. This is the
// recovery path used when the startxref-directed parse fails — a stale
// or wrong startxref offset (the offset points mid-object or a few
// bytes off the real `xref`), or a classical table with non-conforming
// entry bytes. qpdf and mutool do the same. Direct objects only:
// objects inside object streams are not recovered (the files that need
// this recovery predate object streams). The latest definition of each
// id (highest file offset) wins, matching incremental-update order.
foundation::xref::Table ReconstructXref(std::span<const std::byte> input) {
    auto ch = [&](std::size_t p) { return static_cast<char>(input[p]); };
    auto is_dig = [&](std::size_t p) {
        return p < input.size() && ch(p) >= '0' && ch(p) <= '9';
    };
    auto is_wsb = [&](std::size_t p) {
        if (p >= input.size()) return false;
        char c = ch(p);
        return c == ' ' || c == '\t' || c == '\r' || c == '\n'
               || c == '\f' || c == '\0';
    };
    auto read_u = [&](std::size_t& p, std::uint64_t& out) {
        std::size_t s = p;
        out = 0;
        while (is_dig(p)) {
            out = out * 10 + static_cast<std::uint64_t>(ch(p) - '0');
            ++p;
        }
        return p > s;
    };
    auto skip_ws = [&](std::size_t& p) {
        while (p < input.size() && is_wsb(p)) ++p;
    };
    auto match_at = [&](std::size_t p, std::string_view s) {
        if (p + s.size() > input.size()) return false;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (ch(p + i) != s[i]) return false;
        }
        return true;
    };

    // Pass 1: object headers `N G obj`. The header must start on a
    // digit that is not itself preceded by a digit (so `120 0 obj` is
    // not also read as `20 0 obj`). A header-match attempt that fails
    // partway must NOT skip ahead to where it failed — a real header
    // can start one byte later (e.g. the `%PDF-1.4` version digit `4`
    // immediately followed by `1 0 obj` makes a bogus partial match
    // that would otherwise swallow object 1). So advance by exactly one
    // byte on any failure (the for-loop ++p), and jump past the keyword
    // only on a confirmed match.
    std::map<std::uint32_t, foundation::xref::Entry> by_id;
    for (std::size_t p = 0; p < input.size(); ++p) {
        if (!is_dig(p) || (p > 0 && is_dig(p - 1))) continue;
        std::size_t q = p;
        std::uint64_t id = 0, gen = 0;
        if (!read_u(q, id)) continue;
        if (!is_wsb(q)) continue;
        skip_ws(q);
        if (!read_u(q, gen)) continue;
        if (!is_wsb(q)) continue;
        skip_ws(q);
        if (!match_at(q, "obj")) continue;
        by_id[static_cast<std::uint32_t>(id)] = foundation::xref::Entry{
            static_cast<std::uint32_t>(id),
            static_cast<std::uint16_t>(gen),
            foundation::xref::EntryKind::InUse,
            static_cast<std::uint64_t>(p), 0};  // last (latest) wins
        p = q + 2;  // ++p lands just past "obj"
    }

    // Within [start,end): read `/Key <id> <gen> R` into id/gen.
    auto find_ref = [&](std::size_t start, std::size_t end,
                        std::string_view key,
                        std::uint32_t& id, std::uint16_t& gen) {
        for (std::size_t p = start; p + key.size() <= end; ++p) {
            if (!match_at(p, key)) continue;
            std::size_t q = p + key.size();
            std::uint64_t a = 0, b = 0;
            skip_ws(q);
            if (!read_u(q, a)) continue;
            skip_ws(q);
            if (!read_u(q, b)) continue;
            skip_ws(q);
            if (q < input.size() && ch(q) == 'R') {
                id = static_cast<std::uint32_t>(a);
                gen = static_cast<std::uint16_t>(b);
                return true;
            }
        }
        return false;
    };
    auto find_int = [&](std::size_t start, std::size_t end,
                        std::string_view key, std::uint64_t& out) {
        for (std::size_t p = start; p + key.size() <= end; ++p) {
            if (!match_at(p, key)) continue;
            std::size_t q = p + key.size();
            skip_ws(q);
            if (read_u(q, out)) return true;
        }
        return false;
    };

    foundation::xref::Table table;
    // Pass 2: trailer dictionaries. Scan every `trailer` keyword; the
    // last one carrying /Root wins (most recent revision is last in the
    // file). /Size is taken as the max seen.
    for (std::size_t p = 0; p + 7 <= input.size(); ++p) {
        if (!match_at(p, "trailer")) continue;
        std::size_t q = p + 7;
        skip_ws(q);
        if (!match_at(q, "<<")) continue;
        // Bound the dict body to the matching >>.
        std::size_t depth = 1, d = q + 2, dend = std::string::npos;
        while (d + 1 < input.size() && depth > 0) {
            if (match_at(d, "<<")) { ++depth; d += 2; }
            else if (match_at(d, ">>")) {
                if (--depth == 0) { dend = d; break; }
                d += 2;
            } else { ++d; }
        }
        if (dend == std::string::npos) continue;
        std::uint32_t rid = 0, iid = 0;
        std::uint16_t rg = 0, ig = 0;
        if (find_ref(q + 2, dend, "/Root", rid, rg)) {
            table.root_id = rid; table.root_gen = rg;
        }
        if (find_ref(q + 2, dend, "/Info", iid, ig)) {
            table.info_id = iid; table.info_gen = ig;
        }
        std::uint64_t sz = 0;
        if (find_int(q + 2, dend, "/Size", sz) && sz > table.size) {
            table.size = sz;
        }
        p = dend + 1;
    }

    // /Root fallback: the object whose body declares /Type /Catalog.
    if (table.root_id == 0) {
        for (const auto& [id, e] : by_id) {
            std::size_t b = static_cast<std::size_t>(e.offset_or_stream_id);
            std::size_t lim = std::min(input.size(), b + 4096);
            for (std::size_t p = b; p + 13 <= lim; ++p) {
                if (match_at(p, "/Catalog")) {
                    // Confirm it is a /Type value, not an unrelated name.
                    table.root_id = id;
                    table.root_gen = e.generation;
                    break;
                }
                if (match_at(p, "endobj")) break;
            }
            if (table.root_id != 0) break;
        }
    }

    table.entries.reserve(by_id.size());
    for (const auto& kv : by_id) table.entries.push_back(kv.second);
    std::sort(table.entries.begin(), table.entries.end(),
              [](const foundation::xref::Entry& a,
                 const foundation::xref::Entry& b) {
                  if (a.id != b.id) return a.id < b.id;
                  return a.generation < b.generation;
              });
    if (table.size == 0 && !table.entries.empty()) {
        table.size = table.entries.back().id + 1;
    }
    return table;
}

Table ParseViaStartxref(std::span<const std::byte> input) {
    // Find startxref offset (position of the integer's first digit).
    std::size_t startxref_offset = FindStartXrefOffset(input);

    // Parse the integer directly from bytes. A prior attempt routed
    // this through the lexer with a ``while (lex.Next().offset <
    // startxref_offset)`` advance loop, but that has an off-by-one:
    // the loop must call Next() to inspect each token's offset, and
    // when a token whose offset equals startxref_offset shows up,
    // Next() has already consumed it — the subsequent
    // SkipWhitespace then returns whatever follows, throwing
    // "startxref not followed by integer". FindStartXrefOffset has
    // already byte-scanned to the digit start; parse the integer
    // inline here and skip the lexer entirely for this step.
    std::uint64_t current_offset = 0;
    for (std::size_t pos = startxref_offset; pos < input.size(); ++pos) {
        char c = static_cast<char>(input[pos]);
        if (c < '0' || c > '9') break;
        current_offset = current_offset * 10
                         + static_cast<std::uint64_t>(c - '0');
    }

    // Merge entries from newest to oldest
    std::map<std::pair<std::uint32_t, std::uint16_t>, Entry> entries_map;
    std::vector<std::uint64_t> visited_offsets; // detect cycles
    Table result;

    while (current_offset != 0) {
        // Cycle detection
        if (std::find(visited_offsets.begin(), visited_offsets.end(), current_offset) != visited_offsets.end()) {
            throw std::runtime_error("xref: cycle in /Prev chain");
        }
        visited_offsets.push_back(current_offset);

        // Parse section at offset
        auto [table, prev] = ParseSectionAtOffset(input, current_offset);

        // Merge entries (newer revisions supersede older ones)
        for (const Entry& e : table.entries) {
            auto key = std::make_pair(e.id, e.generation);
            if (entries_map.find(key) == entries_map.end()) {
                entries_map[key] = e;
            }
        }

        // Merge trailer fields — newest section (visited first) wins.
        if (result.root_id == 0 && table.root_id != 0) {
            result.root_id = table.root_id; result.root_gen = table.root_gen;
        }
        if (result.info_id == 0 && table.info_id != 0) {
            result.info_id = table.info_id; result.info_gen = table.info_gen;
        }
        if (table.size > result.size) result.size = table.size;

        current_offset = prev;
    }

    // result.root_id is left 0 when the chain carries no /Root. Resolving
    // /Root is the consumer's policy, NOT xref's: objects::Parse uses only
    // .entries and must stay tolerant of a rootless-but-recoverable table,
    // while pages_tree (which needs the Catalog) raises its own clear error
    // when root_id == 0. Throwing here would narrow the whole object-parse
    // stack's input tolerance versus v1.

    // Convert map to sorted vector
    result.entries.reserve(entries_map.size());
    for (const auto& kv : entries_map) {
        result.entries.push_back(kv.second);
    }
    std::sort(result.entries.begin(), result.entries.end(),
              [](const Entry& a, const Entry& b) {
                  if (a.id != b.id) return a.id < b.id;
                  return a.generation < b.generation;
              });

    return result;
}

Table Parse(std::span<const std::byte> input) {
    if (input.empty()) {
        throw std::runtime_error("xref: empty input");
    }
    // Primary path: follow startxref + the /Prev chain. On any hard
    // failure (stale/wrong startxref offset, malformed classical
    // entries, undecodable /XRef stream) or an empty result, fall back
    // to a whole-file reconstruction (xref v3) — the same recovery qpdf
    // and mutool perform.
    try {
        Table t = ParseViaStartxref(input);
        if (!t.entries.empty()) {
            return t;
        }
    } catch (const std::exception&) {
        // fall through to reconstruction
    }
    return ReconstructXref(input);
}

std::string ToCanonical(const Table& table) {
    std::string out;
    out.reserve(table.entries.size() * 40); // rough estimate

    out += "trailer root " + std::to_string(table.root_id) + ' '
         + std::to_string(table.root_gen) + '\n';
    out += "trailer size " + std::to_string(table.size) + '\n';
    if (table.info_id != 0) {
        out += "trailer info " + std::to_string(table.info_id) + ' '
             + std::to_string(table.info_gen) + '\n';
    }

    for (const Entry& e : table.entries) {
        if (e.kind == EntryKind::InUse) {
            out += std::to_string(e.id);
            out += ' ';
            out += std::to_string(e.generation);
            out += " inuse ";
            out += std::to_string(e.offset_or_stream_id);
            out += '\n';
        } else { // Compressed
            out += std::to_string(e.id);
            out += ' ';
            out += std::to_string(e.generation);
            out += " compressed ";
            out += std::to_string(e.offset_or_stream_id);
            out += ' ';
            out += std::to_string(e.index_in_stream);
            out += '\n';
        }
    }

    return out;
}

} // namespace foundation::xref
