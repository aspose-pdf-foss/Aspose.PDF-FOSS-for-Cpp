// foundation/jbig2 — JBIG2 decoder per ITU-T T.88 (PDF /JBIG2Decode).
//
// Scope: segment-header parser + MQ arithmetic coder (T.82 Annex E)
// + arithmetic-coded generic region (T.88 §6.2, GBTEMPLATE 0/1/2/3)
// + symbol dictionary (§6.5) + text region (§6.4) + generic
// refinement region (§6.3) with the refine/aggregate symbol coding
// (SDREFAGG, §6.5.8.2) and per-instance text-region refinement
// (SBREFINE, §6.4.11) built on it. Huffman-coded variants, pattern
// /halftone regions, and standalone refinement-region segments are
// out of scope and throw with a precise message; see
// the project spec for the full scope notes.
//
// Algorithm sources:
//   - MQ decoder: T.82 §E.1 (INITDEC, BYTEIN, RENORMD, DECODE
//     procedures; spec form, no PDFium-style C-inversion
//     optimisation)
//   - QE / NMPS / NLPS / SWITCH tables: T.82 §E.2.4 Table E.1
//     (47-entry probability estimation tables, public domain
//     since 1993; cross-validated against the OpenJPEG and
//     PDFium copies in the wild)
//   - Segment header layout: T.88 §7.2 (BE multi-byte fields,
//     1-byte page association in PDF embedded form)
//   - Generic region: T.88 §6.2.5 (GBTEMPLATE 0/1/2/3 context
//     templates, Tables 5–8) and §6.2.5.4 (TPGD prediction)
//
// No GBMMR (generic-region MMR variant) decoder in v1 — the
// spec routes those to the CCITT G4 path, but the dependency
// adapter is non-trivial and our gate corpus uses arithmetic-
// coded generic regions only.

#include "jbig2.hpp"

#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace foundation::jbig2 {

namespace {

// --- T.82 Annex E Table E.1 + T.88 §6.2.5.3 Tables 5-8 ---
//
// MQ probability estimation tables (47 states) and per-template
// generic-region neighbourhood layouts live in jbig2_data.inc,
// generated from the project spec. Body
// never bakes in spec data inline.
//
// QeEntry is declared here so the included file can reference it.

struct QeEntry {
    std::uint16_t qe;
    std::uint8_t  nmps;
    std::uint8_t  nlps;
    std::uint8_t  swtch;  // not 'switch' — reserved keyword
};

#include "jbig2_data.inc"

// --- MQ Decoder (T.82 §E.1) ---
//
// State variables follow spec naming exactly so the algorithm
// is line-by-line auditable against T.82 §E.1.1 / §E.1.2 /
// §E.1.4 / §E.1.5.
struct MQState {
    std::uint8_t  mps;    // 0 or 1, the more probable symbol
    std::uint8_t  index;  // QE table state, 0..46
};

class MQDecoder {
public:
    void Init(std::span<const std::byte> input) {
        p_ = input.data();
        end_ = p_ + input.size();
        // T.82 §E.1.1 INITDEC:
        //   B = first byte, C = B << 16, BYTEIN, C = C << 7,
        //   CT = CT - 7, A = 0x8000.
        // BYTEIN consumes another byte and adjusts CT/C; we must
        // be careful that BYTEIN doesn't run off the end on
        // a tiny segment.
        if (p_ < end_) {
            b_ = std::to_integer<std::uint8_t>(*p_);
            ++p_;
        } else {
            b_ = 0;
        }
        c_ = static_cast<std::uint32_t>(b_) << 16;
        ByteIn();
        c_ <<= 7;
        ct_ -= 7;
        a_ = 0x8000;
    }

    // T.82 §E.1.2 DECODE(CX). Returns the decoded bit (0/1) and
    // mutates the GBSTATS entry at `state`.
    std::uint8_t Decode(MQState& state) {
        const QeEntry& qe = kQeTable[state.index];
        a_ -= qe.qe;
        std::uint32_t d;
        if ((c_ >> 16) < qe.qe) {
            // LPS path: C is in the LPS sub-interval.
            d = LpsExchange(state, qe);
            Renormalise();
        } else {
            c_ -= static_cast<std::uint32_t>(qe.qe) << 16;
            if ((a_ & 0x8000) == 0) {
                d = MpsExchange(state, qe);
                Renormalise();
            } else {
                d = state.mps;
            }
        }
        return static_cast<std::uint8_t>(d);
    }

private:
    // T.82 §E.1.3 LPS_EXCHANGE.
    std::uint32_t LpsExchange(MQState& state, const QeEntry& qe) {
        std::uint32_t d;
        if (a_ < qe.qe) {
            // The "true" LPS is now in the smaller half — output
            // MPS instead and switch states up.
            d = state.mps;
            state.index = qe.nmps;
        } else {
            d = 1u - state.mps;
            if (qe.swtch) state.mps = static_cast<std::uint8_t>(1u - state.mps);
            state.index = qe.nlps;
        }
        a_ = qe.qe;
        return d;
    }

    // T.82 §E.1.3 MPS_EXCHANGE.
    std::uint32_t MpsExchange(MQState& state, const QeEntry& qe) {
        std::uint32_t d;
        if (a_ < qe.qe) {
            d = 1u - state.mps;
            if (qe.swtch) state.mps = static_cast<std::uint8_t>(1u - state.mps);
            state.index = qe.nlps;
        } else {
            d = state.mps;
            state.index = qe.nmps;
        }
        return d;
    }

    // T.82 §E.1.4 BYTEIN — fetch the next byte, handling 0xFF
    // stuffing.
    void ByteIn() {
        // Look-ahead for FF-stuffing detection.
        if (b_ == 0xFFu) {
            // Peek next byte WITHOUT consuming.
            std::uint8_t b1 = (p_ < end_)
                ? std::to_integer<std::uint8_t>(*p_) : 0xFFu;
            if (b1 > 0x8Fu) {
                // End-of-data marker — stuff zeros into C.
                c_ += 0xFF00u;
                ct_ = 8;
            } else {
                // Stuffed FF — consume next byte.
                b_ = b1;
                if (p_ < end_) ++p_;
                c_ += static_cast<std::uint32_t>(b_) << 9;
                ct_ = 7;
            }
        } else {
            // Normal byte fetch.
            if (p_ < end_) {
                b_ = std::to_integer<std::uint8_t>(*p_);
                ++p_;
            } else {
                b_ = 0xFFu;  // pad with FF when stream exhausted
            }
            c_ += static_cast<std::uint32_t>(b_) << 8;
            ct_ = 8;
        }
    }

    // T.82 §E.1.5 RENORMD.
    void Renormalise() {
        do {
            if (ct_ == 0) ByteIn();
            a_ <<= 1;
            c_ <<= 1;
            --ct_;
        } while ((a_ & 0x8000) == 0);
    }

    std::uint32_t c_ = 0;
    std::uint32_t a_ = 0;
    std::uint32_t ct_ = 0;
    std::uint8_t  b_ = 0;
    const std::byte* p_ = nullptr;
    const std::byte* end_ = nullptr;
};


// --- Bitmap accessors ---
//
// The page bitmap is packed MSB-first per scanline at
// row_bytes = (width + 7) / 8 bytes per row. Bit polarity
// follows JBIG2: 1 = black, 0 = white. Out-of-bounds reads
// return 0 (the spec's "virtual white" convention for pixels
// outside the image, T.88 §6.2.5.3).

inline int GetPixel(const std::vector<std::byte>& bm,
                    int width, int height,
                    int x, int y) {
    if (x < 0 || x >= width) return 0;
    if (y < 0 || y >= height) return 0;
    const int row_bytes = (width + 7) / 8;
    const std::size_t idx =
        static_cast<std::size_t>(y) * row_bytes
        + static_cast<std::size_t>(x / 8);
    const auto byte = std::to_integer<std::uint8_t>(bm[idx]);
    return (byte >> (7 - (x % 8))) & 1;
}

inline void SetPixel(std::vector<std::byte>& bm,
                     int width, int /*height*/,
                     int x, int y, int bit) {
    const int row_bytes = (width + 7) / 8;
    const std::size_t idx =
        static_cast<std::size_t>(y) * row_bytes
        + static_cast<std::size_t>(x / 8);
    const auto byte = std::to_integer<std::uint8_t>(bm[idx]);
    const std::uint8_t mask = 1u << (7 - (x % 8));
    const std::uint8_t out = bit
        ? static_cast<std::uint8_t>(byte | mask)
        : static_cast<std::uint8_t>(byte & ~mask);
    bm[idx] = std::byte{out};
}


// --- Generic region context build (T.88 §6.2.5.3) ---
//
// GBTEMPLATE 0..3 neighbourhood layouts (kGT0..kGT3) and the
// per-template kTemplates table live in jbig2_data.inc, included
// above. Each kGTn slot is a {dx, dy, adaptive} record; adaptive
// = -1 means a fixed offset, adaptive in 0..3 means use the
// segment's GBAT[adaptive] pair (defaults from the template's
// default_gbat back-fill).

// Build the context word at (x, y) for the given template by
// iterating its slot list. T.88 §6.2.5.3.
std::uint32_t BuildContext(int gbtemplate,
                           const std::vector<std::byte>& bm,
                           int width, int height,
                           int x, int y,
                           const std::int8_t* gbat /* 8 bytes */) {
    if (gbtemplate < 0 || gbtemplate > 3) {
        throw std::invalid_argument(
            "jbig2: invalid GBTEMPLATE (must be 0..3)");
    }
    const GbtTemplate& tmpl = kTemplates[gbtemplate];
    std::uint32_t cx = 0;
    for (int b = 0; b < tmpl.bit_count; ++b) {
        const GbtSlot& slot = tmpl.bits[b];
        std::uint8_t pv;
        if (slot.adaptive >= 0) {
            pv = GetPixel(bm, width, height,
                          x + gbat[slot.adaptive * 2],
                          y + gbat[slot.adaptive * 2 + 1]);
        } else {
            pv = GetPixel(bm, width, height, x + slot.dx, y + slot.dy);
        }
        cx |= static_cast<std::uint32_t>(pv) << b;
    }
    return cx;
}

int ContextWidthFor(int gbtemplate) {
    if (gbtemplate < 0 || gbtemplate > 3) return 16;
    return kTemplates[gbtemplate].bit_count;
}


// --- Generic region decoder (T.88 §6.2) ---
//
// Decode `region_data` into the page bitmap at offset (rx, ry)
// using the given width, height, GBTEMPLATE, GBAT, and TPGDON
// flag. The page bitmap is mutated in place.
void DecodeGenericRegion(std::span<const std::byte> region_data,
                         int gbtemplate, bool tpgdon,
                         const std::int8_t* gbat,
                         std::vector<std::byte>& bitmap,
                         int page_width, int page_height,
                         int region_width, int region_height,
                         int rx, int ry) {
    if (region_width <= 0 || region_height <= 0) return;

    const int ctx_width = ContextWidthFor(gbtemplate);
    const std::size_t stats_size = std::size_t{1} << ctx_width;
    std::vector<MQState> stats(stats_size);
    // Default-initialised to {mps=0, index=0}.

    MQDecoder mq;
    mq.Init(region_data);

    // TPGD state — when TPGDON is set, each row may be decoded
    // by COPYING the previous row instead of running the
    // arithmetic loop. The "skip-the-row" decision is itself
    // arithmetic-decoded against a fixed context constant.
    bool ltp = false;
    MQState tpgd_state{};

    for (int y = 0; y < region_height; ++y) {
        if (tpgdon) {
            // T.88 §6.2.5.4: decode the SLTP bit at the
            // template-specific TPGD context.
            const std::uint32_t tpgd_cx = kTemplates[gbtemplate].sltp_ctx;
            // Allocate the TPGD state as a single MQState — it
            // shares no storage with the main GBSTATS array,
            // mirroring the "separate TPGD context state"
            // rule of T.88.
            std::uint8_t sltp = mq.Decode(tpgd_state);
            (void)tpgd_cx;  // SLTP context bits aren't used as
                            // an index into stats — we use a
                            // dedicated MQState above.
            if (sltp) ltp = !ltp;
        }

        if (ltp) {
            // Copy previous row — but ONLY if there is one.
            // First row with LTP set duplicates whatever the
            // initial bitmap row holds, which is zero-initialised.
            if (y > 0) {
                const int row_bytes = (page_width + 7) / 8;
                const std::size_t src_row =
                    static_cast<std::size_t>(ry + y - 1) * row_bytes;
                const std::size_t dst_row =
                    static_cast<std::size_t>(ry + y) * row_bytes;
                std::memcpy(bitmap.data() + dst_row,
                            bitmap.data() + src_row,
                            row_bytes);
            }
            continue;
        }

        // Per-pixel arithmetic decode.
        for (int x = 0; x < region_width; ++x) {
            const int abs_x = rx + x;
            const int abs_y = ry + y;
            const std::uint32_t cx =
                BuildContext(gbtemplate, bitmap,
                             page_width, page_height,
                             abs_x, abs_y, gbat);
            const std::uint8_t bit = mq.Decode(stats[cx]);
            if (bit) SetPixel(bitmap, page_width, page_height,
                              abs_x, abs_y, 1);
        }
    }
}


// --- Segment header parser (T.88 §7.2) ---
//
// Embedded JBIG2 (PDF /JBIG2Decode) uses 1-byte segment_page_
// association, no JBIG2 file header. Only segment types 38
// (immediate generic region), 39 (immediate lossless generic
// region), 48 (page info), 49 (end of page), 51 (end of file)
// are recognised in v1.

struct SegmentHeader {
    std::uint32_t segment_number;
    std::uint8_t  segment_type;
    std::uint32_t data_length;
    std::uint8_t  page_association;
    std::vector<std::uint32_t> referred_to;  // referred-to segment numbers
    std::size_t   header_bytes;  // total header size for cursor
};

std::uint32_t ReadU32BE(const std::byte* p) {
    return (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) << 24) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1])) << 16) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2])) <<  8) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[3])) <<  0);
}

SegmentHeader ParseSegmentHeader(const std::byte* p, const std::byte* end) {
    const std::byte* const start = p;
    if (end - p < 11) {
        throw std::invalid_argument(
            "jbig2: truncated segment header (need ≥ 11 bytes)");
    }
    SegmentHeader h{};
    h.segment_number = ReadU32BE(p);
    p += 4;

    const std::uint8_t flags = std::to_integer<std::uint8_t>(*p);
    h.segment_type = flags & 0x3Fu;
    const bool page_assoc_4 = (flags & 0x40u) != 0;
    ++p;

    // Referred-to segment count + retention flags (§7.2.4). For the
    // short form (R ≤ 4) this is one byte: high 3 bits = R, low
    // (R+1) bits = retention flags. R = 7 selects the long form.
    if (p >= end) throw std::invalid_argument("jbig2: truncated ref-flags");
    const std::uint8_t ref_byte = std::to_integer<std::uint8_t>(*p);
    std::uint32_t ref_count = (ref_byte >> 5) & 0x07u;
    if (ref_count == 7u) {
        if (end - p < 4) throw std::invalid_argument("jbig2: truncated long ref count");
        ref_count = ReadU32BE(p) & 0x1FFFFFFFu;
        p += 4;
        p += static_cast<std::size_t>((ref_count + 8) / 8);  // retention bytes
    } else {
        ++p;
    }
    if (ref_count > (1u << 20)) {
        throw std::invalid_argument("jbig2: implausible referred-to count");
    }

    // Referred-to segment numbers — 1/2/4 bytes each by this
    // segment's own number (§7.2.5).
    const int ref_size = (h.segment_number <= 256) ? 1
                       : (h.segment_number <= 65536) ? 2 : 4;
    if (static_cast<std::size_t>(end - p) <
        static_cast<std::size_t>(ref_size) * ref_count) {
        throw std::invalid_argument("jbig2: truncated referred-to numbers");
    }
    h.referred_to.reserve(ref_count);
    for (std::uint32_t i = 0; i < ref_count; ++i) {
        std::uint32_t r = 0;
        if (ref_size == 1) {
            r = std::to_integer<std::uint8_t>(p[0]);
        } else if (ref_size == 2) {
            r = (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) << 8)
              |  static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1]));
        } else {
            r = ReadU32BE(p);
        }
        h.referred_to.push_back(r);
        p += ref_size;
    }

    // Page association — 1 or 4 bytes (§7.2.6).
    if (end - p < (page_assoc_4 ? 4 : 1)) {
        throw std::invalid_argument(
            "jbig2: truncated segment header at page_association");
    }
    if (page_assoc_4) {
        h.page_association = static_cast<std::uint8_t>(ReadU32BE(p));
        p += 4;
    } else {
        h.page_association = std::to_integer<std::uint8_t>(*p);
        ++p;
    }

    if (end - p < 4) throw std::invalid_argument("jbig2: truncated data length");
    h.data_length = ReadU32BE(p);
    p += 4;

    if (h.data_length == 0xFFFFFFFFu) {
        throw std::invalid_argument(
            "jbig2: indefinite-length segments (data_length = "
            "0xFFFFFFFF) are stripe-mode v3 territory, not "
            "supported in v1/v2");
    }

    h.header_bytes = static_cast<std::size_t>(p - start);
    return h;
}


// --- Page information segment (T.88 §7.4.8) ---
//
// 19-byte body: [4 BE width][4 BE height][4 BE x_res]
//                [4 BE y_res][1 page_segment_flags][2 striping].

struct PageInfo {
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t  default_pixel_value;  // 0 or 1
};

PageInfo ParsePageInfo(std::span<const std::byte> body) {
    if (body.size() < 19) {
        throw std::invalid_argument(
            "jbig2: page info segment too short (need ≥ 19 bytes)");
    }
    PageInfo p{};
    p.width  = ReadU32BE(body.data() +  0);
    p.height = ReadU32BE(body.data() +  4);
    if (p.width == 0) {
        throw std::invalid_argument("jbig2: page width = 0");
    }
    if (p.height == 0xFFFFFFFFu) {
        throw std::invalid_argument(
            "jbig2: indefinite page height not supported in v1");
    }
    if (p.height == 0) {
        throw std::invalid_argument("jbig2: page height = 0");
    }
    const std::uint8_t flags = std::to_integer<std::uint8_t>(body[16]);
    p.default_pixel_value = (flags >> 2) & 0x1u;
    return p;
}


// --- Region segment data header (T.88 §6.2.4) ---
//
// 17-byte fixed header followed by a generic-region-specific
// data header (1 byte flags + GBAT bytes).

struct RegionHeader {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t x;
    std::uint32_t y;
    std::uint8_t  ext_combination_op;
};

RegionHeader ParseRegionHeader(std::span<const std::byte> body) {
    if (body.size() < 17) {
        throw std::invalid_argument(
            "jbig2: region header too short (need ≥ 17 bytes)");
    }
    RegionHeader r{};
    r.width  = ReadU32BE(body.data() +  0);
    r.height = ReadU32BE(body.data() +  4);
    r.x      = ReadU32BE(body.data() +  8);
    r.y      = ReadU32BE(body.data() + 12);
    r.ext_combination_op = std::to_integer<std::uint8_t>(body[16]);
    return r;
}


// --- Generic region segment dispatch (T.88 §7.4.6 + §6.2) ---
//
// Body layout: 17-byte region header, 1-byte GBTEMPLATE flags,
// 0 or 8 bytes GBAT (depending on GBTEMPLATE + GBMMR), then
// the arithmetic-coded data.

void DispatchGenericRegion(std::span<const std::byte> body,
                           std::vector<std::byte>& bitmap,
                           int page_width, int page_height) {
    const RegionHeader rh = ParseRegionHeader(body);
    if (body.size() < 18) {
        throw std::invalid_argument(
            "jbig2: generic region body too short (no flags byte)");
    }
    const std::uint8_t gflags = std::to_integer<std::uint8_t>(body[17]);
    const bool   mmr        = (gflags >> 0) & 0x1u;
    const int    gbtemplate = (gflags >> 1) & 0x3u;
    const bool   tpgdon     = (gflags >> 3) & 0x1u;

    if (mmr) {
        throw std::invalid_argument(
            "jbig2: GBMMR=1 generic regions (CCITT G4 inline) "
            "not supported in v1 — could delegate to "
            "foundation::ccitt but the gate corpus uses "
            "arithmetic-coded streams only");
    }

    // GBAT: 8 bytes for GBTEMPLATE 0; 2 bytes (one pair) for
    // GBTEMPLATE 1/2/3. Each pair is signed 8-bit (dx, dy).
    std::int8_t gbat[8] = {0};
    std::size_t cursor = 18;
    if (gbtemplate == 0) {
        if (body.size() < 18 + 8) {
            throw std::invalid_argument(
                "jbig2: GBTEMPLATE 0 needs 8 GBAT bytes");
        }
        for (int i = 0; i < 8; ++i) {
            gbat[i] = static_cast<std::int8_t>(
                std::to_integer<std::uint8_t>(body[cursor + i]));
        }
        cursor += 8;
    } else {
        if (body.size() < 18 + 2) {
            throw std::invalid_argument(
                "jbig2: GBTEMPLATE 1/2/3 needs 2 GBAT bytes");
        }
        for (int i = 0; i < 2; ++i) {
            gbat[i] = static_cast<std::int8_t>(
                std::to_integer<std::uint8_t>(body[cursor + i]));
        }
        cursor += 2;
    }

    // Arithmetic-coded body starts at `cursor`.
    DecodeGenericRegion(body.subspan(cursor),
                        gbtemplate, tpgdon, gbat,
                        bitmap, page_width, page_height,
                        static_cast<int>(rh.width),
                        static_cast<int>(rh.height),
                        static_cast<int>(rh.x),
                        static_cast<int>(rh.y));
}


// ===================================================================
// JBIG2 v2 — symbol dictionary (T.88 §6.5) + text region (§6.4)
// ===================================================================

// --- Arithmetic integer decoding (T.88 Annex A.2/A.3) ---
//
// Each IAx context bank sits on top of the MQ bit decoder. The
// running PREV value selects the context; the value is built as a
// sign bit, a unary-coded length class, then the magnitude bits.
class ArithIntCtx {
public:
    // Returns false on the out-of-band sentinel (sign set, magnitude 0).
    bool Decode(MQDecoder& mq, std::int32_t& out) {
        std::uint32_t prev = 1;
        auto bit = [&]() -> int {
            const int d = mq.Decode(cx_[prev]);
            if (prev < 256)
                prev = (prev << 1) | static_cast<std::uint32_t>(d);
            else
                prev = ((((prev << 1) | static_cast<std::uint32_t>(d))
                         & 511u) | 256u);
            return d;
        };
        const int sign = bit();
        int n;
        std::int32_t offset;
        if (bit() == 0)      { n = 2;  offset = 0;    }
        else if (bit() == 0) { n = 4;  offset = 4;    }
        else if (bit() == 0) { n = 6;  offset = 20;   }
        else if (bit() == 0) { n = 8;  offset = 84;   }
        else if (bit() == 0) { n = 12; offset = 340;  }
        else                 { n = 32; offset = 4436; }
        std::uint32_t value = 0;
        for (int i = 0; i < n; ++i)
            value = (value << 1) | static_cast<std::uint32_t>(bit());
        const std::int64_t v = static_cast<std::int64_t>(value) + offset;
        if (sign == 0) { out = static_cast<std::int32_t>(v); return true; }
        if (v > 0)     { out = static_cast<std::int32_t>(-v); return true; }
        return false;  // OOB
    }

private:
    MQState cx_[512]{};
};

// IAID symbol-ID decoding (T.88 §6.5.8.2.3 / Annex A.3).
class ArithIaidCtx {
public:
    explicit ArithIaidCtx(int sym_code_len)
        : sym_code_len_(sym_code_len),
          cx_(std::size_t{1} << (sym_code_len + 1)) {}
    std::int32_t Decode(MQDecoder& mq) {
        std::uint32_t prev = 1;
        for (int i = 0; i < sym_code_len_; ++i) {
            const int d = mq.Decode(cx_[prev]);
            prev = (prev << 1) | static_cast<std::uint32_t>(d);
        }
        return static_cast<std::int32_t>(
            prev - (std::uint32_t{1} << sym_code_len_));
    }

private:
    int sym_code_len_;
    std::vector<MQState> cx_;
};

// --- Bitmap with §8.2 combine operators (for symbols + regions) ---

struct Bitmap {
    int width = 0;
    int height = 0;
    std::vector<std::byte> bits;  // ((width + 7) / 8) * height, MSB-first
    int stride() const { return (width + 7) / 8; }
};

Bitmap MakeBitmap(int w, int h, int fill) {
    Bitmap b;
    b.width = w;
    b.height = h;
    b.bits.assign(static_cast<std::size_t>((w + 7) / 8) * h,
                  fill ? std::byte{0xFFu} : std::byte{0x00u});
    return b;
}

int BmGet(const Bitmap& b, int x, int y) {
    if (x < 0 || x >= b.width || y < 0 || y >= b.height) return 0;
    const int sb = b.stride();
    const auto byte = std::to_integer<std::uint8_t>(
        b.bits[static_cast<std::size_t>(y) * sb + x / 8]);
    return (byte >> (7 - (x % 8))) & 1;
}

void BmSet(Bitmap& b, int x, int y, int v) {
    const int sb = b.stride();
    const std::size_t idx = static_cast<std::size_t>(y) * sb + x / 8;
    const auto byte = std::to_integer<std::uint8_t>(b.bits[idx]);
    const std::uint8_t mask = 1u << (7 - (x % 8));
    b.bits[idx] = std::byte{static_cast<std::uint8_t>(
        v ? (byte | mask) : (byte & ~mask))};
}

// Combine src into dst at (dx, dy). op: 0=OR 1=AND 2=XOR 3=XNOR (§8.2).
void CombineBitmap(Bitmap& dst, const Bitmap& src, int dx, int dy, int op) {
    for (int y = 0; y < src.height; ++y) {
        const int ty = dy + y;
        if (ty < 0 || ty >= dst.height) continue;
        for (int x = 0; x < src.width; ++x) {
            const int tx = dx + x;
            if (tx < 0 || tx >= dst.width) continue;
            const int s = BmGet(src, x, y);
            const int d = BmGet(dst, tx, ty);
            int r;
            switch (op & 3) {
                case 0:  r = d | s; break;
                case 1:  r = d & s; break;
                case 2:  r = d ^ s; break;
                default: r = (d ^ s) ? 0 : 1; break;  // XNOR
            }
            BmSet(dst, tx, ty, r);
        }
    }
}

// Decode one standalone generic bitmap reusing an EXISTING mq + stats
// (symbol dictionaries share the GB context bank across symbols).
Bitmap DecodeGenericBitmap(MQDecoder& mq, std::vector<MQState>& stats,
                           int gbtemplate, const std::int8_t* gbat,
                           int w, int h) {
    Bitmap bm = MakeBitmap(w, h, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::uint32_t cx =
                BuildContext(gbtemplate, bm.bits, w, h, x, y, gbat);
            if (mq.Decode(stats[cx])) SetPixel(bm.bits, w, h, x, y, 1);
        }
    }
    return bm;
}

// --- Generic refinement region (T.88 §6.3) ---
//
// Refines a `reference` bitmap (offset by (dx, dy)) into a w×h
// output, reusing an existing MQ decoder and a refinement context
// bank `gr` (shared across the refined symbols of one dictionary or
// text region). GRTEMPLATE 0 builds a 13-bit context (two adaptive
// pixels: GRAT[0] in the output plane, GRAT[1] in the reference);
// GRTEMPLATE 1 builds a fixed 10-bit context (no adaptive pixels).
// The neighbourhood layouts are T.88 §6.3.5.3.

std::uint32_t RefinementContext0(const Bitmap& img, const Bitmap& ref,
                                 int dx, int dy, const std::int8_t* grat,
                                 int x, int y) {
    const int rx = x - dx, ry = y - dy;
    std::uint32_t c = 0;
    c |= static_cast<std::uint32_t>(BmGet(img, x - 1, y))     << 0;
    c |= static_cast<std::uint32_t>(BmGet(img, x + 1, y - 1)) << 1;
    c |= static_cast<std::uint32_t>(BmGet(img, x,     y - 1)) << 2;
    c |= static_cast<std::uint32_t>(BmGet(img, x + grat[0], y + grat[1])) << 3;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + 1, ry + 1)) << 4;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry + 1)) << 5;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx - 1, ry + 1)) << 6;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + 1, ry))     << 7;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry))     << 8;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx - 1, ry))     << 9;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + 1, ry - 1)) << 10;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry - 1)) << 11;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + grat[2], ry + grat[3])) << 12;
    return c;
}

std::uint32_t RefinementContext1(const Bitmap& img, const Bitmap& ref,
                                 int dx, int dy, int x, int y) {
    const int rx = x - dx, ry = y - dy;
    std::uint32_t c = 0;
    c |= static_cast<std::uint32_t>(BmGet(img, x - 1, y))     << 0;
    c |= static_cast<std::uint32_t>(BmGet(img, x + 1, y - 1)) << 1;
    c |= static_cast<std::uint32_t>(BmGet(img, x,     y - 1)) << 2;
    c |= static_cast<std::uint32_t>(BmGet(img, x - 1, y - 1)) << 3;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + 1, ry + 1)) << 4;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry + 1)) << 5;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx + 1, ry))     << 6;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry))     << 7;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx - 1, ry))     << 8;
    c |= static_cast<std::uint32_t>(BmGet(ref, rx,     ry - 1)) << 9;
    return c;
}

// TPGRON typical-prediction (§6.3.5.6): when the 3×3 reference
// neighbourhood around the mapped pixel is uniform, that value is
// implicit; otherwise the pixel is arithmetic-decoded.
int RefinementImplicit(const Bitmap& ref, int dx, int dy, int x, int y) {
    const int i = x - dx, j = y - dy;
    const int m = BmGet(ref, i, j);
    if (BmGet(ref, i - 1, j - 1) == m && BmGet(ref, i, j - 1) == m &&
        BmGet(ref, i + 1, j - 1) == m && BmGet(ref, i - 1, j) == m &&
        BmGet(ref, i + 1, j) == m && BmGet(ref, i - 1, j + 1) == m &&
        BmGet(ref, i, j + 1) == m && BmGet(ref, i + 1, j + 1) == m)
        return m;
    return -1;
}

Bitmap DecodeRefinementBitmap(MQDecoder& mq, std::vector<MQState>& gr,
                              int grtemplate, bool tpgron,
                              const std::int8_t* grat,
                              const Bitmap& ref, int dx, int dy,
                              int w, int h) {
    Bitmap img = MakeBitmap(w, h, 0);
    const bool t0 = (grtemplate == 0);
    // SLTP probe context (§6.3.5.6), distinct per template.
    const std::uint32_t sltp = t0 ? 0x0100u : 0x0040u;
    int ltp = 0;
    for (int y = 0; y < h; ++y) {
        if (tpgron) ltp ^= mq.Decode(gr[sltp]);
        for (int x = 0; x < w; ++x) {
            if (tpgron && ltp) {
                const int iv = RefinementImplicit(ref, dx, dy, x, y);
                if (iv >= 0) { BmSet(img, x, y, iv); continue; }
            }
            const std::uint32_t c = t0
                ? RefinementContext0(img, ref, dx, dy, grat, x, y)
                : RefinementContext1(img, ref, dx, dy, x, y);
            BmSet(img, x, y, mq.Decode(gr[c]));
        }
    }
    return img;
}

// Refinement context bank size: GRTEMPLATE 0 needs 2^13 entries.
constexpr std::size_t kRefinementContextSize = std::size_t{1} << 13;

// --- Text region (T.88 §6.4), arithmetic ---

struct TextRegionParams {
    int width = 0, height = 0;
    int def_pixel = 0;
    int ref_corner = 1;    // 0=BL 1=TL 2=BR 3=TR
    bool transposed = false;
    int sb_strips = 1;
    int sb_comb_op = 0;
    int sbds_offset = 0;
    std::uint32_t num_instances = 0;
    bool sb_refine = false;        // SBREFINE — per-instance refinement
    int sbr_template = 0;          // SBRTEMPLATE
    std::int8_t sbrat[4] = {0};    // SBRAT (2 AT pairs, template 0)
};

// Arithmetic-integer context banks for a text region (§6.4). Kept in
// one struct so the symbol-dictionary aggregate path (§6.5.8.2.2) can
// share a persistent context set across every aggregate symbol.
struct TextRegionCtx {
    ArithIntCtx IADT, IAFS, IADS, IAIT, IARI, IARDW, IARDH, IARDX, IARDY;
    ArithIaidCtx IAID;
    explicit TextRegionCtx(int sym_code_len) : IAID(sym_code_len) {}
};

Bitmap DecodeTextRegionCore(MQDecoder& mq, TextRegionCtx& cx,
                            std::vector<MQState>& gr,
                            const TextRegionParams& p,
                            const std::vector<Bitmap>& symbols);

// --- Symbol dictionary (T.88 §6.5), arithmetic + refine/aggregate ---

struct SymbolDictParams {
    int sd_template = 0;
    std::int8_t sdat[8] = {0};
    std::uint32_t num_ex_syms = 0;
    std::uint32_t num_new_syms = 0;
    bool sd_refagg = false;        // SDREFAGG — refine/aggregate coding
    int sdr_template = 0;          // SDRTEMPLATE
    std::int8_t sdrat[4] = {0};    // SDRAT (2 AT pairs, template 0)
};

std::vector<Bitmap> DecodeSymbolDict(
        std::span<const std::byte> data, const SymbolDictParams& p,
        const std::vector<Bitmap>& input_symbols) {
    MQDecoder mq;
    mq.Init(data);
    ArithIntCtx IADH, IADW, IAEX, IAAI;

    // Generic-coded symbols share the GB bank; refine/aggregate symbols
    // use the GR bank plus a persistent text-region context set (the
    // aggregate case decodes each symbol as a text region on this MQ
    // stream, §6.5.8.2.2).
    const int ctx_width = ContextWidthFor(p.sd_template);
    std::vector<MQState> gb, gr;
    if (p.sd_refagg) gr.assign(kRefinementContextSize, MQState{});
    else             gb.assign(std::size_t{1} << ctx_width, MQState{});

    int sym_code_len = 0;
    while ((1u << sym_code_len) <
           (static_cast<std::uint32_t>(input_symbols.size()) + p.num_new_syms))
        ++sym_code_len;
    TextRegionCtx tcx(sym_code_len);

    std::vector<Bitmap> all = input_symbols;
    all.reserve(input_symbols.size() + p.num_new_syms);

    std::uint32_t num_new = 0;
    std::int32_t HCHEIGHT = 0;
    while (num_new < p.num_new_syms) {
        std::int32_t HCDH = 0;
        IADH.Decode(mq, HCDH);
        HCHEIGHT += HCDH;
        if (HCHEIGHT <= 0 || HCHEIGHT > (1 << 20))
            throw std::invalid_argument("jbig2: invalid symbol height class");
        std::int32_t SYMWIDTH = 0;
        for (;;) {
            std::int32_t DW = 0;
            if (!IADW.Decode(mq, DW)) break;  // OOB → end of height class
            SYMWIDTH += DW;
            if (num_new >= p.num_new_syms) break;
            if (SYMWIDTH <= 0 || SYMWIDTH > (1 << 20))
                throw std::invalid_argument("jbig2: invalid symbol width");
            if (!p.sd_refagg) {
                all.push_back(DecodeGenericBitmap(mq, gb, p.sd_template,
                                                  p.sdat, SYMWIDTH, HCHEIGHT));
            } else {
                std::int32_t refagg_ninst = 0;
                IAAI.Decode(mq, refagg_ninst);
                if (refagg_ninst == 1) {
                    // §6.5.8.2.2 — refine a single existing symbol.
                    const std::int32_t id = tcx.IAID.Decode(mq);
                    std::int32_t rdx = 0, rdy = 0;
                    tcx.IARDX.Decode(mq, rdx);
                    tcx.IARDY.Decode(mq, rdy);
                    if (id < 0 || static_cast<std::size_t>(id) >= all.size())
                        throw std::invalid_argument(
                            "jbig2: refinement references unknown symbol");
                    all.push_back(DecodeRefinementBitmap(
                        mq, gr, p.sdr_template, false, p.sdrat,
                        all[id], rdx, rdy, SYMWIDTH, HCHEIGHT));
                } else {
                    // §6.5.8.2 — aggregate: decode the symbol as a text
                    // region placing REFAGGNINST existing-symbol instances.
                    TextRegionParams tp;
                    tp.width = SYMWIDTH;
                    tp.height = HCHEIGHT;
                    tp.ref_corner = 1;       // TOPLEFT
                    tp.sb_strips = 1;
                    tp.num_instances = static_cast<std::uint32_t>(refagg_ninst);
                    tp.sb_refine = true;
                    tp.sbr_template = p.sdr_template;
                    tp.sbrat[0] = p.sdrat[0];
                    tp.sbrat[1] = p.sdrat[1];
                    tp.sbrat[2] = p.sdrat[2];
                    tp.sbrat[3] = p.sdrat[3];
                    all.push_back(DecodeTextRegionCore(mq, tcx, gr, tp, all));
                }
            }
            ++num_new;
        }
    }

    // Export flags (§6.5.10): alternating not-exported / exported runs.
    std::vector<Bitmap> exported;
    exported.reserve(p.num_ex_syms);
    std::uint32_t i = 0, empty_runs = 0;
    int ex_flag = 0;
    const std::uint32_t limit = static_cast<std::uint32_t>(all.size());
    while (i < limit) {
        std::int32_t run = 0;
        if (!IAEX.Decode(mq, run))
            throw std::invalid_argument("jbig2: OOB decoding symbol export run");
        if (run < 0)
            throw std::invalid_argument("jbig2: negative symbol export run");
        if (run == 0) {
            if (++empty_runs > 1000)
                throw std::invalid_argument("jbig2: too many empty export runs");
        } else {
            empty_runs = 0;
        }
        if (static_cast<std::uint32_t>(run) > limit - i)
            run = static_cast<std::int32_t>(limit - i);
        if (ex_flag)
            for (std::int32_t k = 0; k < run; ++k) exported.push_back(all[i + k]);
        i += static_cast<std::uint32_t>(run);
        ex_flag ^= 1;
    }
    return exported;
}

// Place one symbol at the current strip position, handling the
// reference-corner / transpose geometry (§6.4.5 steps 3c.vi–3c.x).
void PlaceInstance(Bitmap& region, const Bitmap* IB, std::int32_t T,
                   std::int32_t& CURS, const TextRegionParams& p) {
    const int WI = IB ? IB->width : 0;
    const int HI = IB ? IB->height : 0;
    if (!p.transposed && p.ref_corner > 1 && IB)        CURS += WI - 1;
    else if (p.transposed && !(p.ref_corner & 1) && IB) CURS += HI - 1;

    const std::int32_t S = CURS;
    int x = 0, y = 0;
    if (!p.transposed) {
        switch (p.ref_corner) {
            case 1: x = S;                       y = T;                       break;
            case 3: x = IB ? S - WI + 1 : S + 1; y = T;                       break;
            case 0: x = S;                       y = IB ? T - HI + 1 : T + 1; break;
            default:x = IB ? S - WI + 1 : S + 1; y = IB ? T - HI + 1 : T + 1; break;
        }
    } else {
        switch (p.ref_corner) {
            case 1: x = T;                       y = S;                       break;
            case 3: x = IB ? T - WI + 1 : T + 1; y = S;                       break;
            case 0: x = T;                       y = IB ? S - HI + 1 : S + 1; break;
            default:x = IB ? T - WI + 1 : T + 1; y = IB ? S - HI + 1 : S + 1; break;
        }
    }
    if (IB) CombineBitmap(region, *IB, x, y, p.sb_comb_op);

    if (IB && !p.transposed && p.ref_corner < 2)      CURS += WI - 1;
    else if (IB && p.transposed && (p.ref_corner & 1)) CURS += HI - 1;
}

// Core text-region decode on a caller-supplied MQ decoder + context
// set (§6.4.5). The symbol-dictionary aggregate path drives this on its
// own MQ stream with a persistent context set; the standalone wrapper
// below creates fresh state per region.
Bitmap DecodeTextRegionCore(MQDecoder& mq, TextRegionCtx& cx,
                            std::vector<MQState>& gr,
                            const TextRegionParams& p,
                            const std::vector<Bitmap>& symbols) {
    const std::uint32_t num_syms = static_cast<std::uint32_t>(symbols.size());
    Bitmap region = MakeBitmap(p.width, p.height, p.def_pixel);

    std::int32_t dt = 0;
    cx.IADT.Decode(mq, dt);
    std::int32_t STRIPT = -dt * p.sb_strips;
    std::int32_t FIRSTS = 0;
    std::uint32_t ninst = 0;
    while (ninst < p.num_instances) {
        cx.IADT.Decode(mq, dt);
        STRIPT += dt * p.sb_strips;
        bool first = true;
        std::int32_t CURS = 0;
        for (;;) {
            if (first) {
                std::int32_t dfs = 0;
                cx.IAFS.Decode(mq, dfs);
                FIRSTS += dfs;
                CURS = FIRSTS;
                first = false;
            } else {
                if (ninst > p.num_instances) break;
                std::int32_t ids = 0;
                if (!cx.IADS.Decode(mq, ids)) break;  // OOB → end of strip
                CURS += ids + p.sbds_offset;
            }
            std::int32_t CURT = 0;
            if (p.sb_strips != 1) cx.IAIT.Decode(mq, CURT);
            const std::int32_t T = STRIPT + CURT;
            const std::int32_t ID = cx.IAID.Decode(mq);
            const Bitmap* IB = (ID >= 0 && static_cast<std::uint32_t>(ID) < num_syms)
                ? &symbols[ID] : nullptr;

            // §6.4.11 — optional per-instance refinement. The IARI bit is
            // decoded whenever SBREFINE is set (consuming it keeps the MQ
            // stream in sync even when no instance is actually refined).
            Bitmap refined;
            if (p.sb_refine) {
                std::int32_t RI = 0;
                cx.IARI.Decode(mq, RI);
                if (RI && IB) {
                    std::int32_t RDW = 0, RDH = 0, RDX = 0, RDY = 0;
                    cx.IARDW.Decode(mq, RDW);
                    cx.IARDH.Decode(mq, RDH);
                    cx.IARDX.Decode(mq, RDX);
                    cx.IARDY.Decode(mq, RDY);
                    const int rw = IB->width + RDW;
                    const int rh = IB->height + RDH;
                    if (rw > 0 && rh > 0) {
                        const int rdx = (RDW >> 1) + RDX;
                        const int rdy = (RDH >> 1) + RDY;
                        refined = DecodeRefinementBitmap(
                            mq, gr, p.sbr_template, false, p.sbrat,
                            *IB, rdx, rdy, rw, rh);
                        IB = &refined;
                    }
                }
            }

            PlaceInstance(region, IB, T, CURS, p);
            ++ninst;
        }
    }
    return region;
}

Bitmap DecodeTextRegion(std::span<const std::byte> data,
                        const TextRegionParams& p,
                        const std::vector<Bitmap>& symbols) {
    MQDecoder mq;
    mq.Init(data);
    int sym_code_len = 0;
    while ((1u << sym_code_len) < symbols.size()) ++sym_code_len;  // Table 31
    TextRegionCtx cx(sym_code_len);
    std::vector<MQState> gr;
    if (p.sb_refine) gr.assign(kRefinementContextSize, MQState{});
    return DecodeTextRegionCore(mq, cx, gr, p, symbols);
}

// --- v2 segment dispatch ---

using SymbolRegistry = std::map<std::uint32_t, std::vector<Bitmap>>;

std::vector<Bitmap> GatherInputSymbols(
        const std::vector<std::uint32_t>& referred_to,
        const SymbolRegistry& dicts) {
    std::vector<Bitmap> syms;
    for (std::uint32_t r : referred_to) {
        auto it = dicts.find(r);
        if (it != dicts.end())
            syms.insert(syms.end(), it->second.begin(), it->second.end());
    }
    return syms;
}

std::vector<Bitmap> DispatchSymbolDict(std::span<const std::byte> body,
                                       const SegmentHeader& sh,
                                       const SymbolRegistry& dicts) {
    if (body.size() < 2)
        throw std::invalid_argument("jbig2: short symbol dictionary segment");
    const std::uint16_t flags =
        (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(body[0])) << 8)
        | std::to_integer<std::uint8_t>(body[1]);
    if (flags & 1u)
        throw std::invalid_argument(
            "jbig2: Huffman-coded symbol dictionary deferred to v3");
    SymbolDictParams p;
    p.sd_refagg   = (flags >> 1) & 1u;
    p.sd_template = (flags >> 10) & 3u;
    p.sdr_template = (flags >> 12) & 1u;
    std::size_t off = 2;
    // SDAT — generic-template AT pixels (absent when SDREFAGG is set
    // and only refinement coding is used; T.88 §7.4.3.1.2 still emits
    // them unless SDHUFF, so read per SDTEMPLATE).
    const int sdat_bytes = (p.sd_template == 0) ? 8 : 2;
    for (int i = 0; i < sdat_bytes; ++i) {
        if (off >= body.size())
            throw std::invalid_argument("jbig2: truncated SDAT pixels");
        p.sdat[i] = static_cast<std::int8_t>(
            std::to_integer<std::uint8_t>(body[off]));
        ++off;
    }
    // SDRAT — refinement AT pixels, present only for refine/aggregate
    // coding with the refinement template 0 (T.88 §7.4.3.1.3).
    if (p.sd_refagg && p.sdr_template == 0) {
        for (int i = 0; i < 4; ++i) {
            if (off >= body.size())
                throw std::invalid_argument("jbig2: truncated SDRAT pixels");
            p.sdrat[i] = static_cast<std::int8_t>(
                std::to_integer<std::uint8_t>(body[off]));
            ++off;
        }
    }
    if (off + 8 > body.size())
        throw std::invalid_argument("jbig2: truncated symbol dict counts");
    p.num_ex_syms  = ReadU32BE(body.data() + off);
    p.num_new_syms = ReadU32BE(body.data() + off + 4);
    off += 8;
    const auto inputs = GatherInputSymbols(sh.referred_to, dicts);
    return DecodeSymbolDict(body.subspan(off), p, inputs);
}

// Decode a text region and composite it onto the page bitmap.
void DispatchTextRegion(std::span<const std::byte> body,
                        const SegmentHeader& sh, const SymbolRegistry& dicts,
                        Bitmap& page) {
    const RegionHeader rh = ParseRegionHeader(body);
    if (body.size() < 19)
        throw std::invalid_argument("jbig2: short text region segment");
    const std::uint16_t flags =
        (static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(body[17])) << 8)
        | std::to_integer<std::uint8_t>(body[18]);
    if (flags & 1u)
        throw std::invalid_argument(
            "jbig2: Huffman-coded text region deferred to v3");
    TextRegionParams p;
    p.width       = static_cast<int>(rh.width);
    p.height      = static_cast<int>(rh.height);
    p.ref_corner  = (flags >> 4) & 3u;
    p.transposed  = (flags >> 6) & 1u;
    p.sb_comb_op  = (flags >> 7) & 3u;
    p.def_pixel   = (flags >> 9) & 1u;
    int sbds_offset = (flags >> 10) & 0x1Fu;
    if (sbds_offset > 15) sbds_offset -= 32;  // signed 5-bit
    p.sbds_offset = sbds_offset;
    p.sb_strips   = 1 << ((flags >> 2) & 3u);
    p.sb_refine   = (flags >> 1) & 1u;
    p.sbr_template = (flags >> 15) & 1u;

    std::size_t off = 19;
    // SBRAT — refinement AT pixels, present only when SBREFINE is set
    // with the refinement template 0 (T.88 §7.4.4.1.2); they precede
    // SBNUMINSTANCES, so omitting them mis-reads the instance count.
    if (p.sb_refine && p.sbr_template == 0) {
        for (int i = 0; i < 4; ++i) {
            if (off >= body.size())
                throw std::invalid_argument("jbig2: truncated SBRAT pixels");
            p.sbrat[i] = static_cast<std::int8_t>(
                std::to_integer<std::uint8_t>(body[off]));
            ++off;
        }
    }
    if (off + 4 > body.size())
        throw std::invalid_argument("jbig2: truncated text region instance count");
    p.num_instances = ReadU32BE(body.data() + off);
    off += 4;

    const auto symbols = GatherInputSymbols(sh.referred_to, dicts);
    const Bitmap region = DecodeTextRegion(body.subspan(off), p, symbols);
    CombineBitmap(page, region, static_cast<int>(rh.x),
                  static_cast<int>(rh.y), rh.ext_combination_op & 3);
}

std::string SegTypeName(std::uint8_t t) {
    switch (t) {
        case  0: return "symbol dictionary (v2)";
        case  4: case 6: case 7: return "text region (v2)";
        case 16: return "pattern dictionary (v3)";
        case 20: case 22: case 23: return "halftone region (v3)";
        case 36: return "intermediate generic region";
        case 40: case 42: case 43: return "refinement region (v3)";
        case 53: return "tables segment";
        default: return "unknown";
    }
}

}  // namespace


namespace {

// Walk one segment stream (globals or page), updating the shared
// state. Symbol dictionaries register their exported symbols by
// segment number so later text regions (in either stream) resolve
// their referred-to dictionaries.
void ProcessSegments(std::span<const std::byte> stream,
                     SymbolRegistry& dicts, PageInfo& pi,
                     bool& seen_page_info, Bitmap& page) {
    const std::byte* p = stream.data();
    const std::byte* end = p + stream.size();
    while (p < end) {
        const SegmentHeader sh = ParseSegmentHeader(p, end);
        p += sh.header_bytes;
        if (static_cast<std::size_t>(end - p) < sh.data_length) {
            throw std::invalid_argument(
                "jbig2: segment data truncated (announced "
                "data_length exceeds remaining bytes)");
        }
        std::span<const std::byte> body{p, sh.data_length};

        switch (sh.segment_type) {
            case 48: {  // page information
                if (seen_page_info)
                    throw std::invalid_argument(
                        "jbig2: multiple page-info segments not supported");
                pi = ParsePageInfo(body);
                page = MakeBitmap(static_cast<int>(pi.width),
                                  static_cast<int>(pi.height),
                                  pi.default_pixel_value);
                seen_page_info = true;
                break;
            }
            case 0: {  // symbol dictionary
                dicts[sh.segment_number] =
                    DispatchSymbolDict(body, sh, dicts);
                break;
            }
            case 4:    // intermediate text region
            case 6:    // immediate text region
            case 7: {  // immediate lossless text region
                if (!seen_page_info)
                    throw std::invalid_argument(
                        "jbig2: text region before page-info segment");
                DispatchTextRegion(body, sh, dicts, page);
                break;
            }
            case 38:    // immediate generic region
            case 39: {  // immediate lossless generic region
                if (!seen_page_info)
                    throw std::invalid_argument(
                        "jbig2: generic region before page-info segment");
                DispatchGenericRegion(body, page.bits, page.width, page.height);
                break;
            }
            case 49:    // end of page
            case 50:    // end of stripe
            case 51:    // end of file
                break;  // accept gracefully

            default:
                throw std::invalid_argument(
                    "jbig2: segment type " +
                    std::to_string(sh.segment_type) + " (" +
                    SegTypeName(sh.segment_type) +
                    ") not supported; see "
                    "the project spec v2/v3 roadmap");
        }
        p += sh.data_length;
    }
}

}  // namespace

DecodedImage Decode(std::span<const std::byte> page,
                    std::span<const std::byte> globals) {
    if (page.empty()) {
        throw std::invalid_argument("jbig2: empty input (no segments)");
    }

    SymbolRegistry dicts;
    PageInfo pi{};
    bool seen_page_info = false;
    Bitmap page_bm;

    // Globals (symbol dictionaries + tables) are processed first so
    // page text regions can reference them.
    if (!globals.empty())
        ProcessSegments(globals, dicts, pi, seen_page_info, page_bm);
    ProcessSegments(page, dicts, pi, seen_page_info, page_bm);

    if (!seen_page_info) {
        throw std::invalid_argument(
            "jbig2: stream contains no page-info segment");
    }

    DecodedImage out{};
    out.width = pi.width;
    out.height = pi.height;
    out.components = 1;
    out.pixels = std::move(page_bm.bits);
    return out;
}

}  // namespace foundation::jbig2
