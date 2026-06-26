// =============================================================================
// foundation::page_renderer — body. Recovered from
// b322b22^:src/public/software_page_renderer.cpp (1955 LOC) and adapted
// by:
//   * outer namespace public-API → foundation
//   * SoftwarePageRenderer class wrapping → free function Render(...)
//   * RgbaBuffer return type → RenderedPage struct (foundation header)
//   * PageRendererException(kind, msg) → std::runtime_error /
//     std::invalid_argument / std::out_of_range per the spec
//
// Operator dispatch + content-stream walk + graphics state + text + image
// XObject logic unchanged — every call into a foundation primitive
// already used the namespaced foundation::* form, so this body composes
// at the foundation layer cleanly.
// =============================================================================

#include "page_renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "agl.hpp"
#include "ascii85.hpp"
#include "asciihex.hpp"
#include "ccitt.hpp"
#include "cff.hpp"
#include "content_stream.hpp"
#include "dctdecode.hpp"
#include "jpx.hpp"
#include "encoding_tables.hpp"
#include "flate.hpp"
#include "lzw.hpp"
#include "runlength.hpp"
#include "glyph_rasterizer.hpp"
#include "image_compositor.hpp"
#include "jbig2.hpp"
#include "objects.hpp"
#include "pages_tree.hpp"
#include "rasterizer.hpp"
#include "standard14_outlines.hpp"
#include "transform.hpp"
#include "truetype.hpp"

namespace foundation::page_renderer {
namespace {

using foundation::content_stream::Operation;
using foundation::content_stream::Stream;
using foundation::objects::Dump;
using foundation::objects::IndirectObject;
using foundation::objects::Value;
using foundation::rasterizer::FillRule;
using foundation::rasterizer::Path;
using foundation::rasterizer::PathSegment;
using foundation::rasterizer::Raster;
using foundation::rasterizer::SegmentKind;
using foundation::transform::Compose;
using foundation::transform::Matrix;
using foundation::transform::Point;

const IndirectObject* FindObject(const Dump& dump,
                                 std::uint32_t id) {
    for (const auto& obj : dump.objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

const Value* Resolve(const Dump& dump, const Value& v) {
    if (const auto* r = std::get_if<foundation::objects::Ref>(
            &v.v)) {
        const auto* obj = FindObject(dump, r->id);
        return obj ? &obj->value : nullptr;
    }
    return &v;
}

const Value* DictGet(const foundation::objects::Dict& d,
                     const std::string& key) {
    for (const auto& [k, v] : d.entries) {
        if (k == key) return &v;
    }
    return nullptr;
}

double AsNumber(const Value& v, double dflt = 0.0) {
    if (const auto* i = std::get_if<std::int64_t>(&v.v)) {
        return static_cast<double>(*i);
    }
    if (const auto* d = std::get_if<double>(&v.v)) return *d;
    return dflt;
}

// DeviceCMYK → sRGB via the colorspace primitive (§8.6.4.4).
foundation::colorspace::ColorRgb CmykToRgb(double c, double m,
                                           double y, double k) {
    const double comp[4] = {c, m, y, k};
    return foundation::colorspace::ToSrgb(
        foundation::colorspace::Family::DeviceCMYK,
        std::span<const double>(comp, 4));
}

// Interpret the numeric operands of an sc / scn / SC / SCN colour
// operator by component count: 1 → DeviceGray, 3 → DeviceRGB,
// 4 → DeviceCMYK. Returns nullopt when the operands are not a
// pure run of 1/3/4 numbers (e.g. a /Pattern name), leaving the
// caller's current colour untouched.
std::optional<foundation::colorspace::ColorRgb> NumericColor(
        const std::vector<Value>& a) {
    if (a.empty() || a.size() > 4) return std::nullopt;
    for (const auto& v : a) {
        if (!std::holds_alternative<std::int64_t>(v.v)
                && !std::holds_alternative<double>(v.v)) {
            return std::nullopt;
        }
    }
    if (a.size() == 1) {
        const double g = std::clamp(AsNumber(a[0]), 0.0, 1.0);
        return foundation::colorspace::ColorRgb{g, g, g};
    }
    if (a.size() == 3) {
        return foundation::colorspace::ColorRgb{
            std::clamp(AsNumber(a[0]), 0.0, 1.0),
            std::clamp(AsNumber(a[1]), 0.0, 1.0),
            std::clamp(AsNumber(a[2]), 0.0, 1.0)};
    }
    if (a.size() == 4) {
        return CmykToRgb(AsNumber(a[0]), AsNumber(a[1]),
                         AsNumber(a[2]), AsNumber(a[3]));
    }
    return std::nullopt;
}

// Decode a /Contents stream's body — supports raw (no /Filter)
// and /FlateDecode. Unknown filters return an empty buffer
// (consistent with the v1 silent-skip-unsupported policy).
std::vector<std::byte> DecodeStream(
        std::span<const std::byte> pdf_bytes,
        const foundation::objects::Stream& stream) {
    // stream.body is a span into the parsed source buffer
    // (parser bounds-checked at parse time). Suppress the
    // unused pdf_bytes parameter — kept on the signature for
    // call-site compatibility, may be removed in a future
    // cleanup.
    (void)pdf_bytes;
    std::span<const std::byte> body = stream.body;

    const auto* filter_v = DictGet(stream.header, "Filter");
    if (!filter_v) {
        return std::vector<std::byte>(body.begin(), body.end());
    }
    // Decode one filter of a content / font / general stream's chain.
    // The image-only codecs (DCT / CCITT / JBIG2 / JPX) are NOT handled
    // here — they are driven by BuildResolvedImageBase; an unsupported
    // filter yields empty so the caller treats the stream as absent.
    auto apply_one = [&](const std::string& name,
                         std::vector<std::byte> data)
                        -> std::vector<std::byte> {
        const std::span<const std::byte> in(data.data(), data.size());
        try {
            if (name == "FlateDecode" || name == "Fl") {
                return foundation::flate::Decode(in);
            }
            if (name == "ASCII85Decode" || name == "A85") {
                // ascii85::Decode wants the body without the "~>" EOD.
                std::span<const std::byte> body85 = in;
                for (std::size_t i = 0; i + 1 < data.size(); ++i) {
                    if (data[i] == std::byte{'~'} &&
                        data[i + 1] == std::byte{'>'}) {
                        body85 = std::span<const std::byte>(
                            data.data(), i);
                        break;
                    }
                }
                return foundation::ascii85::Decode(body85);
            }
            if (name == "ASCIIHexDecode" || name == "AHx") {
                return foundation::asciihex::Decode(in);
            }
            if (name == "LZWDecode" || name == "LZW") {
                // /DecodeParms /EarlyChange defaults to 1 (PDF). Some
                // producers emit TIFF-style EarlyChange 0 LZW without
                // declaring it (a wrong EarlyChange throws "code out of
                // range"), so retry the other convention on a decode
                // error before giving up (field reproducer 34203.pdf).
                try {
                    return foundation::lzw::Decode(
                        in, foundation::lzw::EarlyChange::Pdf);
                } catch (...) {
                    return foundation::lzw::Decode(
                        in, foundation::lzw::EarlyChange::Tiff);
                }
            }
            if (name == "RunLengthDecode" || name == "RL") {
                return foundation::runlength::Decode(in);
            }
        } catch (...) {
            return std::vector<std::byte>{};
        }
        return std::vector<std::byte>{};
    };
    std::vector<std::byte> data(body.begin(), body.end());
    if (const auto* nm = std::get_if<std::string>(&filter_v->v)) {
        return apply_one(*nm, std::move(data));
    }
    if (const auto* arr = std::get_if<
            foundation::objects::Array>(&filter_v->v)) {
        for (const auto& f : arr->items) {
            const auto* fname = std::get_if<std::string>(&f.v);
            if (!fname) return {};
            data = apply_one(*fname, std::move(data));
        }
        return data;
    }
    return {};
}

std::vector<std::byte> CollectContents(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& page_dict) {
    const auto* contents_v = DictGet(page_dict, "Contents");
    if (!contents_v) return {};

    std::vector<std::byte> out;
    auto push_one = [&](const Value& v) {
        const auto* resolved = Resolve(dump, v);
        if (!resolved) return;
        if (const auto* st = std::get_if<
                foundation::objects::Stream>(&resolved->v)) {
            auto decoded = DecodeStream(pdf_bytes, *st);
            out.insert(out.end(), decoded.begin(),
                       decoded.end());
            out.push_back(static_cast<std::byte>('\n'));
        }
    };
    const auto* root = Resolve(dump, *contents_v);
    if (!root) return {};
    if (const auto* arr = std::get_if<
            foundation::objects::Array>(&root->v)) {
        for (const auto& item : arr->items) push_one(item);
    } else if (std::holds_alternative<
                   foundation::objects::Stream>(root->v)) {
        push_one(*contents_v);
    }
    return out;
}

// Resolve a /Resources dict on a page leaf, walking the
// /Pages parent chain when /Resources is absent (PDF
// §7.7.3.4 — /Resources is inheritable). Returns nullptr if
// no /Resources dict is reachable.
const foundation::objects::Dict* ResolveResources(
        const Dump& dump,
        const foundation::objects::Dict& page_dict) {
    const foundation::objects::Dict* current = &page_dict;
    int hop_budget = 32;
    while (current && hop_budget-- > 0) {
        if (const auto* r = DictGet(*current, "Resources")) {
            if (const auto* res = Resolve(dump, *r)) {
                if (const auto* d = std::get_if<
                        foundation::objects::Dict>(&res->v)) {
                    return d;
                }
            }
        }
        const auto* parent_v = DictGet(*current, "Parent");
        if (!parent_v) break;
        const auto* parent_resolved = Resolve(dump, *parent_v);
        if (!parent_resolved) break;
        current = std::get_if<foundation::objects::Dict>(
            &parent_resolved->v);
    }
    return nullptr;
}

// One font program plus the encoding glue needed to translate
// a PDF char_code to a glyph_id + outline. Built once per
// RenderPage call and indexed by /Resources/Font resource name.
struct ResolvedFont {
    enum class Kind { None, TrueType, Cff } kind = Kind::None;
    foundation::truetype::TrueType tt{};
    foundation::cff::Cff cff{};
    std::uint32_t units_per_em = 1000;
    bool is_cid = false;          // Type0 + CID-keyed descendant
    // CIDFontType2 /CIDToGIDMap, indexed by CID, value = glyph id.
    // Empty ⇒ Identity mapping (GID == CID), the PDF default and
    // the value of a /CIDToGIDMap /Identity name. A non-empty
    // vector comes from a /CIDToGIDMap stream (2 bytes big-endian
    // per CID); CIDs past its end map to GID 0 (.notdef).
    std::vector<std::uint16_t> cid_to_gid{};
    foundation::encoding_tables::Encoding base_encoding =
        foundation::encoding_tables::Encoding::WinAnsiEncoding;
    // /Differences override slots; empty entries mean "no
    // override at this byte". Indexed 0..255.
    std::array<std::string, 256> diff{};
};

// Decode a font-program /FontFile / /FontFile2 / /FontFile3
// stream body via /Filter /FlateDecode. Returns the empty
// vector when no recognisable /FontFile* is present or the
// filter chain can't be decoded.
std::vector<std::byte> ExtractProgramBytes(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& descriptor,
        ResolvedFont::Kind* kind_out) {
    auto pull = [&](const std::string& key)
            -> const foundation::objects::Stream* {
        const auto* v = DictGet(descriptor, key);
        if (!v) return nullptr;
        const auto* r = Resolve(dump, *v);
        if (!r) return nullptr;
        return std::get_if<foundation::objects::Stream>(&r->v);
    };
    if (const auto* s = pull("FontFile2")) {
        if (kind_out) *kind_out = ResolvedFont::Kind::TrueType;
        return DecodeStream(pdf_bytes, *s);
    }
    if (const auto* s = pull("FontFile3")) {
        if (kind_out) {
            // Default to Cff; flip to None for OpenType wrappers
            // (CFF-table extraction is a future foundation beat).
            *kind_out = ResolvedFont::Kind::Cff;
            if (const auto* sub = DictGet(s->header, "Subtype")) {
                if (const auto* sn =
                        std::get_if<std::string>(&sub->v)) {
                    if (*sn == "OpenType") {
                        *kind_out = ResolvedFont::Kind::None;
                    }
                }
            }
        }
        return DecodeStream(pdf_bytes, *s);
    }
    // /FontFile (raw Type 1 PostScript) — out of v1 scope.
    if (kind_out) *kind_out = ResolvedFont::Kind::None;
    return {};
}

// Walk a font_dict's /Encoding entry into base + diff slots.
// Defaults: TrueType → WinAnsi; everything else → Standard.
// /Encoding may be a Name or a dict with /BaseEncoding +
// /Differences. /Differences is an array alternating
// integer-slot-numbers with name-tokens; name tokens after a
// slot consume successive bytes starting at that slot.
void ResolveEncoding(const Dump& dump,
                     const foundation::objects::Dict& font_dict,
                     const std::string& subtype,
                     ResolvedFont& out) {
    out.base_encoding =
        (subtype == "TrueType")
            ? foundation::encoding_tables::Encoding::WinAnsiEncoding
            : foundation::encoding_tables::Encoding::StandardEncoding;
    const auto* enc_v = DictGet(font_dict, "Encoding");
    if (!enc_v) return;
    const auto* resolved = Resolve(dump, *enc_v);
    if (!resolved) return;
    if (const auto* name =
            std::get_if<std::string>(&resolved->v)) {
        if (auto e =
                foundation::encoding_tables::EncodingFromName(
                    *name)) {
            out.base_encoding = *e;
        }
        return;
    }
    const auto* dict =
        std::get_if<foundation::objects::Dict>(&resolved->v);
    if (!dict) return;
    if (const auto* base_v = DictGet(*dict, "BaseEncoding")) {
        if (const auto* bn =
                std::get_if<std::string>(&base_v->v)) {
            if (auto e =
                    foundation::encoding_tables::EncodingFromName(
                        *bn)) {
                out.base_encoding = *e;
            }
        }
    }
    const auto* diff_v = DictGet(*dict, "Differences");
    if (!diff_v) return;
    const auto* arr =
        std::get_if<foundation::objects::Array>(&diff_v->v);
    if (!arr) return;
    int slot = -1;
    for (const auto& item : arr->items) {
        if (const auto* code =
                std::get_if<std::int64_t>(&item.v)) {
            slot = static_cast<int>(*code);
        } else if (const auto* nm =
                       std::get_if<std::string>(&item.v)) {
            if (slot >= 0 && slot < 256) {
                out.diff[static_cast<std::size_t>(slot)] = *nm;
                ++slot;
            }
        }
    }
}

std::optional<std::string> NameValue(const Value* v) {
    if (!v) return std::nullopt;
    if (const auto* n = std::get_if<std::string>(&v->v)) {
        return *n;
    }
    return std::nullopt;
}

// Build a ResolvedFont from a single /Resources/Font entry.
// Returns Kind::None if the font isn't supported by v1
// (e.g. Type 3 fonts, /FontFile /Type1 PostScript programs,
// or /FontFile3 /OpenType wrappers).
ResolvedFont BuildResolvedFont(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& font_dict) {
    ResolvedFont rf;
    const auto subtype =
        NameValue(DictGet(font_dict, "Subtype")).value_or("");

    // For Type0 (CID), the descriptor + program live on the
    // descendant; chase /DescendantFonts[0] before reading.
    const foundation::objects::Dict* prog_dict = &font_dict;
    if (subtype == "Type0") {
        rf.is_cid = true;
        const auto* dfs_v =
            DictGet(font_dict, "DescendantFonts");
        if (dfs_v) {
            if (const auto* r = Resolve(dump, *dfs_v)) {
                if (const auto* arr = std::get_if<
                        foundation::objects::Array>(&r->v)) {
                    if (!arr->items.empty()) {
                        if (const auto* d0 = Resolve(
                                dump, arr->items[0])) {
                            if (const auto* dd = std::get_if<
                                    foundation::objects::Dict>(
                                        &d0->v)) {
                                prog_dict = dd;
                            }
                        }
                    }
                }
            }
        }
        // /CIDToGIDMap on the descendant: a /Identity name (or
        // absence) keeps the Identity default (empty vector); a
        // stream carries 2-byte big-endian GID per CID.
        if (const auto* c2g_v =
                DictGet(*prog_dict, "CIDToGIDMap")) {
            if (const auto* c2g = Resolve(dump, *c2g_v)) {
                if (const auto* st = std::get_if<
                        foundation::objects::Stream>(&c2g->v)) {
                    const auto raw = DecodeStream(pdf_bytes, *st);
                    rf.cid_to_gid.reserve(raw.size() / 2);
                    for (std::size_t i = 0; i + 1 < raw.size();
                         i += 2) {
                        const auto hi = std::to_integer<
                            std::uint16_t>(raw[i]);
                        const auto lo = std::to_integer<
                            std::uint16_t>(raw[i + 1]);
                        rf.cid_to_gid.push_back(
                            static_cast<std::uint16_t>(
                                (hi << 8) | lo));
                    }
                }
            }
        }
    }

    // Try the embedded /FontDescriptor /FontFile* path first.
    // For Standard14 references (typical Type1 entry like
    // `<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>`)
    // the font dict often has NO FontDescriptor at all — the
    // reader is expected to supply outlines per PDF §9.6.2.2.
    // Fall through to the Standard14 lookup in that case too.
    ResolvedFont::Kind kind = ResolvedFont::Kind::None;
    std::vector<std::byte> bytes;
    bool desc_serif = false;  // /FontDescriptor /Flags serif bit (§9.8.2)

    const auto* desc_v = DictGet(*prog_dict, "FontDescriptor");
    if (desc_v) {
        if (const auto* desc_resolved = Resolve(dump, *desc_v)) {
            if (const auto* descriptor = std::get_if<
                    foundation::objects::Dict>(&desc_resolved->v)) {
                bytes = ExtractProgramBytes(
                    pdf_bytes, dump, *descriptor, &kind);
                if (const auto* fl = DictGet(*descriptor, "Flags")) {
                    const auto flags =
                        static_cast<std::uint32_t>(AsNumber(*fl));
                    desc_serif = (flags & 0x02u) != 0u;  // bit 2 = Serif
                }
            }
        }
    }

    // Standard14 fallback. Fires whenever the embedded path didn't
    // produce a usable program — including PDFs that don't carry a
    // FontDescriptor at all, the common shape for /BaseFont
    // /Helvetica references.
    if (kind == ResolvedFont::Kind::None || bytes.empty()) {
        if (subtype != "Type0") {
            const auto base_font_name =
                NameValue(DictGet(font_dict, "BaseFont")).value_or("");
            // Map any non-embedded name to a Standard14 substitute
            // (Arial→Helvetica, TimesNewRoman→Times-Roman, Courier
            // New→Courier, …); an unrecognised family falls back to
            // Helvetica unless the descriptor's serif flag is set.
            // Without this a non-Standard14 non-embedded font resolves
            // to nothing and its text renders blank (§9.6.2.2).
            const std::string canonical =
                foundation::standard14_outlines::Canonicalize(
                    base_font_name, desc_serif);
            bytes = foundation::standard14_outlines::Resolve(canonical);
            if (!bytes.empty()) {
                kind = ResolvedFont::Kind::TrueType;
            }
        }
        if (kind == ResolvedFont::Kind::None || bytes.empty()) {
            return rf;
        }
    }
    std::span<const std::byte> bspan(bytes.data(), bytes.size());
    try {
        if (kind == ResolvedFont::Kind::TrueType) {
            rf.tt = foundation::truetype::Parse(bspan);
            rf.kind = ResolvedFont::Kind::TrueType;
            rf.units_per_em = rf.tt.units_per_em;
        } else if (kind == ResolvedFont::Kind::Cff) {
            rf.cff = foundation::cff::Parse(bspan);
            rf.kind = ResolvedFont::Kind::Cff;
            rf.units_per_em = rf.cff.units_per_em;
        }
    } catch (...) {
        rf.kind = ResolvedFont::Kind::None;
    }
    if (rf.kind != ResolvedFont::Kind::None) {
        ResolveEncoding(dump, font_dict, subtype, rf);
    }
    return rf;
}

// Resolve every /Resources/Font entry into a ResolvedFont,
// keyed by resource name. Empty result means the page draws
// no text or only references unresolvable fonts.
std::map<std::string, ResolvedFont> ResolveFontsFromResources(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& resources) {
    std::map<std::string, ResolvedFont> fonts;
    const auto* font_v = DictGet(resources, "Font");
    if (!font_v) return fonts;
    const auto* font_resolved = Resolve(dump, *font_v);
    if (!font_resolved) return fonts;
    const auto* font_dict = std::get_if<
        foundation::objects::Dict>(&font_resolved->v);
    if (!font_dict) return fonts;
    for (const auto& [name, ref] : font_dict->entries) {
        const auto* fv = Resolve(dump, ref);
        if (!fv) continue;
        const auto* fd = std::get_if<
            foundation::objects::Dict>(&fv->v);
        if (!fd) continue;
        fonts.emplace(name, BuildResolvedFont(pdf_bytes, dump, *fd));
    }
    return fonts;
}

std::map<std::string, ResolvedFont> ResolvePageFonts(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& page_dict) {
    const auto* resources = ResolveResources(dump, page_dict);
    if (!resources) return {};
    return ResolveFontsFromResources(pdf_bytes, dump, *resources);
}

// Resolve a single byte char_code through a ResolvedFont's
// /Encoding + /Differences chain to a glyph name. Returns
// empty string when the byte is unmapped.
std::string ByteToGlyphName(const ResolvedFont& rf,
                            std::uint8_t b) {
    if (!rf.diff[b].empty()) return rf.diff[b];
    auto sv = foundation::encoding_tables::ByteToGlyphName(
        rf.base_encoding, b);
    return std::string(sv);
}

// Find a CFF glyph by name (linear scan).
std::optional<std::uint32_t> CffGidByName(
        const foundation::cff::Cff& cff,
        std::string_view name) {
    if (name.empty()) return std::nullopt;
    for (std::size_t i = 0; i < cff.glyphs.size(); ++i) {
        if (cff.glyphs[i].name == name) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return std::nullopt;
}

// Reverse-lookup a Unicode codepoint through the TrueType
// cmap (small table; linear scan).
std::optional<std::uint32_t> TtGidByCodepoint(
        const foundation::truetype::TrueType& tt,
        std::uint32_t cp) {
    for (const auto& e : tt.cmap) {
        if (e.char_code == cp) return e.glyph_id;
    }
    return std::nullopt;
}

// Resolve a single byte char_code through a ResolvedFont to
// (gid, advance_in_funits). Returns nullopt when the byte
// can't be mapped to any glyph.
struct GlyphLookup {
    std::uint32_t gid;
    std::int32_t advance_funits;
};

std::optional<GlyphLookup> ResolveByteToGlyph(
        const ResolvedFont& rf, std::uint8_t b) {
    if (rf.kind == ResolvedFont::Kind::Cff) {
        if (rf.is_cid) {
            // Identity-H equivalence: byte = CID directly. v1
            // doesn't decode CMaps; the renderer treats Type0
            // single-byte sequences as CID-numbered.
            for (const auto& g : rf.cff.glyphs) {
                if (g.cid == static_cast<std::uint32_t>(b)) {
                    return GlyphLookup{
                        g.gid, g.advance};
                }
            }
            return std::nullopt;
        }
        const auto name = ByteToGlyphName(rf, b);
        auto gid = CffGidByName(rf.cff, name);
        if (!gid) return std::nullopt;
        return GlyphLookup{*gid, rf.cff.glyphs[*gid].advance};
    }
    if (rf.kind == ResolvedFont::Kind::TrueType) {
        const auto name = ByteToGlyphName(rf, b);
        std::optional<std::uint32_t> gid;
        if (!name.empty()) {
            // Try AGL → cmap reverse first (handles "H",
            // "Aacute", etc.).
            const auto cps = foundation::agl::Lookup(name);
            if (!cps.empty()) {
                gid = TtGidByCodepoint(rf.tt, cps[0]);
            }
        }
        if (!gid) {
            // Fallback: cmap directly under the byte value
            // (covers Identity-style mappings where /Encoding
            // is absent and the cmap already keys by byte).
            gid = TtGidByCodepoint(rf.tt, b);
        }
        if (!gid) return std::nullopt;
        std::int32_t adv = 0;
        if (*gid < rf.tt.glyph_widths.size()) {
            adv = static_cast<std::int32_t>(
                rf.tt.glyph_widths[*gid]);
        }
        return GlyphLookup{*gid, adv};
    }
    return std::nullopt;
}

// Resolve a 16-bit CID (already decoded from a 2-byte Identity-H
// code) to (gid, advance_in_funits) for a Type0/CID font. The
// CID→GID step uses the font's /CIDToGIDMap (Identity when the
// map is empty); the advance comes from the embedded program's
// per-glyph width table so it stays in the same funits the
// glyph-CTM scale expects. Returns nullopt only when the font
// carries no usable program.
std::optional<GlyphLookup> ResolveCidToGlyph(
        const ResolvedFont& rf, std::uint16_t cid) {
    if (rf.kind == ResolvedFont::Kind::TrueType) {
        std::uint32_t gid = cid;  // Identity CIDToGIDMap default
        if (!rf.cid_to_gid.empty()) {
            gid = (cid < rf.cid_to_gid.size())
                      ? rf.cid_to_gid[cid]
                      : 0u;  // out of range ⇒ .notdef
        }
        std::int32_t adv = 0;
        if (gid < rf.tt.glyph_widths.size()) {
            adv = static_cast<std::int32_t>(
                rf.tt.glyph_widths[gid]);
        }
        return GlyphLookup{gid, adv};
    }
    if (rf.kind == ResolvedFont::Kind::Cff) {
        // CIDFont CFF: charset records the CID for each glyph.
        for (const auto& g : rf.cff.glyphs) {
            if (g.cid == static_cast<std::uint32_t>(cid)) {
                return GlyphLookup{g.gid, g.advance};
            }
        }
        // Fall back to treating the CID as a direct glyph index
        // (non-CIDFont CFF wrapped in a Type0, Identity ordering).
        if (cid < rf.cff.glyphs.size()) {
            return GlyphLookup{
                static_cast<std::uint32_t>(cid),
                rf.cff.glyphs[cid].advance};
        }
        return std::nullopt;
    }
    return std::nullopt;
}

// Lift a TrueType quadratic (P0, C, P1) into a cubic, append
// to the outline as a Curve segment whose endpoints are
// (C1, C2, P1).
void EmitQuadAsCubic(
        foundation::glyph_rasterizer::Outline& out,
        Point p0, Point c, Point p1) {
    const Point c1{p0.x + (2.0 / 3.0) * (c.x - p0.x),
                   p0.y + (2.0 / 3.0) * (c.y - p0.y)};
    const Point c2{p1.x + (2.0 / 3.0) * (c.x - p1.x),
                   p1.y + (2.0 / 3.0) * (c.y - p1.y)};
    foundation::glyph_rasterizer::OutlineSegment seg;
    seg.op = foundation::glyph_rasterizer::OutlineOp::Curve;
    seg.pts[0] = c1;
    seg.pts[1] = c2;
    seg.pts[2] = p1;
    out.segments.push_back(seg);
}

void EmitMove(foundation::glyph_rasterizer::Outline& out,
              Point p) {
    foundation::glyph_rasterizer::OutlineSegment seg;
    seg.op = foundation::glyph_rasterizer::OutlineOp::Move;
    seg.pts[0] = p;
    out.segments.push_back(seg);
}

void EmitLine(foundation::glyph_rasterizer::Outline& out,
              Point p) {
    foundation::glyph_rasterizer::OutlineSegment seg;
    seg.op = foundation::glyph_rasterizer::OutlineOp::Line;
    seg.pts[0] = p;
    out.segments.push_back(seg);
}

void EmitClose(foundation::glyph_rasterizer::Outline& out) {
    foundation::glyph_rasterizer::OutlineSegment seg;
    seg.op = foundation::glyph_rasterizer::OutlineOp::Close;
    out.segments.push_back(seg);
}

// Convert one TrueType glyph (points + on_curve + contour_ends
// + glyf flag bits) into a glyph_rasterizer::Outline. Lifts
// quadratic curves to cubics and inserts implicit on-curve
// midpoints between consecutive off-curve points (PDF spec
// §9.6.6 + TrueType Reference Manual chapter 5).
foundation::glyph_rasterizer::Outline BuildTtOutline(
        const foundation::truetype::TrueType& tt,
        std::uint32_t gid) {
    foundation::glyph_rasterizer::Outline out;
    if (gid >= tt.glyphs.size()) return out;
    const auto& g = tt.glyphs[gid];
    if (g.points.empty() || g.contour_ends.empty()) return out;

    std::size_t start = 0;
    for (std::uint32_t end_idx : g.contour_ends) {
        const std::size_t end =
            static_cast<std::size_t>(end_idx);
        if (end >= g.points.size() || end < start) {
            start = end + 1;
            continue;
        }
        const std::size_t n = end - start + 1;
        if (n < 1) {
            start = end + 1;
            continue;
        }
        // Find the first on-curve point in the contour. If the
        // contour has no on-curve points (rare but legal), the
        // synthetic start is the midpoint of the last and first
        // off-curve points.
        std::size_t first_on = start;
        bool has_on = false;
        for (std::size_t i = start; i <= end; ++i) {
            if (g.points[i].on_curve) {
                first_on = i;
                has_on = true;
                break;
            }
        }
        Point start_pt;
        if (has_on) {
            start_pt = Point{
                static_cast<double>(g.points[first_on].x),
                static_cast<double>(g.points[first_on].y)};
        } else {
            const auto& a = g.points[start];
            const auto& b = g.points[end];
            start_pt = Point{
                (static_cast<double>(a.x)
                 + static_cast<double>(b.x)) * 0.5,
                (static_cast<double>(a.y)
                 + static_cast<double>(b.y)) * 0.5};
            first_on = start;  // walk from the synthetic anchor
        }
        EmitMove(out, start_pt);

        Point prev_on = start_pt;
        std::optional<Point> pending_off;
        const std::size_t walk_n = has_on ? n : n + 1;

        for (std::size_t step = 1; step <= walk_n; ++step) {
            std::size_t idx;
            bool on;
            Point pt;
            if (step == walk_n) {
                // After the last contour point, close back to
                // the start anchor.
                idx = first_on;
                on = true;
                pt = start_pt;
            } else {
                idx = first_on + step;
                if (idx > end) idx = start + (idx - end - 1);
                on = g.points[idx].on_curve;
                pt = Point{
                    static_cast<double>(g.points[idx].x),
                    static_cast<double>(g.points[idx].y)};
            }

            if (on) {
                if (pending_off) {
                    EmitQuadAsCubic(out, prev_on, *pending_off,
                                    pt);
                    pending_off.reset();
                } else {
                    EmitLine(out, pt);
                }
                prev_on = pt;
            } else {
                if (pending_off) {
                    // Two consecutive off-curves — insert the
                    // implicit on-curve midpoint.
                    const Point mid{
                        (pending_off->x + pt.x) * 0.5,
                        (pending_off->y + pt.y) * 0.5};
                    EmitQuadAsCubic(out, prev_on, *pending_off,
                                    mid);
                    prev_on = mid;
                    pending_off = pt;
                } else {
                    pending_off = pt;
                }
            }
        }
        EmitClose(out);
        start = end + 1;
    }
    return out;
}

// Convert one CFF glyph charstring (already absolute cubics
// per Type 2 → cff::Cff Path conversion) into a
// glyph_rasterizer::Outline.
foundation::glyph_rasterizer::Outline BuildCffOutline(
        const foundation::cff::Cff& cff,
        std::uint32_t gid) {
    foundation::glyph_rasterizer::Outline out;
    if (gid >= cff.glyphs.size()) return out;
    const auto& g = cff.glyphs[gid];
    for (const auto& seg : g.path) {
        foundation::glyph_rasterizer::OutlineSegment os;
        switch (seg.op) {
            case foundation::cff::PathOp::M:
                if (seg.coords.size() < 2) break;
                os.op = foundation::glyph_rasterizer::OutlineOp::Move;
                os.pts[0] = Point{
                    static_cast<double>(seg.coords[0]),
                    static_cast<double>(seg.coords[1])};
                out.segments.push_back(os);
                break;
            case foundation::cff::PathOp::L:
                if (seg.coords.size() < 2) break;
                os.op = foundation::glyph_rasterizer::OutlineOp::Line;
                os.pts[0] = Point{
                    static_cast<double>(seg.coords[0]),
                    static_cast<double>(seg.coords[1])};
                out.segments.push_back(os);
                break;
            case foundation::cff::PathOp::C:
                if (seg.coords.size() < 6) break;
                os.op = foundation::glyph_rasterizer::OutlineOp::Curve;
                os.pts[0] = Point{
                    static_cast<double>(seg.coords[0]),
                    static_cast<double>(seg.coords[1])};
                os.pts[1] = Point{
                    static_cast<double>(seg.coords[2]),
                    static_cast<double>(seg.coords[3])};
                os.pts[2] = Point{
                    static_cast<double>(seg.coords[4]),
                    static_cast<double>(seg.coords[5])};
                out.segments.push_back(os);
                break;
            case foundation::cff::PathOp::Z:
                os.op = foundation::glyph_rasterizer::OutlineOp::Close;
                out.segments.push_back(os);
                break;
        }
    }
    return out;
}

// One image XObject pre-decoded into an RGBA8 buffer. Built once
// per RenderPage call at the page's /Resources/XObject pass and
// indexed by resource name (key in the resource dict). Drawn by
// the /Do operator via foundation::image_compositor.
struct ResolvedImage {
    bool valid = false;          // false ⇒ unsupported / failed; /Do skips
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::byte> rgba8; // exactly width * height * 4 bytes
};

// Decode one /FontFile-style raw bitmap (BPC=8) into RGBA8 given
// (width, height, components). Components 1 → grey broadcast to
// RGB; 3 → straight RGB→RGBA pad; 4 → CMYK out of v1 scope (caller
// records as None). Pixels are sized to width*height*components.
std::vector<std::byte> ExpandRawToRgba8(
        std::span<const std::byte> raw,
        std::uint32_t width, std::uint32_t height,
        std::uint32_t components) {
    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * height;
    std::vector<std::byte> out(pixel_count * 4u,
                               static_cast<std::byte>(255));
    if (components != 1 && components != 3 && components != 4) {
        return {};
    }
    if (raw.size() < pixel_count * components) return {};
    for (std::size_t i = 0; i < pixel_count; ++i) {
        if (components == 1) {
            const auto g = raw[i];
            out[i * 4 + 0] = g;
            out[i * 4 + 1] = g;
            out[i * 4 + 2] = g;
        } else if (components == 3) {
            out[i * 4 + 0] = raw[i * 3 + 0];
            out[i * 4 + 1] = raw[i * 3 + 1];
            out[i * 4 + 2] = raw[i * 3 + 2];
        } else {
            // DeviceCMYK sample → sRGB via the colorspace primitive
            // (§8.6.4.4 subtractive conversion).
            const double comp[4] = {
                std::to_integer<std::uint8_t>(raw[i * 4 + 0]) / 255.0,
                std::to_integer<std::uint8_t>(raw[i * 4 + 1]) / 255.0,
                std::to_integer<std::uint8_t>(raw[i * 4 + 2]) / 255.0,
                std::to_integer<std::uint8_t>(raw[i * 4 + 3]) / 255.0};
            const auto rgb = foundation::colorspace::ToSrgb(
                foundation::colorspace::Family::DeviceCMYK,
                std::span<const double>(comp, 4));
            out[i * 4 + 0] = static_cast<std::byte>(
                foundation::colorspace::Quantize(rgb.r));
            out[i * 4 + 1] = static_cast<std::byte>(
                foundation::colorspace::Quantize(rgb.g));
            out[i * 4 + 2] = static_cast<std::byte>(
                foundation::colorspace::Quantize(rgb.b));
        }
        // alpha already 0xFF from the assign-fill above.
    }
    return out;
}

// Expand an Indexed-colour-space bitmap through a /Lookup palette
// into RGBA8. Index samples are `bpc` bits each (1/2/4/8), packed
// MSB-first with each scanline byte-aligned (§8.9.5.2). The palette
// (already normalised to RGB triples by ResolveColorSpace) holds
// (hival + 1) × 3 bytes. Out-of-range indices saturate to hival.
std::vector<std::byte> ExpandIndexedRgbToRgba8(
        std::span<const std::byte> raw,
        std::uint32_t width, std::uint32_t height,
        std::uint32_t bpc,
        std::span<const std::byte> palette,
        std::uint32_t hival) {
    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * height;
    if (palette.size() < (hival + 1u) * 3u) return {};
    const std::size_t row_bytes =
        (static_cast<std::size_t>(width) * bpc + 7u) / 8u;
    if (raw.size() < row_bytes * height) return {};
    const std::uint32_t mask =
        (bpc >= 8) ? 0xFFu : ((1u << bpc) - 1u);
    std::vector<std::byte> out(pixel_count * 4u,
                               static_cast<std::byte>(255));
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::byte* rowp =
            raw.data() + static_cast<std::size_t>(y) * row_bytes;
        std::uint32_t bitpos = 0;
        for (std::uint32_t x = 0; x < width; ++x) {
            std::uint32_t idx;
            if (bpc == 8) {
                idx = std::to_integer<std::uint8_t>(rowp[x]);
            } else {
                const std::uint32_t byte_i = bitpos >> 3;
                const std::uint32_t bit_off = bitpos & 7u;
                const std::uint8_t bv =
                    std::to_integer<std::uint8_t>(rowp[byte_i]);
                idx = (bv >> (8u - bpc - bit_off)) & mask;
                bitpos += bpc;
            }
            if (idx > hival) idx = hival;
            const std::size_t p = static_cast<std::size_t>(idx) * 3u;
            const std::size_t o =
                (static_cast<std::size_t>(y) * width + x) * 4u;
            out[o + 0] = palette[p + 0];
            out[o + 1] = palette[p + 1];
            out[o + 2] = palette[p + 2];
        }
    }
    return out;
}

// Expand a packed bilevel scanline buffer (1 bit per pixel,
// MSB-first; CCITT / JBIG2 convention: 1=black, 0=white) into
// RGBA8. Bytes-per-row = ceil(width / 8). When `invert` is set
// (the image carries /Decode [1 0]) the polarity is flipped:
// 1=white, 0=black.
std::vector<std::byte> ExpandBilevelToRgba8(
        std::span<const std::byte> packed,
        std::uint32_t width, std::uint32_t height,
        bool invert = false) {
    const std::size_t row_bytes = (width + 7u) / 8u;
    if (packed.size() < row_bytes * height) return {};
    std::vector<std::byte> out(
        static_cast<std::size_t>(width) * height * 4u,
        static_cast<std::byte>(255));
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto byte_v = std::to_integer<std::uint8_t>(
                packed[y * row_bytes + x / 8u]);
            std::uint8_t bit =
                (byte_v >> (7u - (x % 8u))) & 1u;
            if (invert) bit ^= 1u;
            // 1 = black, 0 = white.
            const std::uint8_t v = bit ? 0u : 255u;
            const std::size_t off =
                (static_cast<std::size_t>(y) * width + x) * 4u;
            out[off + 0] = static_cast<std::byte>(v);
            out[off + 1] = static_cast<std::byte>(v);
            out[off + 2] = static_cast<std::byte>(v);
        }
    }
    return out;
}

// Read /ColorSpace into one of three v1-supported tags + an
// optional Indexed palette. Out-of-scope colour spaces (ICC,
// /Pattern, /CalRGB, etc.) return tag = "Unsupported" and the
// caller records the image as None.
struct ResolvedColorSpace {
    // DeviceRGB | DeviceGray | DeviceCMYK | Indexed | Unsupported
    std::string tag = "Unsupported";
    std::vector<std::byte> palette;    // valid only for Indexed
    std::uint32_t hival = 0;           // valid only for Indexed
};

ResolvedColorSpace ResolveColorSpace(const Dump& dump,
                                     const Value* cs_v) {
    ResolvedColorSpace out;
    if (!cs_v) {
        // PDF spec defaults images without /ColorSpace (besides
        // /ImageMask) to DeviceGray; we follow the common case.
        out.tag = "DeviceGray";
        return out;
    }
    const auto* resolved = Resolve(dump, *cs_v);
    if (!resolved) return out;
    if (const auto* name =
            std::get_if<std::string>(&resolved->v)) {
        if (*name == "DeviceRGB" || *name == "RGB") {
            out.tag = "DeviceRGB";
        } else if (*name == "DeviceGray" || *name == "G") {
            out.tag = "DeviceGray";
        } else if (*name == "DeviceCMYK" || *name == "CMYK") {
            out.tag = "DeviceCMYK";
        }
        return out;
    }
    const auto* arr = std::get_if<
        foundation::objects::Array>(&resolved->v);
    if (!arr || arr->items.empty()) return out;
    const auto* family =
        std::get_if<std::string>(&arr->items[0].v);
    if (!family) return out;
    if ((*family == "Indexed" || *family == "I")
            && arr->items.size() >= 4) {
        // [/Indexed base hival lookup] (§8.6.6.3). The base may be any
        // device, calibrated (Cal*) or ICC space — resolve it
        // recursively to its device-component count, then convert each
        // palette entry into the RGB triples the Indexed expander reads.
        // Lookup must be a literal string (stream form not yet
        // supported; ResolveColorSpace has no stream-decode access).
        const ResolvedColorSpace base =
            ResolveColorSpace(dump, &arr->items[1]);
        int base_comps = 0;
        if (base.tag == "DeviceGray")      base_comps = 1;
        else if (base.tag == "DeviceRGB")  base_comps = 3;
        else if (base.tag == "DeviceCMYK") base_comps = 4;
        else return out;  // base unsupported (Pattern / nested Indexed)

        const auto& hi_v = arr->items[2];
        if (const auto* h = std::get_if<std::int64_t>(&hi_v.v)) {
            out.hival = static_cast<std::uint32_t>(
                std::max<std::int64_t>(0, *h));
        }
        const auto* look = Resolve(dump, arr->items[3]);
        if (!look) return out;
        const auto* s = std::get_if<
            foundation::objects::String>(&look->v);
        if (!s) return out;  // stream-form /Lookup not yet supported
        const auto& lut = s->bytes;
        const std::size_t entries =
            static_cast<std::size_t>(out.hival) + 1u;
        if (lut.size() < entries * static_cast<std::size_t>(base_comps))
            return out;
        out.palette.resize(entries * 3u);
        for (std::size_t e = 0; e < entries; ++e) {
            const std::byte* srcp =
                lut.data() + e * static_cast<std::size_t>(base_comps);
            std::byte r, g, b;
            if (base_comps == 1) {
                r = g = b = srcp[0];
            } else if (base_comps == 3) {
                r = srcp[0]; g = srcp[1]; b = srcp[2];
            } else {  // DeviceCMYK → sRGB (§8.6.4.4)
                const double comp[4] = {
                    std::to_integer<std::uint8_t>(srcp[0]) / 255.0,
                    std::to_integer<std::uint8_t>(srcp[1]) / 255.0,
                    std::to_integer<std::uint8_t>(srcp[2]) / 255.0,
                    std::to_integer<std::uint8_t>(srcp[3]) / 255.0};
                const auto c = foundation::colorspace::ToSrgb(
                    foundation::colorspace::Family::DeviceCMYK,
                    std::span<const double>(comp, 4));
                r = static_cast<std::byte>(
                    foundation::colorspace::Quantize(c.r));
                g = static_cast<std::byte>(
                    foundation::colorspace::Quantize(c.g));
                b = static_cast<std::byte>(
                    foundation::colorspace::Quantize(c.b));
            }
            out.palette[e * 3 + 0] = r;
            out.palette[e * 3 + 1] = g;
            out.palette[e * 3 + 2] = b;
        }
        out.tag = "Indexed";
        return out;
    }
    if (*family == "ICCBased" && arr->items.size() >= 2) {
        // [/ICCBased <stream>]. The ICC profile itself is not
        // applied (colour management is out of scope); we honour
        // the stream's /N component count and treat the data as the
        // matching device space, the PDF-sanctioned fallback when
        // no /DefaultGray|RGB|CMYK override is present (§8.6.5.5).
        const auto* sresolved = Resolve(dump, arr->items[1]);
        if (const auto* st = sresolved
                ? std::get_if<foundation::objects::Stream>(
                      &sresolved->v) : nullptr) {
            const auto* n_v = DictGet(st->header, "N");
            const auto n = n_v
                ? static_cast<int>(AsNumber(*n_v)) : 0;
            if (n == 1) out.tag = "DeviceGray";
            else if (n == 3) out.tag = "DeviceRGB";
            else if (n == 4) out.tag = "DeviceCMYK";
        }
        return out;
    }
    if (*family == "CalRGB") {
        // [/CalRGB <dict>]. Calibrated RGB (§8.6.5.3). Colour management
        // (white point / gamma / matrix) is out of scope; map to the
        // device space with the same component count — the PDF-sanctioned
        // device fallback. Without this a CalRGB image is dropped and the
        // page renders blank (field reproducers 33306_whitepoint*).
        out.tag = "DeviceRGB";
        return out;
    }
    if (*family == "CalGray") {
        // [/CalGray <dict>] (§8.6.5.2) → DeviceGray, same as CalRGB.
        out.tag = "DeviceGray";
        return out;
    }
    if (*family == "DeviceRGB" || *family == "RGB") {
        out.tag = "DeviceRGB";
    } else if (*family == "DeviceGray" || *family == "G") {
        out.tag = "DeviceGray";
    } else if (*family == "DeviceCMYK" || *family == "CMYK") {
        out.tag = "DeviceCMYK";
    }
    return out;
}

// Unbox a /Filter entry into the chain of names. Single-name
// (Name) returns a 1-element vector; array form returns one
// per entry. Indirect refs are resolved.
std::vector<std::string> FilterChain(const Dump& dump,
                                     const Value* filter_v) {
    std::vector<std::string> out;
    if (!filter_v) return out;
    const auto* resolved = Resolve(dump, *filter_v);
    if (!resolved) return out;
    if (const auto* name =
            std::get_if<std::string>(&resolved->v)) {
        out.push_back(*name);
        return out;
    }
    if (const auto* arr = std::get_if<
            foundation::objects::Array>(&resolved->v)) {
        for (const auto& item : arr->items) {
            const auto* itr = Resolve(dump, item);
            if (!itr) continue;
            if (const auto* nm =
                    std::get_if<std::string>(&itr->v)) {
                out.push_back(*nm);
            }
        }
    }
    return out;
}

// Unbox /DecodeParms into a single dict (Name-only filter) or
// the Nth dict in the /DecodeParms array for a multi-filter
// chain. Returns nullptr if there's no per-filter parms.
const foundation::objects::Dict* DecodeParms(
        const Dump& dump,
        const foundation::objects::Dict& stream_header,
        std::size_t filter_index) {
    const auto* parms_v = DictGet(stream_header, "DecodeParms");
    if (!parms_v) return nullptr;
    const auto* resolved = Resolve(dump, *parms_v);
    if (!resolved) return nullptr;
    if (const auto* d = std::get_if<
            foundation::objects::Dict>(&resolved->v)) {
        return d;
    }
    if (const auto* arr = std::get_if<
            foundation::objects::Array>(&resolved->v)) {
        if (filter_index >= arr->items.size()) return nullptr;
        const auto* nth =
            Resolve(dump, arr->items[filter_index]);
        if (!nth) return nullptr;
        return std::get_if<foundation::objects::Dict>(&nth->v);
    }
    return nullptr;
}

// Reverse a /DecodeParms predictor (§7.4.4.4) over a freshly
// Flate/LZW-decoded sample buffer. PDF reuses the TIFF (Predictor 2,
// horizontal differencing) and PNG (Predictor 10–15, per-row filter
// byte) predictor families; without undoing them the samples are
// residuals, not pixels — for a DeviceGray /SMask that means an alpha
// channel of near-zero differences, i.e. a fully transparent (blank)
// image. Operates on 8-bit components only (the image paths that call
// this already require BitsPerComponent == 8). `comps` is the colour
// component count from the resolved colour space; /Colors and /Columns
// default to it and to `width`. A Predictor of 1 (or no parms) returns
// the buffer unchanged.
std::vector<std::byte> ApplyImagePredictor(
        std::vector<std::byte> data,
        const foundation::objects::Dict* parms,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t comps,
        std::uint32_t image_bpc = 8) {
    if (!parms) return data;
    const auto* pred_v = DictGet(*parms, "Predictor");
    const int predictor = pred_v ? static_cast<int>(AsNumber(*pred_v)) : 1;
    if (predictor <= 1) return data;

    const auto* colors_v = DictGet(*parms, "Colors");
    const std::size_t colors = colors_v
        ? static_cast<std::size_t>(AsNumber(*colors_v))
        : static_cast<std::size_t>(comps);
    const auto* cols_v = DictGet(*parms, "Columns");
    const std::size_t columns = cols_v
        ? static_cast<std::size_t>(AsNumber(*cols_v))
        : static_cast<std::size_t>(width);
    // Predictor BitsPerComponent: /DecodeParms override, else the image's.
    const auto* pbpc_v = DictGet(*parms, "BitsPerComponent");
    const std::size_t pbpc = pbpc_v
        ? static_cast<std::size_t>(AsNumber(*pbpc_v))
        : static_cast<std::size_t>(image_bpc);
    if (colors == 0 || columns == 0 || pbpc == 0) return data;
    // PNG/TIFF predictors operate byte-wise; the row stride and the
    // per-pixel byte step both follow the sample bit depth, so sub-byte
    // samples (bpc 1/2/4) pack into a ceil(bits/8) row (§7.4.4.4).
    const std::size_t bpp =
        std::max<std::size_t>(1, (colors * pbpc + 7) / 8);
    const std::size_t rowlen = (columns * colors * pbpc + 7) / 8;
    if (rowlen == 0) return data;
    const std::size_t rows = height;

    if (predictor == 2) {
        // TIFF Predictor 2: each sample is stored as the difference
        // from the same component one pixel to its left. Reverse in
        // place, row by row.
        for (std::size_t r = 0; r < rows; ++r) {
            const std::size_t base = r * rowlen;
            if (base + rowlen > data.size()) break;
            for (std::size_t j = bpp; j < rowlen; ++j) {
                data[base + j] = static_cast<std::byte>(
                    (static_cast<std::uint8_t>(data[base + j]) +
                     static_cast<std::uint8_t>(data[base + j - bpp]))
                    & 0xFF);
            }
        }
        return data;
    }

    // PNG predictors (10–15): every row is prefixed by a one-byte
    // filter tag; reconstruct into a tag-free buffer.
    const std::size_t stride = rowlen + 1;
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
    out.reserve(rows * rowlen);
    std::vector<std::uint8_t> prev(rowlen, 0);
    std::vector<std::uint8_t> cur(rowlen, 0);
    for (std::size_t r = 0; r * stride + stride <= data.size(); ++r) {
        const std::size_t base = r * stride;
        const std::uint8_t ft = static_cast<std::uint8_t>(data[base]);
        for (std::size_t j = 0; j < rowlen; ++j) {
            const int x = static_cast<std::uint8_t>(data[base + 1 + j]);
            const int a = (j >= bpp) ? cur[j - bpp] : 0;
            const int b = prev[j];
            const int c = (j >= bpp) ? prev[j - bpp] : 0;
            int recon;
            switch (ft) {
                case 0: recon = x; break;
                case 1: recon = x + a; break;
                case 2: recon = x + b; break;
                case 3: recon = x + ((a + b) >> 1); break;
                case 4: recon = x + paeth(a, b, c); break;
                default: return data;  // malformed filter tag
            }
            cur[j] = static_cast<std::uint8_t>(recon & 0xFF);
        }
        for (std::size_t j = 0; j < rowlen; ++j) {
            out.push_back(static_cast<std::byte>(cur[j]));
        }
        prev.swap(cur);
    }
    return out;
}

// Build one ResolvedImage from a /Subtype /Image stream. Returns
// rf.valid = false for any v1-out-of-scope shape (image mask,
// non-supported filter chain, non-supported colour space, etc.)
// — the /Do operator silently skips invalid entries.
ResolvedImage BuildResolvedImageBase(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Stream& st,
        bool allow_image_mask = false,
        bool for_mask = false) {
    ResolvedImage out;

    // A /Decode [1 0] inverts the sample→colour mapping (§8.9.5.2). For the
    // CCITT / JBIG2 bilevel branches (whose decoded "1 = black" feeds
    // ExpandBilevelToRgba8 directly) this means flipping the output — common
    // on scanned documents where the whole page otherwise floods black
    // (field reproducer 35063.pdf, a CCITT scan with /Decode [1 0]). Applied
    // only to a base image; the /SMask and /Mask sub-image paths read the
    // mask's /Decode themselves (for_mask suppresses the double-apply).
    bool bilevel_invert = false;
    if (!for_mask) {
        if (const auto* dec = DictGet(st.header, "Decode")) {
            const auto* da = std::get_if<
                foundation::objects::Array>(&dec->v);
            if (da && da->items.size() >= 2)
                bilevel_invert = AsNumber(da->items[0]) >
                                 AsNumber(da->items[1]);
        }
    }

    // /ImageMask images carry no colour — a 1-bpc stencil that selects
    // where the *current* colour (or, via /Mask, a base image) paints.
    // As a standalone /Do image it is out of scope; but when resolved as
    // the /Mask of another image we decode it (allow_image_mask) to drive
    // that image's alpha. §8.9.6.2.
    if (const auto* im = DictGet(st.header, "ImageMask")) {
        if (const auto* b = std::get_if<bool>(&im->v)) {
            if (*b && !allow_image_mask) return out;
        }
    }

    const auto* width_v = DictGet(st.header, "Width");
    const auto* height_v = DictGet(st.header, "Height");
    if (!width_v || !height_v) return out;
    const auto width = static_cast<std::uint32_t>(
        AsNumber(*width_v));
    const auto height = static_cast<std::uint32_t>(
        AsNumber(*height_v));
    if (width == 0 || height == 0) return out;

    const auto* bpc_v = DictGet(st.header, "BitsPerComponent");
    const auto bpc = bpc_v
        ? static_cast<std::uint32_t>(AsNumber(*bpc_v))
        : 8u;

    const auto cs = ResolveColorSpace(
        dump, DictGet(st.header, "ColorSpace"));
    if (cs.tag == "Unsupported") return out;

    // Decode the /Filter chain. v1 honours single-filter:
    // FlateDecode, DCTDecode, JPXDecode, CCITTFaxDecode, JBIG2Decode.
    // Multi-filter chains (e.g. Flate + ASCII85) are out of
    // scope; record None and skip.
    const auto chain = FilterChain(
        dump, DictGet(st.header, "Filter"));
    // st.body is a span into the parsed source buffer
    // (parser bounds-checked at parse time).
    (void)pdf_bytes;
    std::span<const std::byte> raw = st.body;

    if (chain.empty()) {
        // Indexed (palette) image: each sample is a bpc-bit index
        // (1/2/4/8) into the RGB palette. Handled before the bilevel
        // path so a 1-bpc Indexed image uses its palette rather than
        // the 1=black /ImageMask stencil convention.
        if (cs.tag == "Indexed") {
            auto rgba = ExpandIndexedRgbToRgba8(
                raw, width, height, bpc,
                std::span<const std::byte>(cs.palette.data(),
                                           cs.palette.size()),
                cs.hival);
            if (rgba.empty()) return out;
            out.valid = true;
            out.width = width;
            out.height = height;
            out.rgba8 = std::move(rgba);
            return out;
        }
        // 1-bpc bilevel (a raw /ImageMask stencil): the bytes are
        // packed bits, row-aligned. ExpandBilevelToRgba8 maps 1→R 0,
        // 0→R 255 — the polarity the /Mask compositor reads.
        if (bpc == 1) {
            auto rgba = ExpandBilevelToRgba8(raw, width, height);
            if (rgba.empty()) return out;
            out.valid = true;
            out.width = width;
            out.height = height;
            out.rgba8 = std::move(rgba);
            return out;
        }
        // Raw bitmap, BPC=8 only in v1.
        if (bpc != 8) return out;
        const std::uint32_t comps =
            (cs.tag == "DeviceRGB") ? 3u
            : (cs.tag == "DeviceGray") ? 1u
            : (cs.tag == "DeviceCMYK") ? 4u : 0u;
        if (comps == 0) return out;
        std::vector<std::byte> rgba =
            ExpandRawToRgba8(raw, width, height, comps);
        if (rgba.empty()) return out;
        out.valid = true;
        out.width = width;
        out.height = height;
        out.rgba8 = std::move(rgba);
        return out;
    }

    if (chain.size() == 1) {
        const auto& f = chain.front();
        if (f == "FlateDecode" || f == "Fl") {
            std::vector<std::byte> decoded;
            try {
                decoded = foundation::flate::Decode(raw);
            } catch (...) { return out; }
            // Indexed (palette) image: bpc-bit indices into the RGB
            // palette. Handled before the bilevel path. A /DecodeParms
            // predictor is byte-wise, so it is stripped for sub-byte
            // (bpc 1/2/4) indices too — ApplyImagePredictor derives the
            // row stride from the sample bit depth (one index component).
            if (cs.tag == "Indexed") {
                decoded = ApplyImagePredictor(
                    std::move(decoded),
                    DecodeParms(dump, st.header, 0),
                    width, height, /*comps=*/1, bpc);
                std::span<const std::byte> dspan(
                    decoded.data(), decoded.size());
                auto rgba = ExpandIndexedRgbToRgba8(
                    dspan, width, height, bpc,
                    std::span<const std::byte>(
                        cs.palette.data(), cs.palette.size()),
                    cs.hival);
                if (rgba.empty()) return out;
                out.valid = true;
                out.width = width;
                out.height = height;
                out.rgba8 = std::move(rgba);
                return out;
            }
            if (bpc == 1) {
                auto rgba = ExpandBilevelToRgba8(
                    std::span<const std::byte>(
                        decoded.data(), decoded.size()),
                    width, height);
                if (rgba.empty()) return out;
                out.valid = true;
                out.width = width;
                out.height = height;
                out.rgba8 = std::move(rgba);
                return out;
            }
            if (bpc != 8) return out;
            const std::uint32_t comps =
                (cs.tag == "DeviceRGB") ? 3u
                : (cs.tag == "DeviceGray") ? 1u
                : (cs.tag == "DeviceCMYK") ? 4u : 0u;
            if (comps == 0) return out;
            // Undo any /DecodeParms predictor (TIFF 2 / PNG 10–15)
            // before the samples are interpreted as pixels.
            decoded = ApplyImagePredictor(
                std::move(decoded), DecodeParms(dump, st.header, 0),
                width, height, comps);
            std::span<const std::byte> dec_span(
                decoded.data(), decoded.size());
            std::vector<std::byte> rgba =
                ExpandRawToRgba8(dec_span, width, height, comps);
            if (rgba.empty()) return out;
            out.valid = true;
            out.width = width;
            out.height = height;
            out.rgba8 = std::move(rgba);
            return out;
        }
        if (f == "DCTDecode" || f == "DCT") {
            // dctdecode produces RGBA8 directly via libjpeg-turbo's
            // JCS_EXT_RGBA. Width/height are read from the JPEG
            // SOF marker; they SHOULD match /Width and /Height
            // but we trust the JPEG over the dict.
            foundation::dctdecode::DecodedImage dec;
            try { dec = foundation::dctdecode::Decode(raw); }
            catch (...) { return out; }
            if (dec.components != 4
                    || dec.width == 0 || dec.height == 0) {
                return out;
            }
            out.valid = true;
            out.width = dec.width;
            out.height = dec.height;
            out.rgba8 = std::move(dec.pixels);
            return out;
        }
        if (f == "JPXDecode" || f == "JPX") {
            // jpx produces RGBA8 directly. Width/height come from the
            // codestream SIZ marker; trust them over the image dict.
            foundation::jpx::DecodedImage dec;
            try { dec = foundation::jpx::Decode(raw); }
            catch (...) { return out; }
            if (dec.components != 4
                    || dec.width == 0 || dec.height == 0) {
                return out;
            }
            out.valid = true;
            out.width = dec.width;
            out.height = dec.height;
            out.rgba8 = std::move(dec.pixels);
            return out;
        }
        if (f == "CCITTFaxDecode" || f == "CCF") {
            // Mono bilevel; honour /DecodeParms (K, Columns,
            // BlackIs1, EndOfBlock, EndOfLine, EncodedByteAlign,
            // Rows). DecodeParms missing → default G3-1D
            // (K=0) at the dict's /Width.
            foundation::ccitt::Params p;
            p.Columns = width;
            p.Rows = height;
            const auto* parms = DecodeParms(dump, st.header, 0);
            if (parms) {
                if (const auto* k_v = DictGet(*parms, "K")) {
                    p.K = static_cast<std::int8_t>(
                        AsNumber(*k_v));
                }
                if (const auto* eol = DictGet(*parms,
                        "EndOfLine")) {
                    if (const auto* b =
                            std::get_if<bool>(&eol->v)) {
                        p.EndOfLine = *b;
                    }
                }
                if (const auto* eba = DictGet(*parms,
                        "EncodedByteAlign")) {
                    if (const auto* b =
                            std::get_if<bool>(&eba->v)) {
                        p.EncodedByteAlign = *b;
                    }
                }
                if (const auto* eob = DictGet(*parms,
                        "EndOfBlock")) {
                    if (const auto* b =
                            std::get_if<bool>(&eob->v)) {
                        p.EndOfBlock = *b;
                    }
                }
                if (const auto* bi = DictGet(*parms,
                        "BlackIs1")) {
                    if (const auto* b =
                            std::get_if<bool>(&bi->v)) {
                        p.BlackIs1 = *b;
                    }
                }
                if (const auto* col = DictGet(*parms,
                        "Columns")) {
                    p.Columns = static_cast<std::uint32_t>(
                        AsNumber(*col));
                }
                if (const auto* rws = DictGet(*parms, "Rows")) {
                    p.Rows = static_cast<std::uint32_t>(
                        AsNumber(*rws));
                }
            }
            foundation::ccitt::DecodedImage dec;
            try { dec = foundation::ccitt::Decode(raw, p); }
            catch (...) { return out; }
            if (dec.width == 0 || dec.height == 0) return out;
            std::span<const std::byte> bits(
                dec.bits.data(), dec.bits.size());
            auto rgba = ExpandBilevelToRgba8(
                bits, dec.width, dec.height, bilevel_invert);
            if (rgba.empty()) return out;
            out.valid = true;
            out.width = dec.width;
            out.height = dec.height;
            out.rgba8 = std::move(rgba);
            return out;
        }
        if (f == "JBIG2Decode") {
            // /JBIG2Globals (segment-stream of shared dicts) is
            // optional; v1 supports at most one referenced
            // global stream resolved through the dump.
            std::span<const std::byte> globals;
            std::vector<std::byte> globals_owned;
            const auto* parms = DecodeParms(dump, st.header, 0);
            if (parms) {
                if (const auto* g = DictGet(*parms,
                        "JBIG2Globals")) {
                    if (const auto* gr =
                            Resolve(dump, *g)) {
                        if (const auto* gs = std::get_if<
                                foundation::objects::Stream>(
                                    &gr->v)) {
                            globals_owned = DecodeStream(
                                pdf_bytes, *gs);
                            globals = std::span<
                                const std::byte>(
                                globals_owned.data(),
                                globals_owned.size());
                        }
                    }
                }
            }
            foundation::jbig2::DecodedImage dec;
            try {
                dec = foundation::jbig2::Decode(raw, globals);
            } catch (...) { return out; }
            if (dec.width == 0 || dec.height == 0) return out;
            std::span<const std::byte> bits(
                dec.pixels.data(), dec.pixels.size());
            auto rgba = ExpandBilevelToRgba8(
                bits, dec.width, dec.height, bilevel_invert);
            if (rgba.empty()) return out;
            out.valid = true;
            out.width = dec.width;
            out.height = dec.height;
            out.rgba8 = std::move(rgba);
            return out;
        }
    }
    // Anything else: multi-filter chain or an unsupported
    // single filter. Out of v1 scope.
    return out;
}

// Build a ResolvedImage and apply a soft mask (/SMask, §11.6.5.2)
// when present: the subsidiary DeviceGray image supplies per-pixel
// opacity (sample value = alpha; 0 = transparent, 255 = opaque),
// which replaces the base image's (opaque) alpha channel. The mask
// may have its own dimensions — it is sampled nearest-neighbour onto
// the base grid. Without this, an image whose transparent regions
// carry arbitrary RGB (commonly black) paints those regions opaque
// (e.g. a black box behind a logo). /Mask colour-key / stencil masks
// stay out of scope.
ResolvedImage BuildResolvedImage(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Stream& st) {
    ResolvedImage out = BuildResolvedImageBase(pdf_bytes, dump, st);
    if (!out.valid || out.width == 0 || out.height == 0) return out;

    // /SMask (§11.6.5.2): a subsidiary DeviceGray image whose sample
    // value is per-pixel opacity (0 = transparent, 255 = opaque); it
    // replaces the base image's alpha.
    if (const auto* smv = DictGet(st.header, "SMask")) {
        const auto* smr = Resolve(dump, *smv);
        const auto* sms = smr ? std::get_if<
            foundation::objects::Stream>(&smr->v) : nullptr;
        if (sms) {
            const ResolvedImage mask =
                BuildResolvedImageBase(pdf_bytes, dump, *sms,
                                       /*allow_image_mask=*/false,
                                       /*for_mask=*/true);
            if (mask.valid && mask.width != 0 && mask.height != 0) {
                // §11.6.5.2: the mask sample is the per-pixel alpha. A
                // /Decode [1 0] on the mask inverts the sample→alpha
                // mapping — the common case on JBIG2-coded soft masks of
                // mixed-raster scans, whose 1=black foreground bit must
                // map to opaque (255) rather than transparent. Without
                // this the dark foreground floods every non-ink pixel.
                bool mask_invert = false;
                if (const auto* dec = DictGet(sms->header, "Decode")) {
                    const auto* da = std::get_if<
                        foundation::objects::Array>(&dec->v);
                    if (da && da->items.size() >= 2)
                        mask_invert = AsNumber(da->items[0]) >
                                      AsNumber(da->items[1]);
                }
                for (std::uint32_t y = 0; y < out.height; ++y) {
                    const std::uint32_t my = mask.height == out.height
                        ? y
                        : static_cast<std::uint32_t>(
                              static_cast<std::uint64_t>(y) *
                              mask.height / out.height);
                    for (std::uint32_t x = 0; x < out.width; ++x) {
                        const std::uint32_t mx = mask.width == out.width
                            ? x
                            : static_cast<std::uint32_t>(
                                  static_cast<std::uint64_t>(x) *
                                  mask.width / out.width);
                        // Mask is DeviceGray (R=G=B=gray); R = opacity.
                        std::uint8_t alpha = std::to_integer<std::uint8_t>(
                            mask.rgba8[(static_cast<std::size_t>(my)
                                        * mask.width + mx) * 4]);
                        if (mask_invert)
                            alpha = static_cast<std::uint8_t>(255 - alpha);
                        out.rgba8[(static_cast<std::size_t>(y)
                                   * out.width + x) * 4 + 3] =
                            std::byte{alpha};
                    }
                }
            }
        }
    }

    // /Mask as a reference to a stencil image (§8.9.6.3, Explicit
    // Masking): a 1-bpc /ImageMask image whose sample selects whether
    // the corresponding base-image pixel is painted. Default /Decode
    // [0 1]: sample 0 = paint (opaque), sample 1 = masked (transparent);
    // /Decode [1 0] inverts. The stencil decodes 1=black → R channel 0,
    // 0=white → R 255 (ExpandBilevelToRgba8), so R==255 ⟺ sample 0.
    // Mixed-raster scans (background image + masked foreground image)
    // depend on this: without it the opaque foreground buries the page.
    if (const auto* mkv = DictGet(st.header, "Mask")) {
        const auto* mkr = Resolve(dump, *mkv);
        const auto* mks = mkr ? std::get_if<
            foundation::objects::Stream>(&mkr->v) : nullptr;
        if (mks) {
            bool invert = false;  // /Decode [1 0]
            if (const auto* dec = DictGet(mks->header, "Decode")) {
                const auto* da = std::get_if<
                    foundation::objects::Array>(&dec->v);
                if (da && da->items.size() >= 2)
                    invert = AsNumber(da->items[0]) > 0.5;
            }
            // A JBIG2-coded stencil uses the inverse polarity: JBIG2's
            // 1 = black (§7.4.7) marks the *painted* (foreground) region,
            // the opposite of the default /ImageMask convention (sample
            // 0 = paint) that raw / FlateDecode stencils follow. Without
            // this, a mixed-raster scan paints the foreground over the
            // ~94 % white-bit area and buries the page (field 30179).
            for (const auto& f :
                 FilterChain(dump, DictGet(mks->header, "Filter"))) {
                if (f == "JBIG2Decode") { invert = !invert; break; }
            }
            const ResolvedImage stencil = BuildResolvedImageBase(
                pdf_bytes, dump, *mks, /*allow_image_mask=*/true,
                /*for_mask=*/true);
            if (stencil.valid && stencil.width != 0
                    && stencil.height != 0) {
                for (std::uint32_t y = 0; y < out.height; ++y) {
                    const std::uint32_t my = stencil.height == out.height
                        ? y
                        : static_cast<std::uint32_t>(
                              static_cast<std::uint64_t>(y) *
                              stencil.height / out.height);
                    for (std::uint32_t x = 0; x < out.width; ++x) {
                        const std::uint32_t mx = stencil.width == out.width
                            ? x
                            : static_cast<std::uint32_t>(
                                  static_cast<std::uint64_t>(x) *
                                  stencil.width / out.width);
                        const std::uint8_t r = std::to_integer<
                            std::uint8_t>(stencil.rgba8[
                                (static_cast<std::size_t>(my)
                                 * stencil.width + mx) * 4]);
                        // sample 0 ⟺ R==255 (white). Paint where sample
                        // 0 (default) / sample 1 (inverted).
                        const bool sample_one = (r < 128);
                        const bool painted =
                            invert ? sample_one : !sample_one;
                        if (!painted) {
                            out.rgba8[(static_cast<std::size_t>(y)
                                       * out.width + x) * 4 + 3] =
                                static_cast<std::byte>(0);
                        }
                    }
                }
            }
        }
    }
    return out;
}

// Resolve every /Resources/XObject /Subtype /Image into a
// ResolvedImage, keyed by resource name. /Subtype /Form entries
// are skipped (Beat 5 territory).
std::map<std::string, ResolvedImage> ResolveImagesFromResources(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& resources) {
    std::map<std::string, ResolvedImage> images;
    const auto* xo_v = DictGet(resources, "XObject");
    if (!xo_v) return images;
    const auto* xo_resolved = Resolve(dump, *xo_v);
    if (!xo_resolved) return images;
    const auto* xo_dict = std::get_if<
        foundation::objects::Dict>(&xo_resolved->v);
    if (!xo_dict) return images;
    for (const auto& [name, ref] : xo_dict->entries) {
        const auto* sv = Resolve(dump, ref);
        if (!sv) continue;
        const auto* st = std::get_if<
            foundation::objects::Stream>(&sv->v);
        if (!st) continue;
        const auto* sub = DictGet(st->header, "Subtype");
        if (!sub) continue;
        const auto* sub_name = std::get_if<std::string>(&sub->v);
        if (!sub_name || *sub_name != "Image") continue;
        images.emplace(name,
            BuildResolvedImage(pdf_bytes, dump, *st));
    }
    return images;
}

std::map<std::string, ResolvedImage> ResolvePageImages(
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict& page_dict) {
    const auto* resources = ResolveResources(dump, page_dict);
    if (!resources) return {};
    return ResolveImagesFromResources(pdf_bytes, dump, *resources);
}

// CTM stack frame for /q /Q. Snapshots the active CTM, the
// non-stroking + stroking colours, and the line width.
struct Gstate {
    Matrix ctm;
    foundation::colorspace::ColorRgb fill_color;
    foundation::colorspace::ColorRgb stroke_color;
    double line_width;
    // Non-stroking colour space = /Pattern (set by `cs /Pattern`),
    // with the selected tiling-pattern resource name (set by
    // `scn <name>`). When set, path-fill operators paint the
    // pattern clipped to the path instead of a solid colour. Any
    // device-colour setter (rg/g/k) clears pattern mode.
    bool fill_is_pattern = false;
    std::string fill_pattern;
    // Non-stroking /Separation colour space (§8.6.6.4): set by
    // `cs <name>` resolving to a [/Separation name alt tintTransform]
    // array. When set, `scn <tint>` evaluates the 1-input tint transform
    // into the alternate space → RGB instead of treating the operand as
    // a device colour (without it a `0.3 scn` spot colour reads as
    // DeviceGray 0.3 — a grey banner instead of the PANTONE colour).
    // null = a plain device space; cleared by rg/g/k or a device cs.
    // `*_tint` points into the (render-stable) Dump.
    const Value* fill_sep_tint = nullptr;
    std::string fill_sep_alt;          // alternate-space family name
    const Value* stroke_sep_tint = nullptr;
    std::string stroke_sep_alt;
    // Line dash pattern (§8.4.3.6), in user-space units. Empty =
    // solid. Set by the `d [array] phase` operator; snapshotted by
    // /q /Q like the rest of the graphics state.
    std::vector<double> dash_array;
    double dash_phase = 0.0;
    // Constant alpha (§11.6.4.4), set via `gs` from an ExtGState's
    // /ca (non-stroking) and /CA (stroking). 1.0 = fully opaque.
    // Multiplies the painted coverage of every fill / stroke / image
    // / shading so semi-transparent overlays composite correctly.
    double fill_alpha = 1.0;
    double stroke_alpha = 1.0;
    // Clipping path (§8.5.4), approximated by its axis-aligned device
    // bounding box — the common rectangular `re W n` case is exact.
    // `has_clip` false = no clip (whole page). Snapshotted by q/Q;
    // W/W* intersect the current path's bbox into it.
    bool has_clip = false;
    foundation::rasterizer::ClipRect clip{};
};

// Subpath cursor — tracks the current point + subpath origin
// for path construction. /m sets both; /l, /c, /v, /y advance
// only the current point; /h closes back to the origin.
struct PathCursor {
    bool has_current = false;
    Point current{0.0, 0.0};
    Point subpath_start{0.0, 0.0};
};

// Text state per PDF §9.3 — populated by Tf/Tc/Tw/TL/Ts/Tz/
// Tm/Td/TD/T*/'/" operators while inside a BT/ET block.
struct TextState {
    Matrix tm = foundation::transform::Identity();
    Matrix tlm = foundation::transform::Identity();
    double font_size = 0.0;
    double char_spacing = 0.0;     // Tc
    double word_spacing = 0.0;     // Tw
    double leading = 0.0;          // TL
    double rise = 0.0;             // Ts
    double horiz_scale = 1.0;      // Tz / 100
    const ResolvedFont* font = nullptr;
};

// PDF text-rendering matrix composer:
//   device_pt = glyph_pt × scale(fs/upem) × Tm × ctm
// Compose(A, B) = A × B in the existing renderer's row-vector
// convention; reading left-to-right, scale runs first, then Tm,
// then ctm.
Matrix BuildGlyphCtm(const Matrix& ctm, const TextState& ts,
                     std::uint32_t upem) {
    // A NEGATIVE font size is legal (PDF §9.4.4): generators pair it with a
    // y-flipping text/CTM (e.g. `cm 1 0 0 -1 0 H`) so the glyph, flipped twice,
    // reads upright. The signed font_size must flow through to the glyph scale
    // (and the advance, which already uses it) — bailing to identity on a
    // negative size drew the glyph at raw font-unit scale (a giant black blob).
    // Only an exactly-zero size renders nothing.
    if (upem == 0 || ts.font_size == 0.0) {
        return foundation::transform::Identity();
    }
    // The text-space glyph scale is (Tfs·Th) in x and Tfs in y (PDF §9.4.4);
    // applying the horizontal scaling Th here — not only to the advance — is
    // what lets a negative Tz cancel a negative font size's x-mirror so the
    // glyph reads upright instead of landing mirrored (and clipped away).
    Matrix s = foundation::transform::Scale(
        ts.font_size * ts.horiz_scale / static_cast<double>(upem),
        ts.font_size / static_cast<double>(upem));
    Matrix tm_then_ctm = Compose(ts.tm, ctm);
    return Compose(s, tm_then_ctm);
}

// Cubic Bézier flattening for path operators — copied from
// Beat 2's helper.
void FlattenCubicTo(Path& path, Point p0, Point p1, Point p2,
                    Point p3) {
    constexpr int kSteps = 16;
    for (int i = 1; i <= kSteps; ++i) {
        const double t = static_cast<double>(i) / kSteps;
        const double s = 1.0 - t;
        const double x =
            s*s*s * p0.x + 3*s*s*t * p1.x + 3*s*t*t * p2.x +
            t*t*t * p3.x;
        const double y =
            s*s*s * p0.y + 3*s*s*t * p1.y + 3*s*t*t * p2.y +
            t*t*t * p3.y;
        path.segments.push_back(
            {SegmentKind::Line,
             {Point{x, y}, Point{0, 0}}});
    }
}

// Emit one butt-capped stroke quad for the segment a→b as a
// rectangle `half` wide on each side of the segment normal.
void EmitStrokeQuad(Path& dst, Point a, Point b, double half) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-9) return;
    const double nx = -dy / len * half;
    const double ny =  dx / len * half;
    dst.segments.push_back(
        {SegmentKind::Move,
         {Point{a.x + nx, a.y + ny}, Point{0, 0}}});
    dst.segments.push_back(
        {SegmentKind::Line,
         {Point{b.x + nx, b.y + ny}, Point{0, 0}}});
    dst.segments.push_back(
        {SegmentKind::Line,
         {Point{b.x - nx, b.y - ny}, Point{0, 0}}});
    dst.segments.push_back(
        {SegmentKind::Line,
         {Point{a.x - nx, a.y - ny}, Point{0, 0}}});
    dst.segments.push_back(
        {SegmentKind::Close,
         {Point{0, 0}, Point{0, 0}}});
}

void AppendStrokeQuads(Path& dst,
                       const std::vector<Point>& vertices,
                       bool close_back,
                       double line_width_user) {
    if (vertices.size() < 2) return;
    const double half = std::max(line_width_user, 1.0) * 0.5;
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
        EmitStrokeQuad(dst, vertices[i], vertices[i + 1], half);
    }
    if (close_back) {
        EmitStrokeQuad(dst, vertices.back(), vertices.front(), half);
    }
}

// Dashed stroke (§8.4.3.6): walk the polyline measuring arc length
// in user space, emitting a stroke quad only over the "on" spans of
// the dash cycle. `dash` alternates on/off lengths starting "on";
// `phase` is the distance into the cycle at which to start. The
// pattern restarts at the beginning of the subpath. Butt caps only
// (no round/projecting cap), which is the v1 stroke model. An empty
// or all-zero `dash` (or a malformed negative entry) degrades to a
// solid stroke.
void AppendDashedStrokeQuads(Path& dst,
                             const std::vector<Point>& vertices,
                             bool close_back,
                             double line_width_user,
                             const std::vector<double>& dash,
                             double phase) {
    if (vertices.size() < 2) return;
    const double half = std::max(line_width_user, 1.0) * 0.5;

    double cycle = 0.0;
    bool valid = !dash.empty();
    for (const double d : dash) {
        if (d < 0.0) valid = false;
        cycle += d;
    }
    if (!valid || cycle <= 0.0) {
        AppendStrokeQuads(dst, vertices, close_back, line_width_user);
        return;
    }

    const std::size_t n = dash.size();
    std::size_t idx = 0;          // current dash element
    bool on = true;               // even indices are "on"
    double rem = dash[0];         // remaining length in this element

    auto advance = [&]() {
        idx = (idx + 1) % n;
        on = (idx % 2 == 0);
        rem = dash[idx];
    };

    // Consume `phase` into the cycle so dashing starts mid-pattern.
    double p = std::fmod(phase, cycle);
    if (p < 0.0) p += cycle;
    for (int guard = 0; p > 0.0 && guard < 100000; ++guard) {
        if (rem <= 0.0) { advance(); continue; }
        if (p < rem) { rem -= p; p = 0.0; }
        else { p -= rem; advance(); }
    }

    auto walk_edge = [&](Point a, Point b) {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double L = std::sqrt(dx * dx + dy * dy);
        if (L < 1e-9) return;
        double pos = 0.0;
        for (int guard = 0; pos < L && guard < 1000000; ++guard) {
            if (rem <= 0.0) { advance(); continue; }
            const double step = std::min(rem, L - pos);
            if (on && step > 0.0) {
                const double t0 = pos / L;
                const double t1 = (pos + step) / L;
                EmitStrokeQuad(dst,
                    Point{a.x + dx * t0, a.y + dy * t0},
                    Point{a.x + dx * t1, a.y + dy * t1}, half);
            }
            pos += step;
            rem -= step;
        }
    };

    for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
        walk_edge(vertices[i], vertices[i + 1]);
    }
    if (close_back) {
        walk_edge(vertices.back(), vertices.front());
    }
}

struct Subpath {
    std::vector<Point> vertices;
    bool closed = false;
};

std::vector<Subpath> ToSubpaths(const Path& path) {
    std::vector<Subpath> subs;
    Subpath cur;
    bool has_started = false;
    for (const auto& seg : path.segments) {
        switch (seg.kind) {
            case SegmentKind::Move:
                if (has_started && !cur.vertices.empty()) {
                    subs.push_back(std::move(cur));
                    cur = Subpath{};
                }
                cur.vertices.push_back(seg.pts[0]);
                has_started = true;
                break;
            case SegmentKind::Line:
                cur.vertices.push_back(seg.pts[0]);
                break;
            case SegmentKind::Close:
                cur.closed = true;
                if (!cur.vertices.empty()) {
                    subs.push_back(std::move(cur));
                    cur = Subpath{};
                    has_started = false;
                }
                break;
            case SegmentKind::Rect:
                break;
        }
    }
    if (has_started && !cur.vertices.empty()) {
        subs.push_back(std::move(cur));
    }
    return subs;
}

void StrokePath(Raster& dst, const Path& path,
                const Matrix& ctm,
                const foundation::colorspace::ColorRgb& color,
                double line_width_user,
                const std::vector<double>& dash = {},
                double dash_phase = 0.0,
                double alpha = 1.0,
                const foundation::rasterizer::ClipRect* clip = nullptr) {
    Path quads;
    for (const auto& sub : ToSubpaths(path)) {
        if (sub.vertices.size() < 2 && !sub.closed) continue;
        if (dash.empty()) {
            AppendStrokeQuads(quads, sub.vertices, sub.closed,
                              line_width_user);
        } else {
            AppendDashedStrokeQuads(quads, sub.vertices, sub.closed,
                                    line_width_user, dash, dash_phase);
        }
    }
    if (!quads.segments.empty()) {
        foundation::rasterizer::Fill(
            dst, quads, ctm, color, alpha, FillRule::NonZero, clip);
    }
}

// Resolve the operand at index `idx` into a string-bytes view.
// Tj and ' ", as well as TJ array elements, take a
// foundation::objects::String (literal or hex). Returns empty
// when the operand isn't a string.
std::span<const std::byte> AsStringBytes(const Value& v) {
    if (const auto* s = std::get_if<
            foundation::objects::String>(&v.v)) {
        return std::span<const std::byte>(
            s->bytes.data(), s->bytes.size());
    }
    return {};
}

// Show one byte: dispatch its glyph through glyph_rasterizer
// and advance the text matrix. Returns the user-space x advance
// the caller can use to translate forward — useful for TJ where
// the loop accumulates per-element shifts.
// Draw one already-resolved glyph and advance the text matrix.
// `apply_word_spacing` adds Tw — true only for the single-byte
// space (code 32); per PDF §9.3.3 word spacing never applies to
// a 2-byte CID code, even one whose low byte is 32.
void ShowGlyph(Raster& dst,
               const Matrix& ctm,
               const Gstate& gs,
               TextState& ts,
               std::optional<GlyphLookup> resolved,
               bool apply_word_spacing) {
    // `ctm` is the CURRENT graphics-state CTM (gs.ctm), not the page default —
    // text is positioned by the same CTM as fills/strokes so that `cm`
    // transforms (e.g. a page-level y-flip) apply to glyphs too.
    if (!ts.font || ts.font->kind == ResolvedFont::Kind::None) {
        // Even without a renderable font we still need to
        // advance Tm so that downstream Tj operands position
        // correctly. Standard14 metrics aren't wired yet so a
        // fontless run advances by zero.
        return;
    }
    if (resolved) {
        foundation::glyph_rasterizer::Outline outline;
        if (ts.font->kind == ResolvedFont::Kind::TrueType) {
            outline = BuildTtOutline(ts.font->tt, resolved->gid);
        } else if (ts.font->kind == ResolvedFont::Kind::Cff) {
            outline = BuildCffOutline(ts.font->cff, resolved->gid);
        }
        if (!outline.segments.empty()) {
            // Render mode 0 (fill) is the v1-supported mode.
            // Modes 1-7 (stroke / fill+stroke / clip) are out
            // of scope; treat them as fill for the v1 surface.
            const Matrix m = BuildGlyphCtm(
                ctm, ts, ts.font->units_per_em);
            foundation::glyph_rasterizer::Draw(
                dst, outline, m, gs.fill_color, gs.fill_alpha,
                gs.has_clip ? &gs.clip : nullptr);
        }
    }

    // Advance Tm regardless of whether the glyph drew anything
    // — a missing glyph still occupies its advance width on
    // page (PDF §9.4.4).
    double w0 = 0.0;
    if (resolved) {
        w0 = static_cast<double>(resolved->advance_funits) /
             static_cast<double>(ts.font->units_per_em);
    }
    double tx = (w0 * ts.font_size + ts.char_spacing
                 + (apply_word_spacing ? ts.word_spacing : 0.0))
                * ts.horiz_scale;
    ts.tm = Compose(foundation::transform::Translate(tx, 0.0),
                    ts.tm);
}

void ShowByte(Raster& dst,
              const Matrix& ctm,
              const Gstate& gs,
              TextState& ts,
              std::uint8_t b) {
    if (!ts.font) return;
    ShowGlyph(dst, ctm, gs, ts,
              ResolveByteToGlyph(*ts.font, b),
              /*apply_word_spacing=*/b == 0x20);
}

void ShowString(Raster& dst, const Matrix& ctm,
                const Gstate& gs, TextState& ts,
                std::span<const std::byte> bytes) {
    const bool cid_2byte = ts.font && ts.font->is_cid;
    if (cid_2byte) {
        // Identity-H: each 2-byte code IS the CID (big-endian).
        // CID→GID then runs through the font's /CIDToGIDMap.
        // Non-Identity CMaps (named or embedded) are out of v1
        // scope — Identity-H/V cover the common subset-font case.
        for (std::size_t i = 0; i + 1 < bytes.size(); i += 2) {
            const auto hi = static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(bytes[i]));
            const auto lo = static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(bytes[i + 1]));
            const std::uint16_t cid =
                static_cast<std::uint16_t>((hi << 8) | lo);
            ShowGlyph(dst, ctm, gs, ts,
                      ResolveCidToGlyph(*ts.font, cid),
                      /*apply_word_spacing=*/false);
        }
        return;
    }
    for (auto b : bytes) {
        ShowByte(dst, ctm, gs, ts,
                 static_cast<std::uint8_t>(b));
    }
}

void HandleTJArray(Raster& dst, const Matrix& ctm,
                   const Gstate& gs, TextState& ts,
                   const Value& arr_v) {
    const auto* arr = std::get_if<
        foundation::objects::Array>(&arr_v.v);
    if (!arr || !ts.font) return;
    for (const auto& item : arr->items) {
        if (const auto* s = std::get_if<
                foundation::objects::String>(&item.v)) {
            ShowString(dst, ctm, gs, ts,
                       std::span<const std::byte>(
                           s->bytes.data(), s->bytes.size()));
        } else {
            // Number: shift the text matrix backward by
            // (-num/1000) * font_size text units.
            const double num =
                std::holds_alternative<std::int64_t>(item.v)
                    ? static_cast<double>(
                          std::get<std::int64_t>(item.v))
                    : (std::holds_alternative<double>(item.v)
                           ? std::get<double>(item.v)
                           : 0.0);
            const double tx =
                ((-num) / 1000.0) * ts.font_size *
                ts.horiz_scale;
            ts.tm = Compose(
                foundation::transform::Translate(tx, 0.0),
                ts.tm);
        }
    }
}

// /Do operator dispatch: resolve an image resource name to a
// ResolvedImage*, build the per-image CTM, and route through
// foundation::image_compositor. Form XObjects are silently
// skipped (Beat 5+).
//
// CTM derivation (image_compositor convention: source-data
// row 0 sits at v=0 in the unit square, with v INCREASING
// DOWNWARD to row H-1 at v=1). PDF spec §8.9.5: the image is
// drawn so that source pixel (0, 0) — top-left of the data —
// lands at PDF user-space (0, 1) (top-left of the unit square,
// y-up). The mapping image_unit → PDF_local is therefore
// {a=1, b=0, c=0, d=-1, e=0, f=1} — a y-flip plus translate.
// Composed onto state.ctm (PDF_local → device) gives the CTM
// the compositor expects: image_unit → device.
// Composite a decoded image (RGBA8, top-row-first) onto `dst`, mapping the
// image's unit square through `state_ctm` (the CTM already places + scales
// the unit square per §8.9.5.1). Shared by the /Do image path and by inline
// images (§8.9.7), which differ only in how the ResolvedImage is obtained.
void CompositeResolvedImage(
        Raster& dst, const Matrix& state_ctm, const ResolvedImage& img,
        double alpha, const foundation::rasterizer::ClipRect* clip) {
    if (!img.valid || img.width == 0 || img.height == 0) return;

    // Top-left of source data → PDF user (0, 1); bottom-right →
    // (1, 0). Row-vector form per the rest of the renderer.
    const Matrix image_unit_to_pdf_local{
        1.0, 0.0,
        0.0, -1.0,
        0.0, 1.0};
    const Matrix composite_ctm = Compose(
        image_unit_to_pdf_local, state_ctm);

    foundation::image_compositor::ImageSource src;
    src.width = img.width;
    src.height = img.height;
    // Non-owning view; `img` outlives this call (a cached ResolvedImage for
    // /Do, a stack local for an inline image), so placement is allocation-
    // free even when the same image is drawn repeatedly (tiling-pattern
    // cells).
    src.pixels = img.rgba8;  // RGBA8, row-major, top-row-first

    foundation::image_compositor::Composite(
        dst, src, composite_ctm, alpha, clip);
}

void HandleDo(Raster& dst,
              const Matrix& state_ctm,
              const std::map<std::string, ResolvedImage>& images,
              const std::string& resource_name,
              double alpha = 1.0,
              const foundation::rasterizer::ClipRect* clip = nullptr) {
    const auto it = images.find(resource_name);
    if (it == images.end()) return;
    CompositeResolvedImage(dst, state_ctm, it->second, alpha, clip);
}

// Expand a §8.9.7 inline-image parameter dict into the full-name dict the
// shared image base path (BuildResolvedImageBase / ResolveColorSpace /
// FilterChain) reads: the abbreviated keys W/H/BPC/CS/F/D/DP/IM/I map to
// Width/Height/BitsPerComponent/ColorSpace/Filter/Decode/DecodeParms/
// ImageMask/Interpolate. The abbreviated colour-space (/G /RGB /CMYK /I) and
// filter (/Fl /DCT /CCF …) *values* are already accepted downstream, so they
// pass through untouched — except a /CS that names an entry in the page's
// /Resources /ColorSpace, which is resolved to that colour-space object.
foundation::objects::Dict ExpandInlineImageHeader(
        const foundation::objects::Dict& in,
        const Dump& dump,
        const foundation::objects::Dict* resources) {
    auto full_key = [](const std::string& k) -> const char* {
        if (k == "W") return "Width";
        if (k == "H") return "Height";
        if (k == "BPC") return "BitsPerComponent";
        if (k == "CS") return "ColorSpace";
        if (k == "F") return "Filter";
        if (k == "D") return "Decode";
        if (k == "DP") return "DecodeParms";
        if (k == "IM") return "ImageMask";
        if (k == "I") return "Interpolate";
        return nullptr;
    };
    foundation::objects::Dict out;
    for (const auto& entry : in.entries) {
        std::string key = entry.first;
        if (const char* fk = full_key(entry.first)) key = fk;

        // A /CS that is a bare name not naming a device space may name an
        // entry in /Resources /ColorSpace (§8.9.5.1). Substitute the
        // referenced colour-space object so ResolveColorSpace can read it.
        if (key == "ColorSpace") {
            if (const auto* nm =
                    std::get_if<std::string>(&entry.second.v)) {
                const bool device =
                    *nm == "DeviceRGB" || *nm == "RGB" ||
                    *nm == "DeviceGray" || *nm == "G" ||
                    *nm == "DeviceCMYK" || *nm == "CMYK";
                if (!device && resources) {
                    if (const auto* csd = DictGet(*resources, "ColorSpace")) {
                        if (const auto* csr = Resolve(dump, *csd)) {
                            if (const auto* csdict = std::get_if<
                                    foundation::objects::Dict>(&csr->v)) {
                                if (const auto* named =
                                        DictGet(*csdict, *nm)) {
                                    out.entries.emplace_back(key, *named);
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
        out.entries.emplace_back(std::move(key), entry.second);
    }
    return out;
}

// Maximum tiling-pattern nesting (a pattern whose content paints
// with another pattern). Bounds recursion; 4 is well past any
// real document.
constexpr int kMaxPatternDepth = 4;

// Forward declaration: the content-stream interpreter recurses
// through FillWithPattern when a path is filled with a tiling
// pattern, so the two functions are mutually recursive.
void RenderContentImpl(
    Raster& dst, const Stream& cs, const Matrix& page_ctm,
    std::span<const std::byte> pdf_bytes, const Dump& dump,
    const foundation::objects::Dict* resources,
    const std::map<std::string, ResolvedFont>& fonts,
    const std::map<std::string, ResolvedImage>& images,
    int depth);

// Resolve /Resources/Pattern/<name> to its stream object.
const foundation::objects::Stream* ResolvePattern(
        const Dump& dump,
        const foundation::objects::Dict& resources,
        const std::string& name) {
    const auto* pat_v = DictGet(resources, "Pattern");
    if (!pat_v) return nullptr;
    const auto* pat_r = Resolve(dump, *pat_v);
    if (!pat_r) return nullptr;
    const auto* pat_d = std::get_if<
        foundation::objects::Dict>(&pat_r->v);
    if (!pat_d) return nullptr;
    const auto* entry = DictGet(*pat_d, name);
    if (!entry) return nullptr;
    const auto* res = Resolve(dump, *entry);
    if (!res) return nullptr;
    return std::get_if<foundation::objects::Stream>(&res->v);
}

// Resolve /Resources/Pattern/<name> to its object value. Tiling
// patterns (PatternType 1) are streams; shading patterns
// (PatternType 2) are plain dictionaries — both reached here.
const Value* ResolvePatternObj(
        const Dump& dump,
        const foundation::objects::Dict& resources,
        const std::string& name) {
    const auto* pat_v = DictGet(resources, "Pattern");
    if (!pat_v) return nullptr;
    const auto* pat_r = Resolve(dump, *pat_v);
    const auto* pat_d = pat_r ? std::get_if<
        foundation::objects::Dict>(&pat_r->v) : nullptr;
    if (!pat_d) return nullptr;
    const auto* entry = DictGet(*pat_d, name);
    return entry ? Resolve(dump, *entry) : nullptr;
}

// The dictionary view of a pattern object (a Stream's header, or
// the dict itself).
const foundation::objects::Dict* PatternDict(const Value& v) {
    if (const auto* st = std::get_if<
            foundation::objects::Stream>(&v.v)) {
        return &st->header;
    }
    return std::get_if<foundation::objects::Dict>(&v.v);
}

// Read a numeric dict entry (resolving an indirect reference);
// `dflt` when the key is absent or not a number.
double DictNumber(const Dump& dump,
                  const foundation::objects::Dict& d,
                  const std::string& key, double dflt) {
    const auto* v = DictGet(d, key);
    if (!v) return dflt;
    const auto* r = Resolve(dump, *v);
    return r ? AsNumber(*r, dflt) : AsNumber(*v, dflt);
}

// Read a /Matrix array (6 numbers) from a dict; identity if
// absent or malformed.
Matrix ReadMatrix(const Dump& dump,
                  const foundation::objects::Dict& d) {
    const auto* mv = DictGet(d, "Matrix");
    if (!mv) return foundation::transform::Identity();
    const auto* r = Resolve(dump, *mv);
    if (!r) return foundation::transform::Identity();
    const auto* arr = std::get_if<
        foundation::objects::Array>(&r->v);
    if (!arr || arr->items.size() != 6) {
        return foundation::transform::Identity();
    }
    return Matrix{
        AsNumber(arr->items[0]), AsNumber(arr->items[1]),
        AsNumber(arr->items[2]), AsNumber(arr->items[3]),
        AsNumber(arr->items[4]), AsNumber(arr->items[5])};
}

// Read an array of numbers (resolving an indirect ref). Empty when
// the value is absent or not a numeric array.
std::vector<double> ReadDoubleArray(const Dump& dump, const Value* v) {
    std::vector<double> out;
    if (!v) return out;
    const auto* r = Resolve(dump, *v);
    const auto* arr = r ? std::get_if<
        foundation::objects::Array>(&r->v) : nullptr;
    if (!arr) return out;
    out.reserve(arr->items.size());
    for (const auto& it : arr->items) out.push_back(AsNumber(it));
    return out;
}

// Evaluate a PDF function (PDF §7.10) at scalar input `t`, returning
// the output component vector. Supports FunctionType 2 (exponential
// interpolation) and 3 (stitching), plus an array-of-functions (each
// scalar-valued, concatenated). Other types (0 sampled, 4 PostScript)
// return empty — callers treat that as "no colour".
std::vector<double> EvalPdfFunction(const Dump& dump, const Value& fv,
                                    double t, int depth) {
    if (depth > 8) return {};
    const auto* r = Resolve(dump, fv);
    if (!r) return {};
    if (const auto* arr = std::get_if<
            foundation::objects::Array>(&r->v)) {
        std::vector<double> out;
        for (const auto& it : arr->items) {
            auto c = EvalPdfFunction(dump, it, t, depth + 1);
            for (double v : c) out.push_back(v);
        }
        return out;
    }
    const foundation::objects::Dict* d = nullptr;
    if (const auto* st = std::get_if<
            foundation::objects::Stream>(&r->v)) {
        d = &st->header;
    } else {
        d = std::get_if<foundation::objects::Dict>(&r->v);
    }
    if (!d) return {};

    const auto domain = ReadDoubleArray(dump, DictGet(*d, "Domain"));
    double d0 = 0.0, d1 = 1.0;
    if (domain.size() >= 2) { d0 = domain[0]; d1 = domain[1]; }
    if (t < std::min(d0, d1)) t = std::min(d0, d1);
    if (t > std::max(d0, d1)) t = std::max(d0, d1);

    const int ftype = static_cast<int>(
        DictNumber(dump, *d, "FunctionType", -1.0));
    if (ftype == 2) {
        auto c0 = ReadDoubleArray(dump, DictGet(*d, "C0"));
        auto c1 = ReadDoubleArray(dump, DictGet(*d, "C1"));
        if (c0.empty()) c0 = {0.0};
        if (c1.empty()) c1 = {1.0};
        const double n = DictNumber(dump, *d, "N", 1.0);
        const double s = (d1 != d0) ? (t - d0) / (d1 - d0) : 0.0;
        const double sn = std::pow(s < 0.0 ? 0.0 : s, n);
        const std::size_t m = std::max(c0.size(), c1.size());
        std::vector<double> out(m, 0.0);
        for (std::size_t i = 0; i < m; ++i) {
            const double a = i < c0.size() ? c0[i] : 0.0;
            const double b = i < c1.size() ? c1[i] : 0.0;
            out[i] = a + sn * (b - a);
        }
        return out;
    }
    if (ftype == 3) {
        const auto* funcs_v = DictGet(*d, "Functions");
        const auto* fr = funcs_v ? Resolve(dump, *funcs_v) : nullptr;
        const auto* farr = fr ? std::get_if<
            foundation::objects::Array>(&fr->v) : nullptr;
        if (!farr || farr->items.empty()) return {};
        const auto bounds = ReadDoubleArray(dump, DictGet(*d, "Bounds"));
        const auto encode = ReadDoubleArray(dump, DictGet(*d, "Encode"));
        int k = 0;
        const int kmax = static_cast<int>(farr->items.size()) - 1;
        while (k < static_cast<int>(bounds.size()) && k < kmax
               && t >= bounds[k]) {
            ++k;
        }
        const double lo = (k == 0) ? d0 : bounds[k - 1];
        const double hi = (k >= static_cast<int>(bounds.size()))
            ? d1 : bounds[k];
        double e0 = 0.0, e1 = 1.0;
        if (2 * k + 1 < static_cast<int>(encode.size())) {
            e0 = encode[2 * k]; e1 = encode[2 * k + 1];
        }
        const double tt = (hi != lo)
            ? e0 + (t - lo) * (e1 - e0) / (hi - lo) : e0;
        return EvalPdfFunction(dump, farr->items[k], tt, depth + 1);
    }
    return {};
}

// Map a shading colour-component vector to sRGB via the shading's
// /ColorSpace (DeviceRGB / DeviceGray / DeviceCMYK supported).
foundation::colorspace::ColorRgb ShadingComponentsToRgb(
        const Dump& dump, const foundation::objects::Dict& sh,
        const std::vector<double>& c) {
    std::string csn = "DeviceRGB";
    if (const auto* csv = DictGet(sh, "ColorSpace")) {
        if (const auto* cr = Resolve(dump, *csv)) {
            if (const auto* nm = std::get_if<std::string>(&cr->v)) {
                csn = *nm;
            }
        }
    }
    if ((csn == "DeviceGray" || csn == "G") && c.size() >= 1) {
        return {c[0], c[0], c[0]};
    }
    if ((csn == "DeviceCMYK" || csn == "CMYK") && c.size() >= 4) {
        return CmykToRgb(c[0], c[1], c[2], c[3]);
    }
    if (c.size() >= 3) return {c[0], c[1], c[2]};
    if (c.size() == 1) return {c[0], c[0], c[0]};
    return {0.0, 0.0, 0.0};
}

// Map a component vector to sRGB given an alternate-space FAMILY name
// (DeviceGray / DeviceRGB / DeviceCMYK) — the tail conversion for a
// /Separation or /DeviceN tint transform's output.
foundation::colorspace::ColorRgb AltComponentsToRgb(
        const std::string& family, const std::vector<double>& c) {
    if ((family == "DeviceCMYK" || family == "CMYK") && c.size() >= 4)
        return CmykToRgb(c[0], c[1], c[2], c[3]);
    if ((family == "DeviceGray" || family == "G") && !c.empty())
        return {c[0], c[0], c[0]};
    if (c.size() >= 3) return {c[0], c[1], c[2]};
    if (c.size() == 1) return {c[0], c[0], c[0]};
    return {0.0, 0.0, 0.0};
}

// Resolve /Resources/ColorSpace/<name> to a /Separation (or single-input
// /DeviceN) descriptor: the tint-transform function value + the
// alternate-space family name. Returns {nullptr, ""} for anything else
// (the caller then treats the space as a plain device space). Only
// 1-input Separations are handled here; multi-input /DeviceN keeps the
// device fallback.
struct SeparationCs {
    const Value* tint = nullptr;
    std::string alt;
};
SeparationCs ResolveSeparationCs(
        const Dump& dump,
        const foundation::objects::Dict* resources,
        const std::string& name) {
    SeparationCs out;
    if (!resources) return out;
    const auto* csd_v = DictGet(*resources, "ColorSpace");
    const auto* csd_r = csd_v ? Resolve(dump, *csd_v) : nullptr;
    const auto* csd = csd_r ? std::get_if<
        foundation::objects::Dict>(&csd_r->v) : nullptr;
    if (!csd) return out;
    const auto* entry = DictGet(*csd, name);
    const auto* er = entry ? Resolve(dump, *entry) : nullptr;
    const auto* arr = er ? std::get_if<
        foundation::objects::Array>(&er->v) : nullptr;
    if (!arr || arr->items.size() < 4) return out;
    const auto* fam = std::get_if<std::string>(&arr->items[0].v);
    if (!fam || (*fam != "Separation" && *fam != "DeviceN")) return out;
    // /DeviceN with >1 colorant has a multi-input tint transform that the
    // 1-input EvalPdfFunction can't drive; leave it to the device fallback.
    if (*fam == "DeviceN") {
        const auto* nr = Resolve(dump, arr->items[1]);
        const auto* names = nr ? std::get_if<
            foundation::objects::Array>(&nr->v) : nullptr;
        if (names && names->items.size() != 1) return out;
    }
    // Alternate space (item 2): a device name, or an array (e.g. ICCBased
    // → device by /N).
    const auto* alt_r = Resolve(dump, arr->items[2]);
    if (alt_r) {
        if (const auto* an = std::get_if<std::string>(&alt_r->v)) {
            out.alt = *an;
        } else if (const auto* aa = std::get_if<
                       foundation::objects::Array>(&alt_r->v)) {
            if (!aa->items.empty()) {
                const auto* afam = std::get_if<std::string>(
                    &aa->items[0].v);
                if (afam && *afam == "ICCBased" && aa->items.size() >= 2) {
                    if (const auto* sr = Resolve(dump, aa->items[1])) {
                        if (const auto* st = std::get_if<
                                foundation::objects::Stream>(&sr->v)) {
                            const int n = static_cast<int>(
                                DictNumber(dump, st->header, "N", 0));
                            out.alt = (n == 1) ? "DeviceGray"
                                    : (n == 4) ? "DeviceCMYK" : "DeviceRGB";
                        }
                    }
                }
            }
        }
    }
    out.tint = &arr->items[3];
    return out;
}

// A ShadingType 3 (radial / two-circle) shading prepared for fast
// per-pixel evaluation. The gradient colour depends only on the 1-D
// parameter s in [0,1], so the PDF colour function is sampled ONCE
// into a fixed-size ramp at build time; the per-pixel hot path then
// solves the two-circle quadratic and interpolates the ramp instead
// of re-evaluating the function (and re-allocating its component
// vectors) for every covered pixel — the difference between an 86 s
// render and a sub-second one on shading-heavy pages. PDF §8.7.4.5.4.
constexpr int kShadingRampN = 256;

struct RadialShading {
    bool valid = false;
    double x0 = 0.0, y0 = 0.0, r0 = 0.0;
    double dcx = 0.0, dcy = 0.0, dr = 0.0;     // (x1,y1,r1) - (x0,y0,r0)
    bool ext0 = false, ext1 = false;
    std::array<foundation::colorspace::ColorRgb, kShadingRampN> ramp{};
};

// Read Coords/Domain/Extend/Function and bake the colour ramp once.
RadialShading BuildRadialShading(const Dump& dump,
                                 const foundation::objects::Dict& sh) {
    RadialShading rs;
    const auto co = ReadDoubleArray(dump, DictGet(sh, "Coords"));
    if (co.size() < 6) return rs;
    rs.x0 = co[0]; rs.y0 = co[1]; rs.r0 = co[2];
    const double x1 = co[3], y1 = co[4], r1 = co[5];
    rs.dcx = x1 - rs.x0; rs.dcy = y1 - rs.y0; rs.dr = r1 - rs.r0;
    const auto dom = ReadDoubleArray(dump, DictGet(sh, "Domain"));
    const double t0 = dom.size() >= 2 ? dom[0] : 0.0;
    const double t1 = dom.size() >= 2 ? dom[1] : 1.0;
    if (const auto* ev = DictGet(sh, "Extend")) {
        if (const auto* er = Resolve(dump, *ev)) {
            if (const auto* ea = std::get_if<
                    foundation::objects::Array>(&er->v)) {
                if (ea->items.size() >= 2) {
                    if (const auto* b = std::get_if<bool>(
                            &ea->items[0].v)) rs.ext0 = *b;
                    if (const auto* b = std::get_if<bool>(
                            &ea->items[1].v)) rs.ext1 = *b;
                }
            }
        }
    }
    const auto* func = DictGet(sh, "Function");
    if (!func) return rs;
    for (int k = 0; k < kShadingRampN; ++k) {
        const double s = static_cast<double>(k) / (kShadingRampN - 1);
        const double tparam = t0 + s * (t1 - t0);
        const auto comps = EvalPdfFunction(dump, *func, tparam, 0);
        if (comps.empty()) return rs;   // unsupported function -> invalid
        rs.ramp[k] = ShadingComponentsToRgb(dump, sh, comps);
    }
    rs.valid = true;
    return rs;
}

// Evaluate the prepared radial shading at (px, py) in shading space.
// Returns false when the point is not covered (outside both circles
// with /Extend false). Equivalent geometry to evaluating the function
// directly; only the colour lookup is ramp-interpolated.
bool SampleRadialShading(const RadialShading& rs, double px, double py,
                         foundation::colorspace::ColorRgb& out) {
    const double pxr = px - rs.x0, pyr = py - rs.y0;
    const double a = rs.dcx * rs.dcx + rs.dcy * rs.dcy - rs.dr * rs.dr;
    const double b = -2.0 * (pxr * rs.dcx + pyr * rs.dcy + rs.r0 * rs.dr);
    const double c = pxr * pxr + pyr * pyr - rs.r0 * rs.r0;

    // Collect candidate s values (largest first).
    double cand[2];
    int ncand = 0;
    if (std::abs(a) < 1e-9) {
        if (std::abs(b) > 1e-12) cand[ncand++] = -c / b;
    } else {
        const double disc = b * b - 4.0 * a * c;
        if (disc >= 0.0) {
            const double sq = std::sqrt(disc);
            const double s1 = (-b + sq) / (2.0 * a);
            const double s2 = (-b - sq) / (2.0 * a);
            cand[0] = std::max(s1, s2);
            cand[1] = std::min(s1, s2);
            ncand = 2;
        }
    }
    for (int i = 0; i < ncand; ++i) {
        const double s = cand[i];
        if (rs.r0 + s * rs.dr < 0.0) continue;  // negative radius
        double s_eff;
        if (s >= 0.0 && s <= 1.0) s_eff = s;
        else if (s > 1.0 && rs.ext1) s_eff = 1.0;
        else if (s < 0.0 && rs.ext0) s_eff = 0.0;
        else continue;                          // outside, no extend
        // Interpolate between adjacent ramp entries.
        const double f = s_eff * (kShadingRampN - 1);
        int lo = static_cast<int>(f);
        if (lo < 0) lo = 0;
        if (lo > kShadingRampN - 1) lo = kShadingRampN - 1;
        const int hi = lo < kShadingRampN - 1 ? lo + 1 : lo;
        const double frac = f - lo;
        const auto& A = rs.ramp[lo];
        const auto& B = rs.ramp[hi];
        out.r = A.r + (B.r - A.r) * frac;
        out.g = A.g + (B.g - A.g) * frac;
        out.b = A.b + (B.b - A.b) * frac;
        return true;
    }
    return false;
}

// Fill `path` with a PatternType 2 (shading) pattern: evaluate the
// shading colour per pixel inside the path's coverage mask. Only
// ShadingType 3 (radial) is supported (the only type in scope);
// other shading types leave the path unpainted.
void FillWithShadingPattern(
        Raster& dst, const Path& path, const Matrix& path_ctm,
        FillRule rule, const foundation::objects::Dict& pat_dict,
        const Matrix& page_ctm, const Dump& dump, double alpha = 1.0,
        const foundation::rasterizer::ClipRect* clip = nullptr) {
    if (dst.width == 0 || dst.height == 0) return;
    const auto* shv = DictGet(pat_dict, "Shading");
    const auto* shr = shv ? Resolve(dump, *shv) : nullptr;
    if (!shr) return;
    const foundation::objects::Dict* sd = nullptr;
    if (const auto* st = std::get_if<
            foundation::objects::Stream>(&shr->v)) {
        sd = &st->header;
    } else {
        sd = std::get_if<foundation::objects::Dict>(&shr->v);
    }
    if (!sd) return;
    if (DictNumber(dump, *sd, "ShadingType", 0.0) != 3.0) {
        return;  // only radial (type 3) supported
    }
    const Matrix sh_ctm = Compose(ReadMatrix(dump, pat_dict), page_ctm);
    const auto sh_inv = foundation::transform::Inverse(sh_ctm);
    if (!sh_inv) return;

    // Bake the gradient colour ramp ONCE (not per pixel). Bail before
    // touching the raster if the shading's function is unsupported.
    const RadialShading shading = BuildRadialShading(dump, *sd);
    if (!shading.valid) return;

    // Device bbox of the path, clamped to the raster.
    double dminx = 1e30, dminy = 1e30, dmaxx = -1e30, dmaxy = -1e30;
    for (const auto& seg : path.segments) {
        const int npts = (seg.kind == SegmentKind::Move ||
                          seg.kind == SegmentKind::Line) ? 1
                       : (seg.kind == SegmentKind::Rect) ? 2 : 0;
        for (int k = 0; k < npts; ++k) {
            const auto p = foundation::transform::Apply(
                path_ctm, seg.pts[k]);
            dminx = std::min(dminx, p.x); dmaxx = std::max(dmaxx, p.x);
            dminy = std::min(dminy, p.y); dmaxy = std::max(dmaxy, p.y);
        }
    }
    if (dminx > dmaxx) return;
    auto clampu = [](double v, std::uint32_t hi) -> std::uint32_t {
        if (v < 0.0) return 0;
        if (v >= static_cast<double>(hi)) return hi;
        return static_cast<std::uint32_t>(v);
    };
    const std::uint32_t bx0 = clampu(std::floor(dminx), dst.width);
    const std::uint32_t bx1 = clampu(std::ceil(dmaxx), dst.width);
    const std::uint32_t by0 = clampu(std::floor(dminy), dst.height);
    const std::uint32_t by1 = clampu(std::ceil(dmaxy), dst.height);
    if (bx1 <= bx0 || by1 <= by0) return;
    const std::uint32_t mw = bx1 - bx0, mh = by1 - by0;

    // Path coverage mask, sized to the path bbox (not the whole page);
    // its alpha channel is the clip weight. Drawn in bbox-local device
    // coords via an INTEGER translate of path_ctm, so coverage is
    // byte-identical to a full-page mask while skipping a full-page
    // allocate+zero on every shading fill.
    Raster mask;
    mask.width = mw;
    mask.height = mh;
    mask.pixels.assign(static_cast<std::size_t>(mw) * mh * 4u, 0u);
    const Matrix mask_ctm = Compose(
        path_ctm,
        foundation::transform::Translate(-static_cast<double>(bx0),
                                         -static_cast<double>(by0)));
    foundation::rasterizer::Fill(
        mask, path, mask_ctm,
        foundation::colorspace::ColorRgb{1.0, 1.0, 1.0}, 1.0, rule);

    // Read range = bbox intersected with the active clip rect.
    std::uint32_t x0 = bx0, x1 = bx1, y0 = by0, y1 = by1;
    if (clip != nullptr) {
        x0 = std::max(x0, clampu(clip->x0, dst.width));
        x1 = std::min(x1, clampu(clip->x1, dst.width));
        y0 = std::max(y0, clampu(clip->y0, dst.height));
        y1 = std::min(y1, clampu(clip->y1, dst.height));
    }

    for (std::uint32_t py = y0; py < y1; ++py) {
        for (std::uint32_t px = x0; px < x1; ++px) {
            const std::size_t idx =
                (static_cast<std::size_t>(py) * dst.width + px) * 4u;
            const std::size_t midx =
                (static_cast<std::size_t>(py - by0) * mw
                 + (px - bx0)) * 4u;
            const double clip = mask.pixels[midx + 3] / 255.0;
            if (clip <= 0.0) continue;
            const auto sp = foundation::transform::Apply(
                *sh_inv,
                foundation::transform::Point{px + 0.5, py + 0.5});
            foundation::colorspace::ColorRgb rgb;
            if (!SampleRadialShading(shading, sp.x, sp.y, rgb)) {
                continue;
            }
            // Constant alpha (§11.6.4.4) scales the source coverage:
            // the effective src-over weight is clip × ca.
            const double eff = clip * alpha;
            if (eff <= 0.0) continue;
            const double inv = 1.0 - eff;
            dst.pixels[idx + 0] = foundation::colorspace::Quantize(
                rgb.r * eff + dst.pixels[idx + 0] / 255.0 * inv);
            dst.pixels[idx + 1] = foundation::colorspace::Quantize(
                rgb.g * eff + dst.pixels[idx + 1] / 255.0 * inv);
            dst.pixels[idx + 2] = foundation::colorspace::Quantize(
                rgb.b * eff + dst.pixels[idx + 2] / 255.0 * inv);
            const double da = dst.pixels[idx + 3] / 255.0;
            dst.pixels[idx + 3] = foundation::colorspace::Quantize(
                eff + da * inv);
        }
    }
}

// Fill `path` (in current-user space via `path_ctm`) with a
// PatternType 1 tiling pattern, clipped to the path's coverage.
// The pattern is executed in its own coordinate system —
// patternMatrix composed onto the PAGE default CTM (`page_ctm`),
// NOT the current CTM (PDF §8.7.3.1) — and tiled across the
// area the path covers. Shading patterns (PatternType 2) and
// uncoloured patterns (PaintType 2) are out of v1 scope and
// leave the path unpainted rather than filling it solid black.
void FillWithPattern(
        Raster& dst, const Path& path, const Matrix& path_ctm,
        FillRule rule, const std::string& pattern_name,
        const Matrix& page_ctm,
        std::span<const std::byte> pdf_bytes, const Dump& dump,
        const foundation::objects::Dict* resources, int depth,
        double alpha = 1.0,
        const foundation::rasterizer::ClipRect* clip = nullptr) {
    if (depth >= kMaxPatternDepth || !resources) return;
    if (dst.width == 0 || dst.height == 0) return;
    const auto* patobj = ResolvePatternObj(dump, *resources,
                                           pattern_name);
    if (!patobj) return;
    const auto* pdict = PatternDict(*patobj);
    if (!pdict) return;

    const double ptype = DictNumber(dump, *pdict, "PatternType", 1.0);
    if (ptype == 2.0) {
        // Shading pattern — evaluate the gradient per pixel.
        FillWithShadingPattern(dst, path, path_ctm, rule, *pdict,
                               page_ctm, dump, alpha, clip);
        return;
    }
    // Tiling patterns (PatternType 1) below. PaintType 2 (uncoloured)
    // needs a base colour from scn's numeric operands — out of scope.
    if (ptype != 1.0) return;
    const auto* pat = std::get_if<
        foundation::objects::Stream>(&patobj->v);
    if (!pat) return;  // tiling pattern must be a stream
    const double xstep = DictNumber(dump, pat->header, "XStep", 0.0);
    const double ystep = DictNumber(dump, pat->header, "YStep", 0.0);
    if (std::abs(xstep) < 1e-6 || std::abs(ystep) < 1e-6) return;

    // Pattern content + its own resources.
    Stream pcs;
    // Kept alive for the whole tile loop below: an inline image (§8.9.7) in
    // `pcs` carries a Stream operand spanning into these bytes.
    std::vector<std::byte> body;
    try {
        body = DecodeStream(pdf_bytes, *pat);
        if (body.empty()) return;
        pcs = foundation::content_stream::Parse(
            std::span<const std::byte>(body.data(), body.size()));
    } catch (...) {
        return;
    }
    const foundation::objects::Dict* pat_res = nullptr;
    if (const auto* rv = DictGet(pat->header, "Resources")) {
        if (const auto* rr = Resolve(dump, *rv)) {
            pat_res = std::get_if<
                foundation::objects::Dict>(&rr->v);
        }
    }
    std::map<std::string, ResolvedFont> pfonts;
    std::map<std::string, ResolvedImage> pimages;
    if (pat_res) {
        pfonts = ResolveFontsFromResources(pdf_bytes, dump, *pat_res);
        pimages = ResolveImagesFromResources(pdf_bytes, dump, *pat_res);
    }

    // Pattern space → device. Pattern matrix maps to the page's
    // default coordinate system, not the current CTM.
    const Matrix pat_base = Compose(
        ReadMatrix(dump, pat->header), page_ctm);
    const auto pat_base_inv = foundation::transform::Inverse(pat_base);
    if (!pat_base_inv) return;

    // Device-space bounding box of the path — bounds both the coverage
    // mask and (mapped to pattern space below) the tile loop.
    double dminx = 1e30, dminy = 1e30, dmaxx = -1e30, dmaxy = -1e30;
    for (const auto& seg : path.segments) {
        const int npts = (seg.kind == SegmentKind::Move ||
                          seg.kind == SegmentKind::Line) ? 1
                       : (seg.kind == SegmentKind::Rect) ? 2 : 0;
        for (int k = 0; k < npts; ++k) {
            const auto p = foundation::transform::Apply(
                path_ctm, seg.pts[k]);
            dminx = std::min(dminx, p.x); dmaxx = std::max(dmaxx, p.x);
            dminy = std::min(dminy, p.y); dmaxy = std::max(dmaxy, p.y);
        }
    }
    if (dminx > dmaxx || dminy > dmaxy) return;  // empty path

    // Pixel bbox, clamped to the raster.
    auto clampu = [](double v, std::uint32_t hi) -> std::uint32_t {
        if (v < 0.0) return 0;
        if (v >= static_cast<double>(hi)) return hi;
        return static_cast<std::uint32_t>(v);
    };
    const std::uint32_t bx0 = clampu(std::floor(dminx), dst.width);
    const std::uint32_t bx1 = clampu(std::ceil(dmaxx), dst.width);
    const std::uint32_t by0 = clampu(std::floor(dminy), dst.height);
    const std::uint32_t by1 = clampu(std::ceil(dmaxy), dst.height);
    if (bx1 <= bx0 || by1 <= by0) return;
    const std::uint32_t mw = bx1 - bx0, mh = by1 - by0;

    // Path coverage mask, sized to the path bbox (not the whole page);
    // its alpha channel is the clip weight. Drawn in bbox-local device
    // coords via an INTEGER translate of path_ctm, so coverage is
    // byte-identical to a full-page mask while skipping a full-page
    // allocate+zero on every tiling fill.
    Raster mask;
    mask.width = mw;
    mask.height = mh;
    mask.pixels.assign(static_cast<std::size_t>(mw) * mh * 4u, 0u);
    const Matrix mask_ctm = Compose(
        path_ctm,
        foundation::transform::Translate(-static_cast<double>(bx0),
                                         -static_cast<double>(by0)));
    foundation::rasterizer::Fill(
        mask, path, mask_ctm,
        foundation::colorspace::ColorRgb{1.0, 1.0, 1.0}, 1.0, rule);

    // Map the four device-bbox corners back through the inverse
    // pattern CTM to bound the tile loop in pattern space.
    double pminx = 1e30, pminy = 1e30, pmaxx = -1e30, pmaxy = -1e30;
    for (const auto& c : {
            foundation::transform::Point{dminx, dminy},
            foundation::transform::Point{dmaxx, dminy},
            foundation::transform::Point{dmaxx, dmaxy},
            foundation::transform::Point{dminx, dmaxy}}) {
        const auto q = foundation::transform::Apply(*pat_base_inv, c);
        pminx = std::min(pminx, q.x); pmaxx = std::max(pmaxx, q.x);
        pminy = std::min(pminy, q.y); pmaxy = std::max(pmaxy, q.y);
    }
    // Tile index ranges. Pattern cell i occupies pattern-space
    // x ≈ [i·XStep + BBox] — widen by a cell on each side so a
    // cell straddling the edge is included.
    const long i0 = static_cast<long>(std::floor(pminx / xstep)) - 1;
    const long i1 = static_cast<long>(std::ceil(pmaxx / xstep)) + 1;
    const long j0 = static_cast<long>(std::floor(pminy / ystep)) - 1;
    const long j1 = static_cast<long>(std::ceil(pmaxy / ystep)) + 1;
    // Guard against pathological tile counts.
    constexpr long kMaxTiles = 100000;
    if ((i1 - i0) * (j1 - j0) > kMaxTiles) return;

    // Render the tiled pattern into a transparent scratch buffer.
    Raster tile_buf;
    tile_buf.width = dst.width;
    tile_buf.height = dst.height;
    tile_buf.pixels.assign(
        static_cast<std::size_t>(dst.width) * dst.height * 4u, 0u);
    for (long j = j0; j <= j1; ++j) {
        for (long i = i0; i <= i1; ++i) {
            const Matrix tile_ctm = Compose(
                foundation::transform::Translate(
                    static_cast<double>(i) * xstep,
                    static_cast<double>(j) * ystep),
                pat_base);
            RenderContentImpl(tile_buf, pcs, tile_ctm, pdf_bytes,
                              dump, pat_res, pfonts, pimages,
                              depth + 1);
        }
    }

    // Composite scratch onto dst, weighted by the clip coverage.
    // tile_buf was built by compositing onto a transparent canvas, so
    // its colour channels are ALREADY premultiplied by the tile alpha
    // (foundation::image_compositor stores premultiplied output). The
    // clip mask scales the source contribution; the correct
    // premultiplied src-over is therefore
    //   out_rgb = tile_rgb * clip + dst_rgb * (1 - tile_a * clip)
    // Treating tile_rgb as a straight (un-premultiplied) colour and
    // multiplying it by tile_a*clip would darken the boundary where
    // the pattern image edge coincides with the fill-path edge — the
    // visible rim around tiling patterns.
    // Composite range = path bbox, narrowed to the active clip
    // rectangle (§8.5.4) when one is set. Pixels outside the bbox have
    // zero coverage, so bounding the loop only skips known no-op work
    // (output-preserving vs. the previous whole-raster sweep).
    std::uint32_t cx0 = bx0, cy0 = by0, cx1 = bx1, cy1 = by1;
    if (clip != nullptr) {
        cx0 = std::max(cx0, clampu(clip->x0, dst.width));
        cx1 = std::min(cx1, clampu(clip->x1, dst.width));
        cy0 = std::max(cy0, clampu(clip->y0, dst.height));
        cy1 = std::min(cy1, clampu(clip->y1, dst.height));
    }
    for (std::uint32_t py = cy0; py < cy1; ++py) {
        for (std::uint32_t px = cx0; px < cx1; ++px) {
            const std::size_t idx =
                (static_cast<std::size_t>(py) * dst.width + px) * 4u;
            const std::size_t midx =
                (static_cast<std::size_t>(py - by0) * mw
                 + (px - bx0)) * 4u;
            const double cov_mask = mask.pixels[midx + 3] / 255.0;
            if (cov_mask <= 0.0) continue;
            const double sa = tile_buf.pixels[idx + 3] / 255.0;
            const double cov = sa * cov_mask * alpha;  // effective src a
            if (cov <= 0.0) continue;
            const double inv = 1.0 - cov;
            for (int ch = 0; ch < 3; ++ch) {
                const double s = tile_buf.pixels[idx + ch] / 255.0;
                const double d = dst.pixels[idx + ch] / 255.0;
                dst.pixels[idx + ch] =
                    foundation::colorspace::Quantize(s * cov_mask + d * inv);
            }
            const double da = dst.pixels[idx + 3] / 255.0;
            dst.pixels[idx + 3] =
                foundation::colorspace::Quantize(cov + da * inv);
        }
    }
}

// Render a /Subtype /Form XObject invoked by `/Do`. The form's
// content stream runs in its own coordinate system — formMatrix
// composed onto the CURRENT CTM at the /Do site (PDF §8.10) — with
// its own /Resources (inheriting the caller's when absent). Mirrors
// the FillWithPattern recursion. Returns false when `name` is not a
// Form XObject (the caller then tries the image path); true when a
// form was handled (even if it drew nothing).
//
// v1 limitation: the form /BBox is NOT applied as a clip — there is
// no clip-path infrastructure yet. Content drawn outside the BBox
// (rare; flattened appearance streams stay inside) is not clipped.
// Core form executor: run form stream `st` with `base_ctm` as the
// pre-formMatrix CTM. The form's own /Matrix is composed on top
// (formMatrix ∘ base_ctm, §8.10); /Resources inherit
// `fallback_resources` when the form carries none. Shared by the
// `/Do` Form path and by annotation /AP appearance rendering.
void RenderFormStream(
        Raster& dst, const Matrix& base_ctm,
        const foundation::objects::Stream& st,
        std::span<const std::byte> pdf_bytes, const Dump& dump,
        const foundation::objects::Dict* fallback_resources,
        int depth) {
    const Matrix form_ctm = Compose(ReadMatrix(dump, st.header),
                                    base_ctm);
    Stream fcs;
    // `body` outlives the parse: any inline image (§8.9.7) in `fcs` carries a
    // Stream operand whose body is a span into these bytes, read later by
    // RenderContentImpl. A try-local buffer would dangle.
    std::vector<std::byte> body;
    try {
        body = DecodeStream(pdf_bytes, st);
        if (body.empty()) return;
        fcs = foundation::content_stream::Parse(
            std::span<const std::byte>(body.data(), body.size()));
    } catch (...) {
        return;  // malformed form content — skip
    }
    const foundation::objects::Dict* form_res = fallback_resources;
    if (const auto* rv = DictGet(st.header, "Resources")) {
        if (const auto* rr = Resolve(dump, *rv)) {
            if (const auto* rd = std::get_if<
                    foundation::objects::Dict>(&rr->v)) {
                form_res = rd;
            }
        }
    }
    std::map<std::string, ResolvedFont> ffonts;
    std::map<std::string, ResolvedImage> fimages;
    if (form_res) {
        ffonts = ResolveFontsFromResources(pdf_bytes, dump, *form_res);
        fimages = ResolveImagesFromResources(
            pdf_bytes, dump, *form_res);
    }
    RenderContentImpl(dst, fcs, form_ctm, pdf_bytes, dump, form_res,
                      ffonts, fimages, depth + 1);
}

bool HandleFormXObject(
        Raster& dst, const Matrix& state_ctm,
        const std::string& name,
        std::span<const std::byte> pdf_bytes, const Dump& dump,
        const foundation::objects::Dict* resources, int depth) {
    if (!resources || depth >= kMaxPatternDepth) return false;
    const auto* xo_v = DictGet(*resources, "XObject");
    if (!xo_v) return false;
    const auto* xo_r = Resolve(dump, *xo_v);
    const auto* xo_d = xo_r ? std::get_if<
        foundation::objects::Dict>(&xo_r->v) : nullptr;
    if (!xo_d) return false;
    const auto* entry = DictGet(*xo_d, name);
    if (!entry) return false;
    const auto* res = Resolve(dump, *entry);
    const auto* st = res ? std::get_if<
        foundation::objects::Stream>(&res->v) : nullptr;
    if (!st) return false;
    const auto* sub = DictGet(st->header, "Subtype");
    const auto* sub_n = sub ? std::get_if<std::string>(&sub->v)
                            : nullptr;
    if (!sub_n || *sub_n != "Form") return false;  // not a form
    RenderFormStream(dst, state_ctm, *st, pdf_bytes, dump,
                     resources, depth);
    return true;
}

// Walk a content stream, dispatching the supported operators
// onto the rasterizer + glyph_rasterizer + image_compositor.
void RenderContentImpl(
        Raster& dst,
        const Stream& cs,
        const Matrix& page_ctm,
        std::span<const std::byte> pdf_bytes,
        const Dump& dump,
        const foundation::objects::Dict* resources,
        const std::map<std::string, ResolvedFont>& fonts,
        const std::map<std::string, ResolvedImage>& images,
        int depth) {
    Gstate state{
        page_ctm,
        foundation::colorspace::ColorRgb{0.0, 0.0, 0.0},
        foundation::colorspace::ColorRgb{0.0, 0.0, 0.0},
        1.0};
    std::vector<Gstate> stack;
    Path current;
    PathCursor cursor;
    TextState ts;

    // `W` / `W*` mark the current path as a pending clip; per §8.5.4
    // the clip takes effect AFTER the following path-painting operator
    // (commonly `n`). reset_path() — called by every painting op —
    // intersects the path's device bbox into the gstate clip first.
    bool pending_clip = false;

    // Active clip as a rasterizer::ClipRect pointer (nullptr = none),
    // for threading into the pixel-writing primitives.
    auto clip_ptr = [&]() -> const foundation::rasterizer::ClipRect* {
        return state.has_clip ? &state.clip : nullptr;
    };

    auto reset_path = [&]() {
        if (pending_clip) {
            pending_clip = false;
            double minx = 1e30, miny = 1e30, maxx = -1e30, maxy = -1e30;
            for (const auto& seg : current.segments) {
                const int npts = (seg.kind == SegmentKind::Move ||
                                  seg.kind == SegmentKind::Line) ? 1
                               : (seg.kind == SegmentKind::Rect) ? 2 : 0;
                for (int k = 0; k < npts; ++k) {
                    const auto p = foundation::transform::Apply(
                        state.ctm, seg.pts[k]);
                    minx = std::min(minx, p.x); maxx = std::max(maxx, p.x);
                    miny = std::min(miny, p.y); maxy = std::max(maxy, p.y);
                }
            }
            if (maxx >= minx && maxy >= miny) {
                foundation::rasterizer::ClipRect nc{
                    static_cast<std::int32_t>(std::floor(minx)),
                    static_cast<std::int32_t>(std::floor(miny)),
                    static_cast<std::int32_t>(std::ceil(maxx)),
                    static_cast<std::int32_t>(std::ceil(maxy))};
                if (state.has_clip) {  // intersect with the existing clip
                    nc.x0 = std::max(nc.x0, state.clip.x0);
                    nc.y0 = std::max(nc.y0, state.clip.y0);
                    nc.x1 = std::min(nc.x1, state.clip.x1);
                    nc.y1 = std::min(nc.y1, state.clip.y1);
                }
                state.clip = nc;
                state.has_clip = true;
            }
            // A degenerate (empty) clip path leaves the clip unchanged.
        }
        current.segments.clear();
        cursor.has_current = false;
    };

    // Fill the current path with the active non-stroking colour OR,
    // when /Pattern is selected, route through FillWithPattern. Used
    // by f / f* AND by the fill side of B / B* / b / b* — without
    // this the fill+stroke operators ignored a pattern fill and
    // painted the stale (default-black) fill colour, leaving black
    // blobs wherever a shading pattern (which FillWithPattern leaves
    // unpainted) was selected. (e.g. the gradient circle/star on
    // features.pdf p12.)
    auto fill_current = [&](FillRule rule) {
        if (state.fill_is_pattern && !state.fill_pattern.empty()) {
            FillWithPattern(dst, current, state.ctm, rule,
                            state.fill_pattern, page_ctm, pdf_bytes,
                            dump, resources, depth, state.fill_alpha,
                            clip_ptr());
        } else {
            foundation::rasterizer::Fill(
                dst, current, state.ctm, state.fill_color,
                state.fill_alpha, rule, clip_ptr());
        }
    };

    for (const auto& op : cs.operations) {
        const auto& name = op.op;
        const auto& a = op.operands;

        if (name == "q") {
            stack.push_back(state);
        } else if (name == "Q") {
            if (!stack.empty()) {
                state = stack.back();
                stack.pop_back();
            }
        } else if (name == "cm" && a.size() == 6) {
            // PDF 32000 §8.4.4 + §8.3.5: each new cm
            // left-multiplies the CTM (most recent cm sits
            // closest to local coords). pt × m × state.ctm
            // = pt_dev means new state.ctm = m × state.ctm.
            // Same direction as Td/TD/T*/'/" updates on
            // ts.tlm — `Compose(local_op, accumulator)`.
            Matrix m{
                AsNumber(a[0]), AsNumber(a[1]),
                AsNumber(a[2]), AsNumber(a[3]),
                AsNumber(a[4]), AsNumber(a[5])};
            state.ctm = Compose(m, state.ctm);
        } else if (name == "w" && a.size() == 1) {
            state.line_width = std::max(AsNumber(a[0]), 0.0);
        } else if (name == "d" && a.size() == 2) {
            // §8.4.3.6 line dash pattern: `[a0 a1 …] phase d`.
            // An empty array resets to a solid line.
            state.dash_array.clear();
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&a[0].v)) {
                for (const auto& it : arr->items) {
                    state.dash_array.push_back(AsNumber(it));
                }
            }
            state.dash_phase = AsNumber(a[1]);
        } else if (name == "gs" && a.size() == 1 && resources) {
            // §8.4.5 / §11.6.4.4: apply a named ExtGState. We read
            // the constant-alpha entries /ca (non-stroking) and /CA
            // (stroking); each is applied only when present, so a
            // gs that sets neither leaves the current alpha intact.
            // Other ExtGState keys (/BM, /SMask, /LW, /D, …) are not
            // yet honoured.
            if (const auto* nm = std::get_if<std::string>(&a[0].v)) {
                if (const auto* egv = DictGet(*resources, "ExtGState")) {
                    if (const auto* egr = Resolve(dump, *egv)) {
                        if (const auto* egd = std::get_if<
                                foundation::objects::Dict>(&egr->v)) {
                            if (const auto* gv = DictGet(*egd, *nm)) {
                                if (const auto* gr =
                                        Resolve(dump, *gv)) {
                                    if (const auto* gd = std::get_if<
                                            foundation::objects::Dict>(
                                                &gr->v)) {
                                        if (DictGet(*gd, "ca")) {
                                            state.fill_alpha =
                                                std::clamp(DictNumber(
                                                    dump, *gd, "ca",
                                                    1.0), 0.0, 1.0);
                                        }
                                        if (DictGet(*gd, "CA")) {
                                            state.stroke_alpha =
                                                std::clamp(DictNumber(
                                                    dump, *gd, "CA",
                                                    1.0), 0.0, 1.0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (name == "rg" && a.size() == 3) {
            state.fill_color = foundation::colorspace::ColorRgb{
                std::clamp(AsNumber(a[0]), 0.0, 1.0),
                std::clamp(AsNumber(a[1]), 0.0, 1.0),
                std::clamp(AsNumber(a[2]), 0.0, 1.0)};
            state.fill_is_pattern = false;
            state.fill_sep_tint = nullptr;
        } else if (name == "RG" && a.size() == 3) {
            state.stroke_color = foundation::colorspace::ColorRgb{
                std::clamp(AsNumber(a[0]), 0.0, 1.0),
                std::clamp(AsNumber(a[1]), 0.0, 1.0),
                std::clamp(AsNumber(a[2]), 0.0, 1.0)};
            state.stroke_sep_tint = nullptr;
        } else if (name == "g" && a.size() == 1) {
            const double k = std::clamp(AsNumber(a[0]),
                                        0.0, 1.0);
            state.fill_color =
                foundation::colorspace::ColorRgb{k, k, k};
            state.fill_is_pattern = false;
            state.fill_sep_tint = nullptr;
        } else if (name == "G" && a.size() == 1) {
            const double k = std::clamp(AsNumber(a[0]),
                                        0.0, 1.0);
            state.stroke_color =
                foundation::colorspace::ColorRgb{k, k, k};
            state.stroke_sep_tint = nullptr;
        } else if (name == "k" && a.size() == 4) {
            state.fill_color = CmykToRgb(
                AsNumber(a[0]), AsNumber(a[1]),
                AsNumber(a[2]), AsNumber(a[3]));
            state.fill_is_pattern = false;
            state.fill_sep_tint = nullptr;
        } else if (name == "K" && a.size() == 4) {
            state.stroke_color = CmykToRgb(
                AsNumber(a[0]), AsNumber(a[1]),
                AsNumber(a[2]), AsNumber(a[3]));
            state.stroke_sep_tint = nullptr;
        } else if (name == "cs" && a.size() == 1) {
            // Non-stroking colour space. Track /Pattern and (single-input)
            // /Separation; other spaces leave the current fill colour and
            // clear any prior pattern / separation selection.
            const auto* nm = std::get_if<std::string>(&a[0].v);
            state.fill_is_pattern = (nm && *nm == "Pattern");
            state.fill_pattern.clear();
            const auto sep = nm ? ResolveSeparationCs(dump, resources, *nm)
                                : SeparationCs{};
            state.fill_sep_tint = sep.tint;
            state.fill_sep_alt = sep.alt;
        } else if (name == "CS" && a.size() == 1) {
            const auto* nm = std::get_if<std::string>(&a[0].v);
            const auto sep = nm ? ResolveSeparationCs(dump, resources, *nm)
                                : SeparationCs{};
            state.stroke_sep_tint = sep.tint;
            state.stroke_sep_alt = sep.alt;
        } else if (name == "scn" || name == "sc") {
            // /Pattern: last operand = pattern resource name. /Separation:
            // the numeric tint drives the tint transform → alternate space
            // → RGB (§8.6.6.4). Otherwise device colour by operand count
            // (1 → Gray, 3 → RGB, 4 → CMYK).
            if (state.fill_is_pattern && !a.empty()
                    && std::get_if<std::string>(&a.back().v)) {
                state.fill_pattern =
                    *std::get_if<std::string>(&a.back().v);
            } else if (state.fill_sep_tint && !a.empty()) {
                const auto comps = EvalPdfFunction(
                    dump, *state.fill_sep_tint, AsNumber(a[0]), 0);
                if (!comps.empty()) {
                    state.fill_color = AltComponentsToRgb(
                        state.fill_sep_alt, comps);
                } else if (auto rgb = NumericColor(a)) {
                    state.fill_color = *rgb;
                }
            } else if (auto rgb = NumericColor(a)) {
                state.fill_color = *rgb;
            }
        } else if (name == "SCN" || name == "SC") {
            if (state.stroke_sep_tint && !a.empty()) {
                const auto comps = EvalPdfFunction(
                    dump, *state.stroke_sep_tint, AsNumber(a[0]), 0);
                if (!comps.empty()) {
                    state.stroke_color = AltComponentsToRgb(
                        state.stroke_sep_alt, comps);
                } else if (auto rgb = NumericColor(a)) {
                    state.stroke_color = *rgb;
                }
            } else if (auto rgb = NumericColor(a)) {
                state.stroke_color = *rgb;
            }
        } else if (name == "re" && a.size() == 4) {
            const double x = AsNumber(a[0]);
            const double y = AsNumber(a[1]);
            const double w = AsNumber(a[2]);
            const double h = AsNumber(a[3]);
            current.segments.push_back(
                {SegmentKind::Move,
                 {Point{x, y}, Point{0, 0}}});
            current.segments.push_back(
                {SegmentKind::Line,
                 {Point{x + w, y}, Point{0, 0}}});
            current.segments.push_back(
                {SegmentKind::Line,
                 {Point{x + w, y + h}, Point{0, 0}}});
            current.segments.push_back(
                {SegmentKind::Line,
                 {Point{x, y + h}, Point{0, 0}}});
            current.segments.push_back(
                {SegmentKind::Close,
                 {Point{0, 0}, Point{0, 0}}});
            cursor.has_current = true;
            cursor.current = Point{x, y};
            cursor.subpath_start = Point{x, y};
        } else if (name == "m" && a.size() == 2) {
            const Point p{AsNumber(a[0]), AsNumber(a[1])};
            current.segments.push_back(
                {SegmentKind::Move, {p, Point{0, 0}}});
            cursor.has_current = true;
            cursor.current = p;
            cursor.subpath_start = p;
        } else if (name == "l" && a.size() == 2) {
            const Point p{AsNumber(a[0]), AsNumber(a[1])};
            current.segments.push_back(
                {SegmentKind::Line, {p, Point{0, 0}}});
            cursor.current = p;
        } else if (name == "c" && a.size() == 6
                   && cursor.has_current) {
            const Point p1{AsNumber(a[0]), AsNumber(a[1])};
            const Point p2{AsNumber(a[2]), AsNumber(a[3])};
            const Point p3{AsNumber(a[4]), AsNumber(a[5])};
            FlattenCubicTo(current, cursor.current, p1, p2, p3);
            cursor.current = p3;
        } else if (name == "v" && a.size() == 4
                   && cursor.has_current) {
            const Point p1 = cursor.current;
            const Point p2{AsNumber(a[0]), AsNumber(a[1])};
            const Point p3{AsNumber(a[2]), AsNumber(a[3])};
            FlattenCubicTo(current, cursor.current, p1, p2, p3);
            cursor.current = p3;
        } else if (name == "y" && a.size() == 4
                   && cursor.has_current) {
            const Point p1{AsNumber(a[0]), AsNumber(a[1])};
            const Point p3{AsNumber(a[2]), AsNumber(a[3])};
            FlattenCubicTo(current, cursor.current, p1, p3, p3);
            cursor.current = p3;
        } else if (name == "h") {
            current.segments.push_back(
                {SegmentKind::Close,
                 {Point{0, 0}, Point{0, 0}}});
            if (cursor.has_current) {
                cursor.current = cursor.subpath_start;
            }
        } else if (name == "f" || name == "F") {
            fill_current(FillRule::NonZero);
            reset_path();
        } else if (name == "f*") {
            fill_current(FillRule::EvenOdd);
            reset_path();
        } else if (name == "S") {
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "s") {
            current.segments.push_back(
                {SegmentKind::Close,
                 {Point{0, 0}, Point{0, 0}}});
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "B") {
            fill_current(FillRule::NonZero);
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "B*") {
            fill_current(FillRule::EvenOdd);
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "b") {
            current.segments.push_back(
                {SegmentKind::Close,
                 {Point{0, 0}, Point{0, 0}}});
            fill_current(FillRule::NonZero);
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "b*") {
            current.segments.push_back(
                {SegmentKind::Close,
                 {Point{0, 0}, Point{0, 0}}});
            fill_current(FillRule::EvenOdd);
            StrokePath(dst, current, state.ctm,
                       state.stroke_color, state.line_width,
                       state.dash_array, state.dash_phase,
                       state.stroke_alpha, clip_ptr());
            reset_path();
        } else if (name == "n") {
            reset_path();
        } else if (name == "W" || name == "W*") {
            // §8.5.4: mark the current path as the pending clip. It is
            // intersected into the gstate clip (as its device bbox) by
            // the NEXT path-painting operator's reset_path(). W and W*
            // differ only in winding rule; for a bbox clip that is
            // immaterial.
            pending_clip = true;
        }

        // ----- Text operators -----
        else if (name == "BT") {
            ts.tm = foundation::transform::Identity();
            ts.tlm = foundation::transform::Identity();
        } else if (name == "ET") {
            ts.font = nullptr;
        } else if (name == "Tf" && a.size() == 2) {
            const auto* fname = std::get_if<std::string>(
                &a[0].v);
            const double size = AsNumber(a[1]);
            ts.font_size = size;
            if (fname) {
                auto it = fonts.find(*fname);
                ts.font = (it != fonts.end()) ? &it->second
                                              : nullptr;
            } else {
                ts.font = nullptr;
            }
        } else if (name == "Tc" && a.size() == 1) {
            ts.char_spacing = AsNumber(a[0]);
        } else if (name == "Tw" && a.size() == 1) {
            ts.word_spacing = AsNumber(a[0]);
        } else if (name == "TL" && a.size() == 1) {
            ts.leading = AsNumber(a[0]);
        } else if (name == "Ts" && a.size() == 1) {
            ts.rise = AsNumber(a[0]);
        } else if (name == "Tz" && a.size() == 1) {
            // Tz / 100. A NEGATIVE horizontal scale is legal and is used (with
            // a negative font size) to render upright text under a y-flipping
            // CTM — clamping it to 0 collapsed the glyph x-scale and advance.
            ts.horiz_scale = AsNumber(a[0]) / 100.0;
        } else if (name == "Tm" && a.size() == 6) {
            Matrix m{
                AsNumber(a[0]), AsNumber(a[1]),
                AsNumber(a[2]), AsNumber(a[3]),
                AsNumber(a[4]), AsNumber(a[5])};
            ts.tm = m;
            ts.tlm = m;
        } else if (name == "Td" && a.size() == 2) {
            const Matrix tr =
                foundation::transform::Translate(
                    AsNumber(a[0]), AsNumber(a[1]));
            ts.tlm = Compose(tr, ts.tlm);
            ts.tm = ts.tlm;
        } else if (name == "TD" && a.size() == 2) {
            const double tx = AsNumber(a[0]);
            const double ty = AsNumber(a[1]);
            ts.leading = -ty;
            const Matrix tr =
                foundation::transform::Translate(tx, ty);
            ts.tlm = Compose(tr, ts.tlm);
            ts.tm = ts.tlm;
        } else if (name == "T*") {
            const Matrix tr =
                foundation::transform::Translate(
                    0.0, -ts.leading);
            ts.tlm = Compose(tr, ts.tlm);
            ts.tm = ts.tlm;
        } else if (name == "Tj" && a.size() == 1) {
            ShowString(dst, state.ctm, state, ts,
                       AsStringBytes(a[0]));
        } else if (name == "'") {
            // Move to next line + show.
            const Matrix tr =
                foundation::transform::Translate(
                    0.0, -ts.leading);
            ts.tlm = Compose(tr, ts.tlm);
            ts.tm = ts.tlm;
            if (a.size() == 1) {
                ShowString(dst, state.ctm, state, ts,
                           AsStringBytes(a[0]));
            }
        } else if (name == "\"") {
            // " aw ac string : set Tw, Tc, then ' string.
            if (a.size() == 3) {
                ts.word_spacing = AsNumber(a[0]);
                ts.char_spacing = AsNumber(a[1]);
                const Matrix tr =
                    foundation::transform::Translate(
                        0.0, -ts.leading);
                ts.tlm = Compose(tr, ts.tlm);
                ts.tm = ts.tlm;
                ShowString(dst, state.ctm, state, ts,
                           AsStringBytes(a[2]));
            }
        } else if (name == "TJ" && a.size() == 1) {
            HandleTJArray(dst, state.ctm, state, ts, a[0]);
        }
        // ----- XObject dispatch (Image or Form) -----
        else if (name == "Do" && a.size() == 1) {
            const auto* res_name =
                std::get_if<std::string>(&a[0].v);
            if (res_name) {
                // Pre-resolved image XObjects live in `images`;
                // anything else may be a Form XObject (executed
                // recursively in the current CTM).
                const auto it = images.find(*res_name);
                if (it != images.end()) {
                    HandleDo(dst, state.ctm, images, *res_name,
                             state.fill_alpha, clip_ptr());
                } else {
                    HandleFormXObject(dst, state.ctm, *res_name,
                                      pdf_bytes, dump, resources,
                                      depth);
                }
            }
        }
        else if (name == "INLINE_IMAGE" && a.size() == 1) {
            // §8.9.7 inline image: content_stream carries it as a Stream
            // operand (header = inline param dict, body = raw samples).
            // Expand the abbreviated keys, then decode + composite through
            // the same base-image path as /Do, at the current CTM.
            const auto* st = std::get_if<
                foundation::objects::Stream>(&a[0].v);
            if (st) {
                foundation::objects::Stream expanded;
                expanded.header =
                    ExpandInlineImageHeader(st->header, dump, resources);
                expanded.body = st->body;
                const ResolvedImage img = BuildResolvedImageBase(
                    pdf_bytes, dump, expanded);
                CompositeResolvedImage(dst, state.ctm, img,
                                       state.fill_alpha, clip_ptr());
            }
        }
        // Tr / J / j / M / d0 / d1 silently consumed —
        // see capability YAML's out_of_scope.
    }
}

// Device raster dimensions for a page of the given user-space extent, target
// DPI and /Rotate value. The extent is scaled by dpi/72 and CEILed (not
// rounded) so the raster fully covers the page: a fractional-point extent must
// not drop its final user-space row/column (e.g. an A4 page 595.276pt high at
// 150dpi is 1240.16px → 1241, not 1240). This matches the reference and the
// poppler oracle the validation tool diffs against, which both ceil. For
// /Rotate 90|270 the displayed page is landscape, so the device width takes the
// page's y-extent and the height its x-extent (axes swapped) — the unswapped
// extents emit a portrait raster and mis-scale the rotated content.
struct DeviceSize { std::uint32_t width; std::uint32_t height; };
DeviceSize PageDeviceSize(double page_w_pdf, double page_h_pdf,
                          std::int32_t rotate_degrees, double dpi) {
    double dw = page_w_pdf, dh = page_h_pdf;
    if (rotate_degrees == 90 || rotate_degrees == 270)
        std::swap(dw, dh);
    const auto px = [dpi](double extent) {
        // Multiply before dividing (extent * dpi / 72, not extent * (dpi/72))
        // so an integer-point page at an integer-pixel size stays exact, then
        // ceil with a sub-pixel epsilon to absorb the residual float noise —
        // an exact 792pt page at 150dpi is 1650px, not ceil(1650.0000000002).
        const double v = extent * dpi / 72.0;
        return static_cast<std::uint32_t>(
            std::max(1L, static_cast<long>(std::ceil(v - 1e-6))));
    };
    return DeviceSize{px(dw), px(dh)};
}

Matrix UserToDeviceCtm(double page_w_pdf,
                       double page_h_pdf,
                       std::int32_t rotate_degrees,
                       double dpi,
                       double origin_x = 0.0,
                       double origin_y = 0.0) {
    // The MediaBox origin (llx, lly) is frequently non-zero — and
    // sometimes negative (e.g. a form authored at [-90 -1008 702 216]).
    // Content is drawn relative to that origin, so the viewport must map
    // [llx, lly, urx, ury], not [0, 0, w, h]; Viewport shifts the box
    // origin to (0,0) for us. Dropping the origin pushes the whole page
    // off-canvas (renders blank).
    foundation::transform::Rect mb{origin_x, origin_y,
                                   origin_x + page_w_pdf,
                                   origin_y + page_h_pdf};
    const DeviceSize ds = PageDeviceSize(page_w_pdf, page_h_pdf,
                                         rotate_degrees, dpi);
    return foundation::transform::Viewport(
        mb, rotate_degrees, ds.width, ds.height);
}

// Render each annotation's normal appearance stream (/AP /N) on top
// of the page content (PDF §12.5.5). For each /Annots entry that is
// not Hidden/NoView, the appearance Form XObject is mapped so its
// /Matrix-transformed /BBox exactly covers the annotation /Rect, then
// executed via RenderFormStream. /N may be the appearance stream
// directly, or a sub-dictionary keyed by the appearance state /AS
// (checkbox/radio on-off). Annotations without an /AP are skipped
// (we do not synthesise appearances). /BBox clipping is not applied
// (same v1 limitation as Form XObjects).
void RenderAnnotations(Raster& dst, const Matrix& page_ctm,
                       std::span<const std::byte> pdf_bytes,
                       const Dump& dump,
                       const foundation::objects::Dict& page_dict) {
    const auto* annots_v = DictGet(page_dict, "Annots");
    if (!annots_v) return;
    const auto* ar = Resolve(dump, *annots_v);
    const auto* arr = ar ? std::get_if<
        foundation::objects::Array>(&ar->v) : nullptr;
    if (!arr) return;

    for (const auto& item : arr->items) {
        const auto* av = Resolve(dump, item);
        const auto* ad = av ? std::get_if<
            foundation::objects::Dict>(&av->v) : nullptr;
        if (!ad) continue;

        // /F flags: skip Hidden (bit 2 = 0x2) and NoView (bit 6 = 0x20).
        if (const auto* fv = DictGet(*ad, "F")) {
            const long fl = static_cast<long>(AsNumber(*fv));
            if (fl & 0x2) continue;
            if (fl & 0x20) continue;
        }

        // /Rect → normalised (rx0,ry0)-(rx1,ry1).
        const auto* rv = DictGet(*ad, "Rect");
        const auto* rr = rv ? Resolve(dump, *rv) : nullptr;
        const auto* rect = rr ? std::get_if<
            foundation::objects::Array>(&rr->v) : nullptr;
        if (!rect || rect->items.size() != 4) continue;
        double rx0 = AsNumber(rect->items[0]);
        double ry0 = AsNumber(rect->items[1]);
        double rx1 = AsNumber(rect->items[2]);
        double ry1 = AsNumber(rect->items[3]);
        if (rx1 < rx0) std::swap(rx0, rx1);
        if (ry1 < ry0) std::swap(ry0, ry1);

        // /AP /N → appearance stream (directly or via /AS sub-dict).
        const auto* ap_v = DictGet(*ad, "AP");
        const auto* ap_r = ap_v ? Resolve(dump, *ap_v) : nullptr;
        const auto* ap_d = ap_r ? std::get_if<
            foundation::objects::Dict>(&ap_r->v) : nullptr;
        if (!ap_d) continue;
        const auto* n_v = DictGet(*ap_d, "N");
        const auto* n_r = n_v ? Resolve(dump, *n_v) : nullptr;
        if (!n_r) continue;
        const foundation::objects::Stream* ap_st =
            std::get_if<foundation::objects::Stream>(&n_r->v);
        if (!ap_st) {
            // Sub-dictionary keyed by the appearance state /AS.
            const auto* nd = std::get_if<
                foundation::objects::Dict>(&n_r->v);
            if (!nd) continue;
            const auto* as_v = DictGet(*ad, "AS");
            const auto* as_n = as_v ? std::get_if<std::string>(
                &as_v->v) : nullptr;
            if (!as_n) continue;  // no state selector — skip
            const auto* sv = DictGet(*nd, *as_n);
            const auto* sr = sv ? Resolve(dump, *sv) : nullptr;
            ap_st = sr ? std::get_if<
                foundation::objects::Stream>(&sr->v) : nullptr;
            if (!ap_st) continue;
        }

        // §12.5.5: map the /Matrix-transformed appearance /BBox onto
        // /Rect via an axis-aligned scale+translate A; the form's own
        // /Matrix is re-applied inside RenderFormStream.
        Matrix base_ctm = page_ctm;
        const auto* bv = DictGet(ap_st->header, "BBox");
        const auto* br = bv ? Resolve(dump, *bv) : nullptr;
        const auto* bb = br ? std::get_if<
            foundation::objects::Array>(&br->v) : nullptr;
        if (bb && bb->items.size() == 4) {
            const double bx0 = AsNumber(bb->items[0]);
            const double by0 = AsNumber(bb->items[1]);
            const double bx1 = AsNumber(bb->items[2]);
            const double by1 = AsNumber(bb->items[3]);
            const Matrix fm = ReadMatrix(dump, ap_st->header);
            double tminx = 1e30, tminy = 1e30;
            double tmaxx = -1e30, tmaxy = -1e30;
            for (const auto& c : {
                    foundation::transform::Point{bx0, by0},
                    foundation::transform::Point{bx1, by0},
                    foundation::transform::Point{bx1, by1},
                    foundation::transform::Point{bx0, by1}}) {
                const auto p = foundation::transform::Apply(fm, c);
                tminx = std::min(tminx, p.x); tmaxx = std::max(tmaxx, p.x);
                tminy = std::min(tminy, p.y); tmaxy = std::max(tmaxy, p.y);
            }
            const double tw = tmaxx - tminx, th = tmaxy - tminy;
            const double sx = (std::abs(tw) > 1e-9)
                ? (rx1 - rx0) / tw : 1.0;
            const double sy = (std::abs(th) > 1e-9)
                ? (ry1 - ry0) / th : 1.0;
            const Matrix a{sx, 0.0, 0.0, sy,
                           rx0 - sx * tminx, ry0 - sy * tminy};
            base_ctm = Compose(a, page_ctm);
        }

        RenderFormStream(dst, base_ctm, *ap_st, pdf_bytes, dump,
                         nullptr, /*depth=*/0);
    }
}

}  // namespace

RenderedPage Render(std::span<const std::byte> input,
                    std::uint64_t page_index,
                    double target_dpi,
                    bool transparent_background) {
    if (target_dpi <= 0.0) {
        throw std::invalid_argument(
            "foundation::page_renderer::Render: target_dpi must be > 0");
    }

    foundation::pages_tree::Tree tree;
    Dump dump;
    try {
        tree = foundation::pages_tree::Parse(input);
        dump = foundation::objects::Parse(input);
    } catch (const std::exception& e) {
        throw std::runtime_error(e.what());
    }
    if (page_index >=
        tree.leaves.size()) {
        throw std::out_of_range("foundation::page_renderer::Render: "
            "page_index " + std::to_string(page_index) +
            " >= page count " +
            std::to_string(tree.leaves.size()));
    }

    const auto& leaf = tree.leaves[
        static_cast<std::size_t>(page_index)];

    const DeviceSize ds = PageDeviceSize(leaf.width, leaf.height,
                                         leaf.rotation, target_dpi);
    const auto pixel_w = ds.width;
    const auto pixel_h = ds.height;

    Raster dst;
    dst.width = pixel_w;
    dst.height = pixel_h;
    dst.pixels.assign(
        static_cast<std::size_t>(pixel_w) * pixel_h * 4u,
        static_cast<std::uint8_t>(transparent_background ? 0 : 255));

    // Recover the MediaBox origin (llx, lly) so content drawn at a
    // non-zero — or negative — origin maps onto the raster. leaf.width /
    // leaf.height already carry the box extent (pages_tree, inheritance
    // applied); the origin is read from the same MediaBox here.
    double origin_x = 0.0, origin_y = 0.0;
    if (const auto* po0 = FindObject(dump, leaf.id)) {
        if (const auto* pd0 = std::get_if<
                foundation::objects::Dict>(&po0->value.v)) {
            if (const auto* mbv = DictGet(*pd0, "MediaBox")) {
                if (const auto* mbr = Resolve(dump, *mbv)) {
                    if (const auto* arr = std::get_if<
                            foundation::objects::Array>(&mbr->v)) {
                        if (arr->items.size() >= 4) {
                            const double x0 = AsNumber(*Resolve(dump, arr->items[0]));
                            const double y0 = AsNumber(*Resolve(dump, arr->items[1]));
                            const double x1 = AsNumber(*Resolve(dump, arr->items[2]));
                            const double y1 = AsNumber(*Resolve(dump, arr->items[3]));
                            origin_x = std::min(x0, x1);
                            origin_y = std::min(y0, y1);
                        }
                    }
                }
            }
        }
    }

    const Matrix page_ctm = UserToDeviceCtm(
        leaf.width, leaf.height, leaf.rotation,
        target_dpi, origin_x, origin_y);

    const auto* page_obj = FindObject(dump, leaf.id);
    if (page_obj) {
        const auto* page_dict = std::get_if<
            foundation::objects::Dict>(&page_obj->value.v);
        if (page_dict) {
            auto bytes = CollectContents(
                input, dump, *page_dict);
            if (!bytes.empty()) {
                Stream cs;
                try {
                    cs = foundation::content_stream::Parse(
                        std::span<const std::byte>(
                            bytes.data(), bytes.size()));
                } catch (const std::exception& e) {
                    throw std::runtime_error(
                        std::string("content_stream: ") +
                        e.what());
                }
                auto fonts = ResolvePageFonts(
                    input, dump, *page_dict);
                auto images = ResolvePageImages(
                    input, dump, *page_dict);
                const auto* resources =
                    ResolveResources(dump, *page_dict);
                RenderContentImpl(dst, cs, page_ctm, input, dump,
                                  resources, fonts, images, 0);
            }
            // Annotation appearances paint on top of page content,
            // independent of whether the page had a content stream.
            RenderAnnotations(dst, page_ctm, input, dump, *page_dict);
        }
    }

    RenderedPage out;
    out.width = pixel_w;
    out.height = pixel_h;
    out.pixels = std::move(dst.pixels);
    return out;
}

}  // namespace foundation::page_renderer
