#include "text_extractor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "agl.hpp"
#include "content_stream.hpp"
#include "crypt_filter.hpp"
#include "encoding_tables.hpp"
#include "flate.hpp"
#include "lexer.hpp"
#include "objects.hpp"
#include "to_unicode.hpp"

namespace {

// Decryption context threaded through the per-page walk.
// Empty file_key = plaintext mode (the original v1/v2/v2.1
// behaviour, kept for the password-less Parse entry point).
struct CryptoCtx {
    std::span<const std::byte> file_key;
};

constexpr CryptoCtx kPlaintext{};

// Helper: resolve a value (follow Refs)
foundation::objects::Value ResolveValue(const foundation::objects::Value& value, const foundation::objects::Dump& dump) {
    if (std::holds_alternative<foundation::objects::Ref>(value.v)) {
        const auto& ref = std::get<foundation::objects::Ref>(value.v);
        for (const auto& obj : dump.objects) {
            if (obj.id == ref.id && obj.generation == ref.generation) {
                return obj.value;
            }
        }
        return value; // unresolved Ref → return as-is (will be caught later)
    }
    return value;
}

// Helper: find dict entry by key (linear search over vector)
std::pair<std::string, foundation::objects::Value>* FindDictEntry(foundation::objects::Dict& dict, const std::string& key) {
    for (auto& entry : dict.entries) {
        if (entry.first == key) return &entry;
    }
    return nullptr;
}

const std::pair<std::string, foundation::objects::Value>* FindDictEntry(const foundation::objects::Dict& dict, const std::string& key) {
    for (const auto& entry : dict.entries) {
        if (entry.first == key) return &entry;
    }
    return nullptr;
}

// Helper: resolve stream bytes with optional decryption + filter
// decoding. obj_id == 0 means "no indirect reference" (inline
// stream); per PDF 32000 §7.6 such streams are never encrypted,
// so decryption is skipped regardless of crypto.file_key. When
// crypto.file_key is non-empty AND obj_id != 0, the body bytes
// are run through foundation::crypt_filter::Decrypt with method
// AESV2 BEFORE any /Filter chain (decrypt-then-decompress is
// the spec order for encrypted streams).
std::vector<std::byte> ResolveStreamBytes(
        const foundation::objects::Stream& stream,
        const foundation::objects::Dump& dump,
        std::span<const std::byte> pdf_bytes,
        const CryptoCtx& crypto,
        std::uint32_t obj_id,
        std::uint32_t generation) {
    // Get /Length
    const auto* length_entry = FindDictEntry(stream.header, "Length");
    if (!length_entry) {
        throw std::runtime_error("/Length missing in stream header");
    }

    foundation::objects::Value length_val = ResolveValue(length_entry->second, dump);
    if (!std::holds_alternative<std::int64_t>(length_val.v)) {
        throw std::runtime_error("/Length is not an integer");
    }
    std::int64_t length = std::get<std::int64_t>(length_val.v);

    // Check /Filter
    const auto* filter_entry = FindDictEntry(stream.header, "Filter");

    if (filter_entry) {
        foundation::objects::Value filter_val = ResolveValue(filter_entry->second, dump);

        if (std::holds_alternative<foundation::objects::Array>(filter_val.v)) {
            const auto& arr = std::get<foundation::objects::Array>(filter_val.v);
            if (arr.items.size() != 1 || !std::holds_alternative<std::string>(arr.items[0].v)) {
                throw std::runtime_error("Unsupported /Filter array (expected single Name)");
            }
            const std::string& filter_name = std::get<std::string>(arr.items[0].v);
            if (filter_name != "FlateDecode") {
                throw std::runtime_error("Unsupported filter: " + filter_name);
            }
        } else if (std::holds_alternative<std::string>(filter_val.v)) {
            const std::string& filter_name = std::get<std::string>(filter_val.v);
            if (filter_name != "FlateDecode") {
                throw std::runtime_error("Unsupported filter: " + filter_name);
            }
        } else {
            throw std::runtime_error("/Filter is not a Name or Array");
        }
    }

    // stream.body is a span pointing into the parsed source
    // buffer (parser bounds-checks at parse time). Copy to an
    // owned vector so the downstream decrypt / FlateDecode
    // chain can mutate without aliasing the source.
    std::vector<std::byte> body(stream.body.begin(), stream.body.end());

    // Decrypt before /Filter (PDF §7.6 spec order). Skip when in
    // plaintext mode (file_key empty) or when the stream is inline
    // (obj_id == 0) — inline streams are never encrypted.
    if (!crypto.file_key.empty() && obj_id != 0) {
        body = foundation::crypt_filter::Decrypt(
            foundation::crypt_filter::Method::AESV2,
            crypto.file_key, obj_id, generation,
            std::span<const std::byte>{body.data(), body.size()});
    }

    // Apply FlateDecode if present
    if (filter_entry) {
        body = foundation::flate::Decode({body.data(), body.size()});
    }

    return body;
}

// Capture {obj_id, gen} from a Value that is (typically) an
// indirect reference. Inline values return {0, 0}, signalling
// "no indirect reference" to ResolveStreamBytes so it skips
// decryption — inline streams are unencrypted per PDF §7.6.
std::pair<std::uint32_t, std::uint32_t> RefIdentity(
        const foundation::objects::Value& v) {
    if (std::holds_alternative<foundation::objects::Ref>(v.v)) {
        const auto& r = std::get<foundation::objects::Ref>(v.v);
        return {r.id, static_cast<std::uint32_t>(r.generation)};
    }
    return {0u, 0u};
}

// Helper: find /Resources by walking up /Parent chain
foundation::objects::Dict ResolveResources(const foundation::objects::Dict& page_dict, const foundation::objects::Dump& dump) {
    const auto* resources_entry = FindDictEntry(page_dict, "Resources");
    if (resources_entry) {
        foundation::objects::Value resources_val = ResolveValue(resources_entry->second, dump);
        if (std::holds_alternative<foundation::objects::Dict>(resources_val.v)) {
            return std::get<foundation::objects::Dict>(resources_val.v);
        }
    }
    
    // Walk up /Parent chain
    const auto* parent_entry = FindDictEntry(page_dict, "Parent");
    while (parent_entry) {
        foundation::objects::Value parent_val = ResolveValue(parent_entry->second, dump);
        if (!std::holds_alternative<foundation::objects::Dict>(parent_val.v)) {
            break;
        }
        const foundation::objects::Dict& parent_dict = std::get<foundation::objects::Dict>(parent_val.v);
        
        resources_entry = FindDictEntry(parent_dict, "Resources");
        if (resources_entry) {
            foundation::objects::Value resources_val = ResolveValue(resources_entry->second, dump);
            if (std::holds_alternative<foundation::objects::Dict>(resources_val.v)) {
                return std::get<foundation::objects::Dict>(resources_val.v);
            }
        }
        
        parent_entry = FindDictEntry(parent_dict, "Parent");
    }
    
    return {}; // No /Resources found
}

// Helper: get font dict from resources by font name
foundation::objects::Dict GetFontDict(const foundation::objects::Dict& resources, const std::string& font_name, const foundation::objects::Dump& dump) {
    const auto* fonts_entry = FindDictEntry(resources, "Font");
    if (!fonts_entry) {
        throw std::runtime_error("No /Font dictionary in /Resources");
    }
    
    foundation::objects::Value fonts_val = ResolveValue(fonts_entry->second, dump);
    if (!std::holds_alternative<foundation::objects::Dict>(fonts_val.v)) {
        throw std::runtime_error("/Font is not a dictionary");
    }
    
    const foundation::objects::Dict& fonts_dict = std::get<foundation::objects::Dict>(fonts_val.v);
    const auto* font_entry = FindDictEntry(fonts_dict, font_name);
    if (!font_entry) {
        throw std::runtime_error("Font '" + font_name + "' not found in /Resources");
    }
    
    foundation::objects::Value font_val = ResolveValue(font_entry->second, dump);
    if (!std::holds_alternative<foundation::objects::Dict>(font_val.v)) {
        throw std::runtime_error("Font '" + font_name + "' is not a dictionary");
    }
    
    return std::get<foundation::objects::Dict>(font_val.v);
}

// Helper: append a Unicode codepoint as UTF-8.
void EmitUtf8(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// Per-font /Encoding state.
//   * base = the standard encoding selected by /Encoding /<Name>
//            or /Encoding << /BaseEncoding /<Name> >>. nullopt
//            when /Encoding is absent or names an unknown encoding.
//   * differences = per-slot overrides parsed from /Differences.
//                   Empty when /Differences is absent or
//                   /Encoding is a simple Name (which can't carry
//                   /Differences).
struct FontEncoding {
    std::optional<foundation::encoding_tables::Encoding> base;
    std::vector<std::pair<std::uint8_t, std::string>> differences;
};

// Parse the font dict's /Encoding entry into a FontEncoding.
// Returns nullopt only when /Encoding is absent — both base
// and differences may be empty within the returned struct
// (rare but legal: /Encoding /SymbolEncoding leaves base
// nullopt; /Encoding << /Differences [...] >> with no
// /BaseEncoding does the same).
std::optional<FontEncoding> GetFontEncoding(
        const foundation::objects::Dict& font_dict,
        const foundation::objects::Dump& dump) {
    const auto* enc_entry = FindDictEntry(font_dict, "Encoding");
    if (!enc_entry) return std::nullopt;

    foundation::objects::Value enc_val =
        ResolveValue(enc_entry->second, dump);

    FontEncoding info;

    // Simple form: /Encoding /WinAnsiEncoding
    if (std::holds_alternative<std::string>(enc_val.v)) {
        info.base = foundation::encoding_tables::EncodingFromName(
            std::get<std::string>(enc_val.v));
        return info;
    }

    // Dict form: /Encoding << /BaseEncoding /WinAnsiEncoding
    //                        /Differences [128 /Euro 153 /trademark] >>
    if (std::holds_alternative<foundation::objects::Dict>(enc_val.v)) {
        const auto& enc_dict =
            std::get<foundation::objects::Dict>(enc_val.v);

        // /BaseEncoding (optional within the dict form).
        if (const auto* base = FindDictEntry(enc_dict, "BaseEncoding")) {
            foundation::objects::Value base_val =
                ResolveValue(base->second, dump);
            if (std::holds_alternative<std::string>(base_val.v)) {
                info.base = foundation::encoding_tables::EncodingFromName(
                    std::get<std::string>(base_val.v));
            }
        }

        // /Differences — PostScript-style array. An integer
        // sets the next-slot cursor; subsequent Names fill
        // consecutive slots starting at that cursor. Per
        // §9.6.6.1, an entry value > 255 is invalid for the
        // 256-slot byte-addressing table.
        if (const auto* diff = FindDictEntry(enc_dict, "Differences")) {
            foundation::objects::Value diff_val =
                ResolveValue(diff->second, dump);
            if (std::holds_alternative<foundation::objects::Array>(diff_val.v)) {
                const auto& arr =
                    std::get<foundation::objects::Array>(diff_val.v);
                int cursor = 0;
                for (const auto& item : arr.items) {
                    if (std::holds_alternative<std::int64_t>(item.v)) {
                        std::int64_t v = std::get<std::int64_t>(item.v);
                        if (v < 0 || v > 255) {
                            throw std::runtime_error(
                                "/Differences index out of byte range");
                        }
                        cursor = static_cast<int>(v);
                    } else if (std::holds_alternative<std::string>(item.v)) {
                        if (cursor < 0 || cursor > 255) {
                            throw std::runtime_error(
                                "/Differences cursor out of byte range");
                        }
                        info.differences.push_back(
                            {static_cast<std::uint8_t>(cursor),
                             std::get<std::string>(item.v)});
                        cursor++;
                    }
                    // Other token kinds are not valid in
                    // /Differences per §9.6.6.1; ignore for
                    // robustness.
                }
            }
        }

        return info;
    }

    return std::nullopt;
}

// Helper: get ToUnicode CMap from font dict
foundation::to_unicode::ToUnicode GetToUnicode(
        const foundation::objects::Dict& font_dict,
        const foundation::objects::Dump& dump,
        std::span<const std::byte> pdf_bytes,
        const CryptoCtx& crypto) {
    const auto* to_unicode_entry = FindDictEntry(font_dict, "ToUnicode");
    if (!to_unicode_entry) {
        throw std::runtime_error("Font has no /ToUnicode");
    }

    // Capture the Ref identity BEFORE resolution so we can decrypt
    // the resolved stream's body using the right per-object key.
    auto [obj_id, gen] = RefIdentity(to_unicode_entry->second);
    foundation::objects::Value to_unicode_val = ResolveValue(to_unicode_entry->second, dump);
    if (!std::holds_alternative<foundation::objects::Stream>(to_unicode_val.v)) {
        throw std::runtime_error("/ToUnicode is not a stream");
    }

    const foundation::objects::Stream& stream = std::get<foundation::objects::Stream>(to_unicode_val.v);
    std::vector<std::byte> stream_bytes = ResolveStreamBytes(
        stream, dump, pdf_bytes, crypto, obj_id, gen);

    return foundation::to_unicode::Parse({stream_bytes.data(), stream_bytes.size()});
}

// Helper: decode a String value. Decoder priority per
// PDF 32000-1:2008 §9.10.2 (mutually exclusive):
//   1. /ToUnicode CMap (most accurate; wins when present).
//   2. /Encoding (with optional /Differences override on top
//      of the BaseEncoding) → AGL.
//   3. Latin-1 byte-as-char fallback.
std::string DecodeString(
        const foundation::objects::String& str,
        const foundation::to_unicode::ToUnicode* cmap,
        const FontEncoding* encoding) {

    // Priority 1: /ToUnicode CMap.
    if (cmap && !cmap->codespace.empty()) {
        std::string result;
        size_t i = 0;
        while (i < str.bytes.size()) {
            bool found = false;
            for (const auto& range : cmap->codespace) {
                if (range.byte_count == 1) {
                    if (i < str.bytes.size()) {
                        uint32_t code = static_cast<uint8_t>(str.bytes[i]);
                        if (code >= range.start && code <= range.end) {
                            auto it = std::lower_bound(
                                cmap->mappings.begin(), cmap->mappings.end(),
                                code,
                                [](const auto& m, uint32_t c) { return m.char_code < c; });
                            if (it != cmap->mappings.end()
                                    && it->char_code == code
                                    && it->byte_count == 1) {
                                for (uint32_t cp : it->unicode) EmitUtf8(result, cp);
                            } else {
                                result.push_back(static_cast<char>(
                                    static_cast<unsigned char>(str.bytes[i])));
                            }
                            i++;
                            found = true;
                            break;
                        }
                    }
                } else if (range.byte_count == 2) {
                    if (i + 1 < str.bytes.size()) {
                        uint32_t code =
                            (static_cast<uint8_t>(str.bytes[i]) << 8)
                            | static_cast<uint8_t>(str.bytes[i + 1]);
                        if (code >= range.start && code <= range.end) {
                            auto it = std::lower_bound(
                                cmap->mappings.begin(), cmap->mappings.end(),
                                code,
                                [](const auto& m, uint32_t c) { return m.char_code < c; });
                            if (it != cmap->mappings.end()
                                    && it->char_code == code
                                    && it->byte_count == 2) {
                                for (uint32_t cp : it->unicode) EmitUtf8(result, cp);
                            } else {
                                result.push_back(static_cast<char>(
                                    static_cast<unsigned char>(str.bytes[i])));
                            }
                            i += 2;
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (!found) {
                result.push_back(static_cast<char>(
                    static_cast<unsigned char>(str.bytes[i])));
                i++;
            }
        }
        return result;
    }

    // Priority 2: /Encoding (BaseEncoding ± /Differences) → AGL.
    if (encoding) {
        std::string result;
        result.reserve(str.bytes.size());
        for (std::byte b : str.bytes) {
            std::uint8_t code = static_cast<std::uint8_t>(b);

            // /Differences override wins over BaseEncoding.
            std::string_view glyph;
            std::string diff_glyph;
            for (const auto& [slot, name] : encoding->differences) {
                if (slot == code) { diff_glyph = name; break; }
            }
            if (!diff_glyph.empty()) {
                glyph = diff_glyph;
            } else if (encoding->base) {
                glyph = foundation::encoding_tables::ByteToGlyphName(
                    *encoding->base, code);
            }

            if (glyph.empty()) {
                // No override matched AND base is nullopt OR
                // base has the slot unmapped. Preserve byte.
                result.push_back(static_cast<char>(code));
                continue;
            }
            auto cps = foundation::agl::Lookup(glyph);
            if (cps.empty()) {
                result.push_back(static_cast<char>(code));
                continue;
            }
            for (std::uint32_t cp : cps) EmitUtf8(result, cp);
        }
        return result;
    }

    // Priority 3a: UTF-16BE byte-order-mark fallback. With neither a
    // /ToUnicode CMap nor a font /Encoding, a show-string that opens with the
    // UTF-16BE BOM (0xFE 0xFF) is decoded as UTF-16BE — the PDF text-string
    // convention (§7.9.2.2) applied as a last-resort heuristic before raw
    // byte interpretation.
    if (str.bytes.size() >= 2 &&
        static_cast<unsigned char>(str.bytes[0]) == 0xFE &&
        static_cast<unsigned char>(str.bytes[1]) == 0xFF) {
        std::string result;
        for (std::size_t i = 2; i + 1 < str.bytes.size(); i += 2) {
            std::uint32_t cp =
                (static_cast<std::uint32_t>(
                     static_cast<unsigned char>(str.bytes[i])) << 8) |
                 static_cast<std::uint32_t>(
                     static_cast<unsigned char>(str.bytes[i + 1]));
            EmitUtf8(result, cp);
        }
        return result;
    }

    // Priority 3b: Latin-1 fallback.
    std::string result;
    result.reserve(str.bytes.size());
    for (std::byte b : str.bytes) {
        result.push_back(static_cast<char>(static_cast<unsigned char>(b)));
    }
    return result;
}

// Helper: extract text from a single page
std::string ExtractTextFromPage(
        const foundation::objects::Dict& page_dict,
        const foundation::objects::Dump& dump,
        std::span<const std::byte> pdf_bytes,
        const CryptoCtx& crypto) {
    // Get /Resources
    foundation::objects::Dict resources = ResolveResources(page_dict, dump);

    // Get /Contents
    const auto* contents_entry = FindDictEntry(page_dict, "Contents");
    if (!contents_entry) {
        return ""; // No content stream
    }

    // Capture the Ref identity BEFORE resolution so the single-
    // Stream branch can pass the right per-object key to the
    // decryptor. The Array branch captures per-item identity in
    // the loop below.
    auto [contents_obj_id, contents_gen] = RefIdentity(contents_entry->second);
    foundation::objects::Value contents_val = ResolveValue(contents_entry->second, dump);

    std::vector<std::byte> content_bytes;

    if (std::holds_alternative<foundation::objects::Stream>(contents_val.v)) {
        const foundation::objects::Stream& stream = std::get<foundation::objects::Stream>(contents_val.v);
        content_bytes = ResolveStreamBytes(
            stream, dump, pdf_bytes, crypto,
            contents_obj_id, contents_gen);
    } else if (std::holds_alternative<foundation::objects::Array>(contents_val.v)) {
        const foundation::objects::Array& arr = std::get<foundation::objects::Array>(contents_val.v);
        for (const auto& item : arr.items) {
            auto [item_obj_id, item_gen] = RefIdentity(item);
            foundation::objects::Value resolved = ResolveValue(item, dump);
            if (!std::holds_alternative<foundation::objects::Stream>(resolved.v)) {
                throw std::runtime_error("/Contents array contains non-Stream value");
            }
            const foundation::objects::Stream& stream = std::get<foundation::objects::Stream>(resolved.v);
            std::vector<std::byte> stream_bytes = ResolveStreamBytes(
                stream, dump, pdf_bytes, crypto,
                item_obj_id, item_gen);
            content_bytes.insert(content_bytes.end(), stream_bytes.begin(), stream_bytes.end());
        }
    } else {
        throw std::runtime_error("/Contents is not a Stream or Array");
    }
    
    // Parse content stream
    foundation::content_stream::Stream content_stream = foundation::content_stream::Parse({content_bytes.data(), content_bytes.size()});
    
    // Process operations
    std::string page_text;
    std::optional<std::string> current_font_name;
    std::optional<foundation::to_unicode::ToUnicode> current_cmap;
    std::optional<FontEncoding> current_encoding;

    // Text-positioning state for baseline-aware line breaking
    // (PDF 32000 §9.4.2). A newline is emitted before a fragment
    // only when its text-line origin y differs from the previously
    // shown fragment's, so a visual line an authoring tool split
    // across many same-baseline BT/ET blocks joins into one line,
    // while distinct baselines stay on separate lines.
    double tm[6]  = {1, 0, 0, 1, 0, 0};
    double tlm[6] = {1, 0, 0, 1, 0, 0};
    double leading = 0.0;
    std::optional<double> last_shown_y;

    auto num = [](const foundation::objects::Value& v) -> double {
        if (std::holds_alternative<double>(v.v)) {
            return std::get<double>(v.v);
        }
        if (std::holds_alternative<std::int64_t>(v.v)) {
            return static_cast<double>(std::get<std::int64_t>(v.v));
        }
        return 0.0;
    };
    auto set_line_matrix = [&](double a, double b, double c,
                               double d, double e, double f) {
        tlm[0] = a; tlm[1] = b; tlm[2] = c;
        tlm[3] = d; tlm[4] = e; tlm[5] = f;
        for (int i = 0; i < 6; ++i) tm[i] = tlm[i];
    };
    // Td: tlm = [1 0 0 1 tx ty] x tlm, then tm = tlm (PDF §9.4.2).
    auto translate_line = [&](double tx, double ty) {
        double ne = tx * tlm[0] + ty * tlm[2] + tlm[4];
        double nf = tx * tlm[1] + ty * tlm[3] + tlm[5];
        tlm[4] = ne; tlm[5] = nf;
        for (int i = 0; i < 6; ++i) tm[i] = tlm[i];
    };
    // Emit a line break before the next fragment when the baseline
    // origin moved since the last shown fragment; then record the
    // current y. Same-y fragments join with no separator.
    auto break_if_new_line = [&]() {
        const double y = tm[5];
        if (last_shown_y && std::abs(y - *last_shown_y) > 1e-3) {
            if (!page_text.empty() && page_text.back() != '\n') {
                page_text.push_back('\n');
            }
        }
        last_shown_y = y;
    };

    for (const auto& op : content_stream.operations) {
        if (op.op == "BT") {
            // Begin text: reset matrices to identity (PDF §9.4.1).
            // last_shown_y persists so baselines carry across blocks.
            set_line_matrix(1, 0, 0, 1, 0, 0);
        } else if (op.op == "ET") {
            // No newline on ET — line breaks are baseline-driven.
        } else if (op.op == "Tm") {
            if (op.operands.size() >= 6) {
                set_line_matrix(num(op.operands[0]), num(op.operands[1]),
                                num(op.operands[2]), num(op.operands[3]),
                                num(op.operands[4]), num(op.operands[5]));
            }
        } else if (op.op == "Td") {
            if (op.operands.size() >= 2) {
                translate_line(num(op.operands[0]), num(op.operands[1]));
            }
        } else if (op.op == "TD") {
            if (op.operands.size() >= 2) {
                leading = -num(op.operands[1]);
                translate_line(num(op.operands[0]), num(op.operands[1]));
            }
        } else if (op.op == "T*") {
            translate_line(0.0, -leading);
        } else if (op.op == "TL") {
            if (!op.operands.empty()) leading = num(op.operands[0]);
        } else if (op.op == "Tf") {
            if (op.operands.size() < 2) continue;

            // Tf first operand is a Name (std::string), NOT String
            if (!std::holds_alternative<std::string>(op.operands[0].v)) continue;

            current_font_name = std::get<std::string>(op.operands[0].v);
            current_cmap.reset();
            current_encoding.reset();

            foundation::objects::Dict font_dict;
            try {
                font_dict = GetFontDict(resources, *current_font_name, dump);
            } catch (...) {
                continue;
            }

            // /ToUnicode wins per §9.10.2; /Encoding (with
            // /Differences) is the secondary fallback.
            try {
                current_cmap = GetToUnicode(font_dict, dump, pdf_bytes, crypto);
            } catch (...) {
                current_encoding = GetFontEncoding(font_dict, dump);
            }
        } else if (op.op == "Tj" || op.op == "'" || op.op == "\"") {
            if (op.operands.empty()) continue;

            // ' and " move to the next line before showing
            // (PDF §9.4.3); modelling that as a T* lets the
            // baseline check below produce the line break.
            if (op.op == "'" || op.op == "\"") {
                translate_line(0.0, -leading);
            }

            // For " the show-string is the THIRD operand (aw ac
            // string); Tj and ' carry it as the first.
            const foundation::objects::Value& sv =
                (op.op == "\"") ? op.operands.back() : op.operands[0];

            if (std::holds_alternative<foundation::objects::String>(sv.v)) {
                break_if_new_line();
                const foundation::objects::String& str = std::get<foundation::objects::String>(sv.v);
                std::string decoded = DecodeString(str, current_cmap ? &*current_cmap : nullptr, current_encoding ? &*current_encoding : nullptr);
                page_text += decoded;
            }
        } else if (op.op == "TJ") {
            if (op.operands.empty() || !std::holds_alternative<foundation::objects::Array>(op.operands[0].v)) continue;

            const foundation::objects::Array& arr = std::get<foundation::objects::Array>(op.operands[0].v);
            bool any_text = false;

            for (const auto& item : arr.items) {
                if (std::holds_alternative<foundation::objects::String>(item.v)) {
                    if (!any_text) {
                        break_if_new_line();
                    }

                    const foundation::objects::String& str = std::get<foundation::objects::String>(item.v);
                    std::string decoded = DecodeString(str, current_cmap ? &*current_cmap : nullptr, current_encoding ? &*current_encoding : nullptr);
                    page_text += decoded;
                    any_text = true;
                }
            }
        }
        // Other operators are ignored
    }
    
    // Strip trailing whitespace
    while (!page_text.empty() && (page_text.back() == ' ' || page_text.back() == '\t' || page_text.back() == '\n' || page_text.back() == '\r')) {
        page_text.pop_back();
    }
    
    return page_text;
}

// Helper: find leaf pages in /Pages-tree preorder
void FindLeafPages(const foundation::objects::Dict& pages_dict, const foundation::objects::Dump& dump, std::vector<foundation::objects::Dict>& pages) {
    // Check /Type
    const auto* type_entry = FindDictEntry(pages_dict, "Type");
    bool is_pages = false;
    if (type_entry) {
        foundation::objects::Value type_val = ResolveValue(type_entry->second, dump);
        if (std::holds_alternative<std::string>(type_val.v) && std::get<std::string>(type_val.v) == "Pages") {
            is_pages = true;
        }
    }
    
    if (is_pages) {
        // Get /Kids
        const auto* kids_entry = FindDictEntry(pages_dict, "Kids");
        if (kids_entry) {
            foundation::objects::Value kids_val = ResolveValue(kids_entry->second, dump);
            if (std::holds_alternative<foundation::objects::Array>(kids_val.v)) {
                const foundation::objects::Array& kids = std::get<foundation::objects::Array>(kids_val.v);
                for (const auto& kid : kids.items) {
                    foundation::objects::Value resolved = ResolveValue(kid, dump);
                    if (std::holds_alternative<foundation::objects::Dict>(resolved.v)) {
                        FindLeafPages(std::get<foundation::objects::Dict>(resolved.v), dump, pages);
                    }
                }
            }
        }
    } else {
        // This is a leaf /Page
        pages.push_back(pages_dict);
    }
}

// Helper: extract text from all pages
std::vector<std::string> ExtractTextFromPages(
        const foundation::objects::Dump& dump,
        std::span<const std::byte> pdf_bytes,
        const CryptoCtx& crypto) {
    std::vector<std::string> result;
    
    // Find the root /Pages object
    // Look for trailer dict and find /Root
    foundation::objects::Dict root_dict;
    bool found_root = false;
    
    // Scan for trailer dict (last one in the dump)
    for (const auto& obj : dump.objects) {
        if (std::holds_alternative<foundation::objects::Dict>(obj.value.v)) {
            const foundation::objects::Dict& dict = std::get<foundation::objects::Dict>(obj.value.v);
            const auto* type_entry = FindDictEntry(dict, "Type");
            if (type_entry) {
                foundation::objects::Value type_val = ResolveValue(type_entry->second, dump);
                if (std::holds_alternative<std::string>(type_val.v) && std::get<std::string>(type_val.v) == "Catalog") {
                    const auto* root_entry = FindDictEntry(dict, "Pages");
                    if (root_entry) {
                        foundation::objects::Value pages_val = ResolveValue(root_entry->second, dump);
                        if (std::holds_alternative<foundation::objects::Dict>(pages_val.v)) {
                            root_dict = std::get<foundation::objects::Dict>(pages_val.v);
                            found_root = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if (!found_root) {
        return result; // No catalog found
    }
    
    // Find leaf pages in preorder
    std::vector<foundation::objects::Dict> pages;
    FindLeafPages(root_dict, dump, pages);
    
    // Extract text from each page
    for (const auto& page : pages) {
        result.push_back(ExtractTextFromPage(page, dump, pdf_bytes, crypto));
    }
    
    return result;
}

} // namespace

namespace foundation::text_extractor {

std::vector<std::string> Parse(std::span<const std::byte> pdf_bytes) {
    foundation::objects::Dump dump = foundation::objects::Parse(pdf_bytes);

    // Reject encrypted PDFs on the password-less Parse() entry.
    // The scan is broader than spec-strict (any dict carrying
    // /Encrypt, not just the trailer dict) but it has caught
    // every encrypted fixture in the corpus to date and avoids
    // a separate trailer-parse round trip. Callers that DO have
    // a file_key call ParseWithKey instead.
    for (const auto& obj : dump.objects) {
        if (std::holds_alternative<foundation::objects::Dict>(obj.value.v)) {
            const foundation::objects::Dict& dict = std::get<foundation::objects::Dict>(obj.value.v);
            const auto* encrypt_entry = FindDictEntry(dict, "Encrypt");
            if (encrypt_entry) {
                throw std::runtime_error(
                    "Encrypted PDF not supported by Parse; "
                    "call ParseWithKey with the file_key");
            }
        }
    }

    return ExtractTextFromPages(dump, pdf_bytes, kPlaintext);
}

std::vector<std::string> ParseWithKey(
        std::span<const std::byte> pdf_bytes,
        std::span<const std::byte> file_key) {
    // V=4 R=4 AES-128 mandates a 16-byte file_key (PDF 32000
    // §7.6.4.3 Algorithm 2). Reject other lengths up-front so
    // callers see a clear "use Parse for plaintext / supply a
    // 16-byte key for V=4 AES" message rather than an indirect
    // flate failure when decryption is silently skipped.
    if (file_key.size() != 16) {
        throw std::runtime_error(
            "ParseWithKey requires a 16-byte file_key (V=4 R=4 "
            "AES-128); call Parse for plaintext PDFs");
    }
    foundation::objects::Dump dump = foundation::objects::Parse(pdf_bytes);
    CryptoCtx crypto{file_key};
    return ExtractTextFromPages(dump, pdf_bytes, crypto);
}

std::string ToCanonical(const std::vector<std::string>& pages) {
    if (pages.empty()) {
        return "";
    }
    
    if (pages.size() == 1) {
        return pages[0] + "\n";
    }
    
    std::string result;
    for (size_t i = 0; i < pages.size(); ++i) {
        if (i > 0) {
            result.push_back(0x0C); // Form feed
        }
        result += pages[i];
    }
    result.push_back('\n');
    
    return result;
}

} // namespace foundation::text_extractor
