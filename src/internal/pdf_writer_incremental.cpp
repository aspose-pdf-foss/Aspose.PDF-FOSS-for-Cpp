// foundation::pdf_writer_incremental
// implementation. See the project spec
// for the contract; this file is the inverse of objects::Parse for
// non-stream Values, plus the §7.5.6 incremental-update mechanics
// (xref subsection + trailer with /Prev + startxref + EOF).

#include "pdf_writer_incremental.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace foundation::pdf_writer_incremental {

namespace {

void AppendStr(std::vector<std::byte>& out, const std::string& s) {
    for (char c : s) {
        out.push_back(static_cast<std::byte>(c));
    }
}

void AppendByte(std::vector<std::byte>& out, std::uint8_t b) {
    out.push_back(static_cast<std::byte>(b));
}

// Format a real with shortest round-trip decimal + a `.`.
// `5` → `5.0`; `0.5` → `0.5`; `-0.123` → `-0.123`; never
// scientific notation. PDF spec only requires 5 fractional
// digits but we use up to 6 for safety against round-trip.
std::string FormatReal(double x) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6f", x);
    std::string s(buf);
    // Trim trailing zeros after the dot — but keep at least one
    // digit after the dot ("5.0" not "5.").
    auto dot = s.find('.');
    if (dot != std::string::npos) {
        std::size_t last_nonzero = s.size() - 1;
        while (last_nonzero > dot + 1 && s[last_nonzero] == '0') {
            --last_nonzero;
        }
        s.erase(last_nonzero + 1);
    }
    return s;
}

// Encode a Name's bytes per §7.3.5 — 0x21..0x7E pass through
// except for delimiters; everything else escapes as `#XX`.
std::string EncodeName(const std::string& decoded) {
    std::string out;
    out.reserve(decoded.size() + 1);
    out.push_back('/');
    for (unsigned char c : decoded) {
        const bool printable = c >= 0x21 && c <= 0x7E;
        const bool delim = (c == '(' || c == ')' || c == '<' ||
                            c == '>' || c == '[' || c == ']' ||
                            c == '{' || c == '}' || c == '/' ||
                            c == '%' || c == '#');
        if (printable && !delim) {
            out.push_back(static_cast<char>(c));
        } else {
            char hex[4];
            std::snprintf(hex, sizeof(hex), "#%02X", c);
            out.append(hex);
        }
    }
    return out;
}

// Encode a literal string per §7.3.4.2 — wrap in `()`,
// double-escape backslashes, escape `(`/`)` and the
// standard control sequences. Non-printable bytes go to
// octal `\DDD` form.
void AppendLiteralString(std::vector<std::byte>& out,
                          const std::vector<std::byte>& bytes) {
    AppendByte(out, '(');
    for (auto b : bytes) {
        const std::uint8_t c = static_cast<std::uint8_t>(b);
        switch (c) {
            case '\\':
                AppendByte(out, '\\');
                AppendByte(out, '\\');
                break;
            case '(':
                AppendByte(out, '\\');
                AppendByte(out, '(');
                break;
            case ')':
                AppendByte(out, '\\');
                AppendByte(out, ')');
                break;
            case '\n':
                AppendByte(out, '\\');
                AppendByte(out, 'n');
                break;
            case '\r':
                AppendByte(out, '\\');
                AppendByte(out, 'r');
                break;
            case '\t':
                AppendByte(out, '\\');
                AppendByte(out, 't');
                break;
            case '\b':
                AppendByte(out, '\\');
                AppendByte(out, 'b');
                break;
            case '\f':
                AppendByte(out, '\\');
                AppendByte(out, 'f');
                break;
            default:
                if (c >= 0x20 && c <= 0x7E) {
                    AppendByte(out, c);
                } else {
                    char buf[5];
                    std::snprintf(buf, sizeof(buf),
                                  "\\%03o", c);
                    for (int i = 0; buf[i]; ++i) {
                        AppendByte(out, buf[i]);
                    }
                }
                break;
        }
    }
    AppendByte(out, ')');
}

void AppendHexString(std::vector<std::byte>& out,
                      const std::vector<std::byte>& bytes) {
    AppendByte(out, '<');
    char buf[3];
    for (auto b : bytes) {
        std::snprintf(buf, sizeof(buf), "%02X",
                      static_cast<std::uint8_t>(b));
        AppendByte(out, buf[0]);
        AppendByte(out, buf[1]);
    }
    AppendByte(out, '>');
}

// Forward declaration — Value variants are mutually recursive.
void SerializeValue(std::vector<std::byte>& out,
                     const objects::Value& v);

void SerializeArray(std::vector<std::byte>& out,
                     const objects::Array& a) {
    AppendStr(out, "[ ");
    for (const auto& item : a.items) {
        SerializeValue(out, item);
        AppendByte(out, ' ');
    }
    AppendByte(out, ']');
}

void SerializeDict(std::vector<std::byte>& out,
                    const objects::Dict& d) {
    AppendStr(out, "<<\n");
    for (const auto& [k, v] : d.entries) {
        AppendStr(out, "  ");
        AppendStr(out, EncodeName(k));
        AppendByte(out, ' ');
        SerializeValue(out, v);
        AppendByte(out, '\n');
    }
    AppendStr(out, ">>");
}

// Emit a top-level Stream object: augmented header (with
// /Length replaced/added to the body byte length), then
// `\nstream\n<body>\nendstream`. The caller-supplied /Length
// is overridden for honesty — the serialised body length is
// authoritative at write time.
void SerializeStream(std::vector<std::byte>& out,
                     const objects::Stream& sv) {
    objects::Dict augmented;
    augmented.entries.reserve(sv.header.entries.size() + 1);
    bool length_set = false;
    for (const auto& kv : sv.header.entries) {
        if (kv.first == "Length") {
            objects::Value len;
            len.v = static_cast<std::int64_t>(sv.body.size());
            augmented.entries.emplace_back("Length", std::move(len));
            length_set = true;
        } else {
            augmented.entries.push_back(kv);
        }
    }
    if (!length_set) {
        objects::Value len;
        len.v = static_cast<std::int64_t>(sv.body.size());
        augmented.entries.emplace_back("Length", std::move(len));
    }
    SerializeDict(out, augmented);
    AppendStr(out, "\nstream\n");
    out.insert(out.end(), sv.body.begin(), sv.body.end());
    AppendStr(out, "\nendstream");
}

void SerializeValue(std::vector<std::byte>& out,
                     const objects::Value& v) {
    if (std::holds_alternative<std::monostate>(v.v)) {
        AppendStr(out, "null");
        return;
    }
    if (const auto* b = std::get_if<bool>(&v.v)) {
        AppendStr(out, *b ? "true" : "false");
        return;
    }
    if (const auto* i = std::get_if<std::int64_t>(&v.v)) {
        AppendStr(out, std::to_string(*i));
        return;
    }
    if (const auto* x = std::get_if<double>(&v.v)) {
        AppendStr(out, FormatReal(*x));
        return;
    }
    if (const auto* name = std::get_if<std::string>(&v.v)) {
        AppendStr(out, EncodeName(*name));
        return;
    }
    if (const auto* str = std::get_if<objects::String>(&v.v)) {
        if (str->kind == objects::StringKind::Hex) {
            AppendHexString(out, str->bytes);
        } else {
            AppendLiteralString(out, str->bytes);
        }
        return;
    }
    if (const auto* arr = std::get_if<objects::Array>(&v.v)) {
        SerializeArray(out, *arr);
        return;
    }
    if (const auto* dict = std::get_if<objects::Dict>(&v.v)) {
        SerializeDict(out, *dict);
        return;
    }
    if (std::holds_alternative<objects::Stream>(v.v)) {
        // §7.3.8.1 forbids direct streams. v1.1 accepts Stream
        // ONLY as the top-level DirtyObject.value (handled by
        // the caller via SerializeStream); appearing here means
        // a Stream is nested inside a dict / array.
        throw std::runtime_error(
            "pdf_writer_incremental: Stream cannot appear as a "
            "direct child (PDF §7.3.8.1); streams are legal only "
            "as the top-level DirtyObject.value.");
    }
    if (const auto* ref = std::get_if<objects::Ref>(&v.v)) {
        AppendStr(out, std::to_string(ref->id));
        AppendByte(out, ' ');
        AppendStr(out, std::to_string(ref->generation));
        AppendStr(out, " R");
        return;
    }
    throw std::runtime_error(
        "pdf_writer_incremental: unhandled Value variant.");
}

// Find the LAST occurrence of `needle` in `haystack`.
// Used to locate `startxref` and `trailer` near the end of
// the original PDF. Returns std::string::npos if not found.
std::size_t FindLast(std::span<const std::byte> haystack,
                     std::string_view needle) {
    if (needle.empty() || haystack.size() < needle.size()) {
        return std::string::npos;
    }
    for (std::size_t i = haystack.size() - needle.size() + 1;
         i > 0; --i) {
        const std::size_t pos = i - 1;
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (static_cast<char>(haystack[pos + j])
                != needle[j]) {
                match = false;
                break;
            }
        }
        if (match) return pos;
    }
    return std::string::npos;
}

// Parse `startxref\n<N>\n%%EOF` near the end of the input.
// Returns the integer N (the byte offset of the original
// xref). Throws on malformed input.
std::uint64_t ParseStartXref(std::span<const std::byte> input) {
    const auto sx = FindLast(input, "startxref");
    if (sx == std::string::npos) {
        throw std::runtime_error(
            "pdf_writer_incremental: startxref not found in "
            "original input");
    }
    // Skip past "startxref" and any whitespace.
    std::size_t i = sx + std::strlen("startxref");
    while (i < input.size()) {
        const char c = static_cast<char>(input[i]);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            ++i;
        } else {
            break;
        }
    }
    // Read decimal digits.
    std::uint64_t n = 0;
    bool any = false;
    while (i < input.size()) {
        const char c = static_cast<char>(input[i]);
        if (c >= '0' && c <= '9') {
            n = n * 10 + static_cast<std::uint64_t>(c - '0');
            any = true;
            ++i;
        } else {
            break;
        }
    }
    if (!any) {
        throw std::runtime_error(
            "pdf_writer_incremental: startxref has no offset");
    }
    return n;
}

// Find the trailer dict in the input. Returns a substring
// (offset, length) covering `<< … >>` after the last
// `trailer` keyword. The original may have multiple trailers
// (in a chain of incremental updates); we want the last.
std::pair<std::size_t, std::size_t> LocateTrailerDict(
        std::span<const std::byte> input) {
    const auto tr = FindLast(input, "trailer");
    if (tr == std::string::npos) {
        throw std::runtime_error(
            "pdf_writer_incremental: trailer keyword not "
            "found in original input");
    }
    // Find the opening `<<` after `trailer`.
    std::size_t i = tr + std::strlen("trailer");
    while (i + 1 < input.size()) {
        if (static_cast<char>(input[i]) == '<'
            && static_cast<char>(input[i + 1]) == '<') {
            break;
        }
        ++i;
    }
    if (i + 1 >= input.size()) {
        throw std::runtime_error(
            "pdf_writer_incremental: trailer dict open `<<` "
            "not found");
    }
    const std::size_t start = i;
    // Find matching `>>` accounting for nested `<<` / `>>`.
    int depth = 0;
    while (i + 1 < input.size()) {
        if (static_cast<char>(input[i]) == '<'
            && static_cast<char>(input[i + 1]) == '<') {
            ++depth;
            i += 2;
            continue;
        }
        if (static_cast<char>(input[i]) == '>'
            && static_cast<char>(input[i + 1]) == '>') {
            --depth;
            i += 2;
            if (depth == 0) {
                return {start, i - start};
            }
            continue;
        }
        ++i;
    }
    throw std::runtime_error(
        "pdf_writer_incremental: trailer dict close `>>` not "
        "found");
}

// Parse the trailer dict by feeding the captured bytes into a
// minimal indirect-object wrapper that objects::Parse can
// consume. The wrapper makes the trailer look like
//   "1 0 obj <<...>> endobj"
// in a tiny synthetic byte buffer with a 1-entry xref so
// objects::Parse succeeds. Returns the parsed Dict.
objects::Dict ParseTrailerAsDict(
        std::span<const std::byte> input,
        std::size_t dict_offset,
        std::size_t dict_length) {
    // Wrap the dict bytes in a synthetic mini-PDF that
    // objects::Parse can ingest.
    std::string prefix = "%PDF-1.4\n1 0 obj\n";
    std::string suffix = "\nendobj\n";
    std::string xref_pos_marker;
    std::vector<std::byte> wrap;
    for (char c : prefix) {
        wrap.push_back(static_cast<std::byte>(c));
    }
    const std::size_t obj_start =
        wrap.size() - std::strlen("1 0 obj\n");
    for (std::size_t k = 0; k < dict_length; ++k) {
        wrap.push_back(input[dict_offset + k]);
    }
    for (char c : suffix) {
        wrap.push_back(static_cast<std::byte>(c));
    }
    const std::size_t xref_off = wrap.size();
    char xref_buf[256];
    std::snprintf(xref_buf, sizeof(xref_buf),
        "xref\n0 2\n0000000000 65535 f \n%010zu 00000 n \n"
        "trailer\n<< /Size 2 /Root 1 0 R >>\n"
        "startxref\n%zu\n%%%%EOF",
        obj_start, xref_off);
    for (int k = 0; xref_buf[k]; ++k) {
        wrap.push_back(static_cast<std::byte>(xref_buf[k]));
    }
    auto dump = objects::Parse(
        std::span<const std::byte>(wrap.data(), wrap.size()));
    if (dump.objects.empty()) {
        throw std::runtime_error(
            "pdf_writer_incremental: trailer dict reparse "
            "produced empty dump");
    }
    if (auto* d = std::get_if<objects::Dict>(
            &dump.objects.front().value.v)) {
        return std::move(*d);
    }
    throw std::runtime_error(
        "pdf_writer_incremental: trailer is not a dict");
}

// Replace the `/Prev` entry on a trailer Dict with the given
// offset. If `/Prev` already exists (chained incremental),
// overwrite it; otherwise, append.
void SetOrReplacePrev(objects::Dict& trailer,
                       std::uint64_t prev_offset) {
    objects::Value v;
    v.v = static_cast<std::int64_t>(prev_offset);
    for (auto& [k, val] : trailer.entries) {
        if (k == "Prev") {
            val = std::move(v);
            return;
        }
    }
    trailer.entries.emplace_back("Prev", std::move(v));
}

// Update `/Size` to the new max id+1 if that is larger than
// the original. Per spec, /Size is the highest id+1 the file
// uses; an incremental that adds a new id must bump it.
void UpdateSize(objects::Dict& trailer,
                 std::uint32_t new_max_plus_one) {
    for (auto& [k, val] : trailer.entries) {
        if (k == "Size") {
            if (auto* sz =
                    std::get_if<std::int64_t>(&val.v)) {
                if (static_cast<std::uint32_t>(*sz)
                    < new_max_plus_one) {
                    *sz = new_max_plus_one;
                }
                return;
            }
        }
    }
    objects::Value v;
    v.v = static_cast<std::int64_t>(new_max_plus_one);
    trailer.entries.emplace_back("Size", std::move(v));
}

}  // namespace

namespace {

// Set or replace the `/Encrypt` indirect reference on the trailer
// dict. Caller provides the (id, generation) of the /Encrypt
// indirect object — the dict body itself is supplied as a
// separate DirtyObject in the dirty list.
void SetOrReplaceEncrypt(objects::Dict& trailer,
                          std::uint32_t encrypt_id,
                          std::uint16_t encrypt_gen) {
    objects::Value v;
    v.v = objects::Ref{encrypt_id, encrypt_gen};
    for (auto& [k, val] : trailer.entries) {
        if (k == "Encrypt") {
            val = std::move(v);
            return;
        }
    }
    trailer.entries.emplace_back("Encrypt", std::move(v));
}

// Set or replace the `/ID` two-element byte-string array on the
// trailer dict. The two byte vectors become Hex-kind strings on
// the wire (PDF spec says either kind is legal; hex is the
// conventional choice for /ID, matching what pikepdf / qpdf
// emit).
void SetOrReplaceFileId(objects::Dict& trailer,
                         const std::vector<std::byte>& first,
                         const std::vector<std::byte>& second) {
    objects::Array id_array;
    {
        objects::String s;
        s.bytes = first;
        s.kind = objects::StringKind::Hex;
        objects::Value v;
        v.v = std::move(s);
        id_array.items.push_back(std::move(v));
    }
    {
        objects::String s;
        s.bytes = second;
        s.kind = objects::StringKind::Hex;
        objects::Value v;
        v.v = std::move(s);
        id_array.items.push_back(std::move(v));
    }
    objects::Value v;
    v.v = std::move(id_array);
    for (auto& [k, val] : trailer.entries) {
        if (k == "ID") {
            val = std::move(v);
            return;
        }
    }
    trailer.entries.emplace_back("ID", std::move(v));
}

}  // namespace

std::vector<std::byte> AppendIncremental(
        std::span<const std::byte> original,
        std::span<const DirtyObject> dirty,
        const AppendOptions& options) {
    // Validate dirty list — duplicate ids forbidden.
    {
        std::set<std::uint32_t> seen;
        for (const auto& d : dirty) {
            if (!seen.insert(d.id).second) {
                throw std::runtime_error(
                    "pdf_writer_incremental: duplicate id "
                    + std::to_string(d.id) + " in dirty list");
            }
        }
    }

    // Locate startxref + trailer dict in the original.
    const std::uint64_t orig_startxref = ParseStartXref(original);
    const auto [tr_off, tr_len] = LocateTrailerDict(original);
    objects::Dict trailer =
        ParseTrailerAsDict(original, tr_off, tr_len);

    // Compute new /Size — if any dirty id exceeds the
    // original's /Size, bump it.
    std::uint32_t max_id = 0;
    for (const auto& d : dirty) {
        if (d.id > max_id) max_id = d.id;
    }
    UpdateSize(trailer, max_id + 1);
    SetOrReplacePrev(trailer, orig_startxref);

    // v2: trailer overlay for encryption-aware writes.
    if (options.has_encrypt_ref) {
        SetOrReplaceEncrypt(trailer,
                             options.encrypt_id,
                             options.encrypt_gen);
    }
    if (options.has_file_id) {
        SetOrReplaceFileId(trailer,
                            options.file_id_first,
                            options.file_id_second);
    }

    // Start the output buffer with the original verbatim.
    std::vector<std::byte> out(original.begin(), original.end());

    // PDF requires the file to end with `%%EOF` followed by an
    // optional EOL. If the original ends without a trailing
    // newline, add one before appending so the new section
    // starts on a fresh line.
    if (!out.empty()) {
        const auto last = static_cast<char>(out.back());
        if (last != '\n' && last != '\r') {
            out.push_back(static_cast<std::byte>('\n'));
        }
    }

    // Emit each dirty object, recording its byte offset. A
    // top-level Stream value is emitted via SerializeStream
    // (dict + stream/endstream with /Length auto-injected);
    // any other variant goes through SerializeValue.
    std::vector<std::pair<std::uint32_t, std::size_t>> offsets;
    offsets.reserve(dirty.size());
    for (const auto& d : dirty) {
        offsets.emplace_back(d.id, out.size());
        AppendStr(out, std::to_string(d.id));
        AppendByte(out, ' ');
        AppendStr(out, std::to_string(d.generation));
        AppendStr(out, " obj\n");
        if (const auto* sv = std::get_if<objects::Stream>(&d.value.v)) {
            SerializeStream(out, *sv);
        } else {
            SerializeValue(out, d.value);
        }
        AppendStr(out, "\nendobj\n");
    }

    // Sort by id ascending so we can build dense subsections.
    std::sort(offsets.begin(), offsets.end());

    const std::size_t new_xref_offset = out.size();
    AppendStr(out, "xref\n");

    // Emit the free-list head subsection (object 0 always
    // present per §7.5.4).
    AppendStr(out, "0 1\n0000000000 65535 f \n");

    // Emit one subsection per run of consecutive ids.
    std::size_t i = 0;
    while (i < offsets.size()) {
        std::size_t j = i + 1;
        while (j < offsets.size()
               && offsets[j].first == offsets[j - 1].first + 1) {
            ++j;
        }
        const std::uint32_t first_id = offsets[i].first;
        const std::size_t run_len = j - i;
        AppendStr(out, std::to_string(first_id));
        AppendByte(out, ' ');
        AppendStr(out, std::to_string(run_len));
        AppendByte(out, '\n');
        for (std::size_t k = i; k < j; ++k) {
            char buf[32];
            // Find the dirty entry for this id to recover its
            // generation (offsets[] only carries id+offset).
            std::uint16_t gen = 0;
            for (const auto& d : dirty) {
                if (d.id == offsets[k].first) {
                    gen = d.generation;
                    break;
                }
            }
            std::snprintf(buf, sizeof(buf),
                "%010zu %05u n \n",
                offsets[k].second,
                static_cast<unsigned>(gen));
            AppendStr(out, buf);
        }
        i = j;
    }

    // Emit the new trailer + startxref + EOF.
    AppendStr(out, "trailer\n");
    SerializeDict(out, trailer);
    AppendStr(out, "\nstartxref\n");
    AppendStr(out, std::to_string(new_xref_offset));
    AppendStr(out, "\n%%EOF\n");

    return out;
}

// v1.1 compatibility overload — empty AppendOptions = behave
// exactly as before v2 (no trailer overlay).
std::vector<std::byte> AppendIncremental(
        std::span<const std::byte> original,
        std::span<const DirtyObject> dirty) {
    return AppendIncremental(original, dirty, AppendOptions{});
}

}  // namespace foundation::pdf_writer_incremental
