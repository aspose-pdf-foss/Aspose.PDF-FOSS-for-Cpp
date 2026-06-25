#include "rasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace foundation::rasterizer {

namespace {

// Internal edge representation: a single line segment in device
// coordinates, with sign tracking the up/down direction needed
// for winding-number computation.
struct Edge {
    double x0, y0, x1, y1;
};

// Flatten a Path into a list of edges, applying the CTM at
// flatten time so every subsequent step works in device pixel
// coordinates. Move starts a subpath; Close emits an edge back
// to the most recent Move's endpoint; Rect emits four implicit
// edges in CCW order (per PDF 32000 §8.5.2.6).
//
// Horizontal edges (y0 == y1) contribute zero to the winding
// number and are dropped — keeping them in the list would only
// waste later iteration cycles.
std::vector<Edge> Flatten(const Path& path,
                          const foundation::transform::Matrix& ctm) {
    std::vector<Edge> edges;
    edges.reserve(path.segments.size() * 2);

    foundation::transform::Point cur{0.0, 0.0};
    foundation::transform::Point sub_start{0.0, 0.0};
    bool have_cur = false;

    auto apply = [&](const foundation::transform::Point& p) {
        return foundation::transform::Apply(ctm, p);
    };

    auto add_edge = [&](double x0, double y0,
                        double x1, double y1) {
        if (y0 == y1) return;  // horizontal: zero winding contribution
        edges.push_back(Edge{x0, y0, x1, y1});
    };

    for (const auto& seg : path.segments) {
        switch (seg.kind) {
            case SegmentKind::Move: {
                const auto p = apply(seg.pts[0]);
                cur = p;
                sub_start = p;
                have_cur = true;
                break;
            }
            case SegmentKind::Line: {
                if (!have_cur) break;
                const auto p = apply(seg.pts[0]);
                add_edge(cur.x, cur.y, p.x, p.y);
                cur = p;
                break;
            }
            case SegmentKind::Rect: {
                const foundation::transform::Point p0 =
                    apply(foundation::transform::Point{
                        seg.pts[0].x, seg.pts[0].y});
                const foundation::transform::Point p1 =
                    apply(foundation::transform::Point{
                        seg.pts[1].x, seg.pts[0].y});
                const foundation::transform::Point p2 =
                    apply(foundation::transform::Point{
                        seg.pts[1].x, seg.pts[1].y});
                const foundation::transform::Point p3 =
                    apply(foundation::transform::Point{
                        seg.pts[0].x, seg.pts[1].y});
                add_edge(p0.x, p0.y, p1.x, p1.y);
                add_edge(p1.x, p1.y, p2.x, p2.y);
                add_edge(p2.x, p2.y, p3.x, p3.y);
                add_edge(p3.x, p3.y, p0.x, p0.y);
                cur = p0;
                sub_start = p0;
                have_cur = true;
                break;
            }
            case SegmentKind::Close: {
                if (have_cur && (cur.x != sub_start.x ||
                                 cur.y != sub_start.y)) {
                    add_edge(cur.x, cur.y,
                             sub_start.x, sub_start.y);
                }
                cur = sub_start;
                break;
            }
        }
    }
    return edges;
}

// Winding-number contribution of one edge at sub-pixel point
// (px, py), using the "left-inclusive, right-exclusive"
// convention (strict `>` on the crossing x) so a sample lying
// exactly on the right edge of a pixel does NOT count and two
// abutting polygons sharing an edge produce no double-count.
// Returns +1 for an upward crossing, -1 for a downward one, 0
// otherwise. Horizontal edges (y0 == y1) were filtered at
// flatten time and contribute nothing.
inline int EdgeWinding(const Edge& e, double px, double py) {
    if (e.y0 < e.y1) {
        // Upward edge.
        if (e.y0 <= py && py < e.y1) {
            const double t = (py - e.y0) / (e.y1 - e.y0);
            const double x_cross = e.x0 + t * (e.x1 - e.x0);
            if (px > x_cross) return 1;
        }
    } else if (e.y1 < e.y0) {
        // Downward edge.
        if (e.y1 <= py && py < e.y0) {
            const double t = (py - e.y0) / (e.y1 - e.y0);
            const double x_cross = e.x0 + t * (e.x1 - e.x0);
            if (px > x_cross) return -1;
        }
    }
    return 0;
}

// Apply the fill rule to a winding number. NonZero = "any
// non-zero winding is inside"; EvenOdd = "odd winding is
// inside" (regardless of sign).
inline bool Inside(int winding, FillRule rule) {
    if (rule == FillRule::NonZero) return winding != 0;
    // C++ % preserves sign on negative operands; modulus 2 of
    // a non-zero winding is ±1, so test against zero.
    return (winding % 2) != 0;
}

}  // namespace

void Fill(Raster& dst, const Path& path,
          const foundation::transform::Matrix& ctm,
          const foundation::colorspace::ColorRgb& color, double alpha,
          FillRule rule, const ClipRect* clip) {
    if (dst.width == 0 || dst.height == 0) return;
    if (dst.pixels.size() !=
        static_cast<std::size_t>(dst.width) * dst.height * 4) {
        return;  // ill-formed raster; caller error, no-op for safety
    }

    const auto edges = Flatten(path, ctm);
    if (edges.empty()) return;  // nothing to draw

    // Device-space bounding box of the flattened edges. A filled
    // path is implicitly closed, so every sub-sample outside this
    // box has winding number zero (the +x ray either crosses no
    // edge or crosses a balanced closed contour) and contributes
    // no coverage. Restricting the scan to the box is therefore
    // output-preserving and turns per-glyph fill cost from
    // O(page-pixels) into O(glyph-pixels) — see rasterizer.yaml
    // "Performance notes". One pixel of padding on each side keeps
    // every boundary sub-sample inside the iterated range.
    double min_x = edges[0].x0, max_x = edges[0].x0;
    double min_y = edges[0].y0, max_y = edges[0].y0;
    for (const auto& e : edges) {
        min_x = std::min({min_x, e.x0, e.x1});
        max_x = std::max({max_x, e.x0, e.x1});
        min_y = std::min({min_y, e.y0, e.y1});
        max_y = std::max({max_y, e.y0, e.y1});
    }
    auto clamp_axis = [](double v, std::uint32_t hi) -> std::uint32_t {
        if (v < 0.0) return 0;
        if (v >= static_cast<double>(hi)) return hi;
        return static_cast<std::uint32_t>(v);
    };
    const std::uint32_t x_begin =
        clamp_axis(std::floor(min_x) - 1.0, dst.width);
    const std::uint32_t x_end =
        clamp_axis(std::ceil(max_x) + 1.0, dst.width);
    std::uint32_t y_begin =
        clamp_axis(std::floor(min_y) - 1.0, dst.height);
    std::uint32_t y_end =
        clamp_axis(std::ceil(max_y) + 1.0, dst.height);

    // Intersect the scan extent with the optional clip rectangle
    // (§8.5.4). Output-preserving: pixels outside the clip are simply
    // not visited. Re-declare the x bounds as mutable so the clip can
    // narrow them too.
    std::uint32_t x_begin_c = x_begin, x_end_c = x_end;
    if (clip != nullptr) {
        auto clamp_i = [](std::int32_t v, std::uint32_t hi)
            -> std::uint32_t {
            if (v < 0) return 0;
            if (static_cast<std::uint32_t>(v) > hi) return hi;
            return static_cast<std::uint32_t>(v);
        };
        const std::uint32_t cx0 = clamp_i(clip->x0, dst.width);
        const std::uint32_t cx1 = clamp_i(clip->x1, dst.width);
        const std::uint32_t cy0 = clamp_i(clip->y0, dst.height);
        const std::uint32_t cy1 = clamp_i(clip->y1, dst.height);
        x_begin_c = std::max(x_begin_c, cx0);
        x_end_c = std::min(x_end_c, cx1);
        y_begin = std::max(y_begin, cy0);
        y_end = std::min(y_end, cy1);
    }
    if (x_begin_c >= x_end_c || y_begin >= y_end) return;

    // Pre-clamp source colour + alpha to the [0, 1] domain via
    // the same convention foundation::colorspace::Quantize uses
    // (NaN → 0, <0 → 0, >1 → 1). Multiplying these clamped
    // values by per-pixel coverage stays in range.
    auto clamp01 = [](double v) {
        if (std::isnan(v)) return 0.0;
        if (v < 0.0) return 0.0;
        if (v > 1.0) return 1.0;
        return v;
    };
    const double sr = clamp01(color.r);
    const double sg = clamp01(color.g);
    const double sb = clamp01(color.b);
    const double sa = clamp01(alpha);

    // 4×4 centred sub-pixel grid: offsets (i + 0.5) / 4 for
    // i in 0..3. Total 16 sub-samples per pixel.
    static constexpr double kSubOffsets[4] = {
        0.125, 0.375, 0.625, 0.875,
    };

    // Scanline edge bucketing (active-edge sweep). A naïve fill
    // walks every edge at every sub-sample — O(edges) per sample,
    // which dominates for stroke-heavy paths that emit thousands of
    // short edges scattered over a large bbox. An edge contributes
    // to the winding number at sub-scanline `sy` only while
    // ymin <= sy < ymax (the half-open y-range tested in
    // EdgeWinding); outside that band its contribution is exactly
    // zero. Sweeping sub-scanlines in increasing y and maintaining
    // the set of edges that span the current band lets each
    // sub-sample consider only the active edges. This is purely a
    // performance change — the winding number, coverage, and
    // composite are computed from the identical arithmetic, so the
    // output is bit-for-bit the same as the naïve scan. See
    // rasterizer.yaml "Performance notes".
    const std::size_t edge_count = edges.size();
    std::vector<double> ey_lo(edge_count), ey_hi(edge_count);
    std::vector<std::uint32_t> by_lo(edge_count);
    for (std::size_t i = 0; i < edge_count; ++i) {
        ey_lo[i] = std::min(edges[i].y0, edges[i].y1);
        ey_hi[i] = std::max(edges[i].y0, edges[i].y1);
        by_lo[i] = static_cast<std::uint32_t>(i);
    }
    // Edges ordered by lower y, so the sweep admits them in one
    // monotone pass.
    std::sort(by_lo.begin(), by_lo.end(),
              [&](std::uint32_t a, std::uint32_t b) {
                  return ey_lo[a] < ey_lo[b];
              });

    std::vector<std::uint32_t> active;
    active.reserve(edge_count);
    std::size_t next_enter = 0;  // next index into by_lo to admit

    // Per-column sub-sample hit counts for the row in flight,
    // reused across rows to avoid reallocation.
    const std::uint32_t row_span = x_end_c - x_begin_c;
    std::vector<int> count_inside(row_span);

    for (std::uint32_t py = y_begin; py < y_end; ++py) {
        std::fill(count_inside.begin(), count_inside.end(), 0);

        for (int j = 0; j < 4; ++j) {
            const double sy = static_cast<double>(py)
                              + kSubOffsets[j];

            // Admit edges whose lower y has been reached; sub-scan
            // lines increase monotonically so `next_enter` never
            // rewinds.
            while (next_enter < edge_count &&
                   ey_lo[by_lo[next_enter]] <= sy) {
                active.push_back(by_lo[next_enter]);
                ++next_enter;
            }
            // Retire edges whose upper y has been passed. The
            // y-range is half-open (a sub-sample exactly at ymax is
            // OUTSIDE the edge), matching EdgeWinding's `< ymax`.
            active.erase(
                std::remove_if(active.begin(), active.end(),
                               [&](std::uint32_t e) {
                                   return ey_hi[e] <= sy;
                               }),
                active.end());

            for (std::uint32_t px = x_begin_c; px < x_end_c; ++px) {
                int& c = count_inside[px - x_begin_c];
                for (int i = 0; i < 4; ++i) {
                    const double sx = static_cast<double>(px)
                                      + kSubOffsets[i];
                    int w = 0;
                    for (const std::uint32_t e : active) {
                        w += EdgeWinding(edges[e], sx, sy);
                    }
                    if (Inside(w, rule)) ++c;
                }
            }
        }

        for (std::uint32_t px = x_begin_c; px < x_end_c; ++px) {
            // Count sub-samples whose winding satisfied the fill
            // rule across all four sub-scanlines of this row.
            const int count = count_inside[px - x_begin_c];
            if (count == 0) continue;

            const double coverage =
                static_cast<double>(count) / 16.0;
            const double effective_alpha = sa * coverage;

            // src-over with linear (non-premultiplied) source.
            const std::size_t idx =
                (static_cast<std::size_t>(py) * dst.width + px) * 4;
            const double dr = dst.pixels[idx + 0] / 255.0;
            const double dg = dst.pixels[idx + 1] / 255.0;
            const double db = dst.pixels[idx + 2] / 255.0;
            const double da = dst.pixels[idx + 3] / 255.0;
            const double inv = 1.0 - effective_alpha;
            const double or_ = sr * effective_alpha + dr * inv;
            const double og  = sg * effective_alpha + dg * inv;
            const double ob  = sb * effective_alpha + db * inv;
            const double oa  = effective_alpha + da * inv;

            dst.pixels[idx + 0] =
                foundation::colorspace::Quantize(or_);
            dst.pixels[idx + 1] =
                foundation::colorspace::Quantize(og);
            dst.pixels[idx + 2] =
                foundation::colorspace::Quantize(ob);
            dst.pixels[idx + 3] =
                foundation::colorspace::Quantize(oa);
        }
    }
}

namespace {

// Tiny LE-byte reader for the binary fixture format. Throws
// std::invalid_argument on truncation. Uses memcpy for every
// multi-byte field so misaligned source addresses are handled
// portably.
struct ByteReader {
    const std::byte* p;
    const std::byte* end;

    void need(std::size_t n) {
        if (static_cast<std::size_t>(end - p) < n) {
            throw std::invalid_argument(
                "rasterizer Decode: truncated input");
        }
    }
    std::uint8_t U8() {
        need(1);
        const auto v = std::to_integer<std::uint8_t>(*p);
        ++p;
        return v;
    }
    std::uint32_t U32() {
        need(4);
        std::uint32_t v;
        std::memcpy(&v, p, 4);
        p += 4;
        return v;
    }
    double F64() {
        need(8);
        double v;
        std::memcpy(&v, p, 8);
        p += 8;
        return v;
    }
};

}  // namespace

DecodedImage Decode(std::span<const std::byte> bytes) {
    ByteReader r{bytes.data(), bytes.data() + bytes.size()};

    const std::uint8_t magic = r.U8();
    if (magic != 0xCA) {
        throw std::invalid_argument(
            "rasterizer Decode: bad magic byte");
    }
    const std::uint8_t version = r.U8();
    if (version != 1) {
        throw std::invalid_argument(
            "rasterizer Decode: unsupported version");
    }
    const std::uint8_t fill_rule_byte = r.U8();
    if (fill_rule_byte > 1) {
        throw std::invalid_argument(
            "rasterizer Decode: bad fill_rule");
    }
    (void)r.U8();  // reserved

    const std::uint32_t width = r.U32();
    const std::uint32_t height = r.U32();

    foundation::transform::Matrix ctm{};
    ctm.a = r.F64();
    ctm.b = r.F64();
    ctm.c = r.F64();
    ctm.d = r.F64();
    ctm.e = r.F64();
    ctm.f = r.F64();

    const std::uint8_t cr = r.U8();
    const std::uint8_t cg = r.U8();
    const std::uint8_t cb = r.U8();
    const std::uint8_t ca = r.U8();

    const foundation::colorspace::ColorRgb color{
        cr / 255.0, cg / 255.0, cb / 255.0,
    };
    const double alpha = ca / 255.0;
    const FillRule rule = (fill_rule_byte == 0)
        ? FillRule::NonZero : FillRule::EvenOdd;

    const std::uint32_t num_segments = r.U32();
    Path path;
    path.segments.reserve(num_segments);
    for (std::uint32_t i = 0; i < num_segments; ++i) {
        const std::uint8_t kind_byte = r.U8();
        if (kind_byte > 3) {
            throw std::invalid_argument(
                "rasterizer Decode: bad segment kind");
        }
        const SegmentKind kind = static_cast<SegmentKind>(kind_byte);
        PathSegment seg{};
        seg.kind = kind;
        switch (kind) {
            case SegmentKind::Move:
            case SegmentKind::Line: {
                seg.pts[0].x = r.F64();
                seg.pts[0].y = r.F64();
                break;
            }
            case SegmentKind::Rect: {
                seg.pts[0].x = r.F64();
                seg.pts[0].y = r.F64();
                seg.pts[1].x = r.F64();
                seg.pts[1].y = r.F64();
                break;
            }
            case SegmentKind::Close:
                break;
        }
        path.segments.push_back(seg);
    }

    Raster raster{};
    raster.width = width;
    raster.height = height;
    raster.pixels.assign(
        static_cast<std::size_t>(width) * height * 4, 0);
    Fill(raster, path, ctm, color, alpha, rule);

    DecodedImage out{};
    out.width = width;
    out.height = height;
    out.components = 4;
    out.pixels.resize(raster.pixels.size());
    std::memcpy(out.pixels.data(), raster.pixels.data(),
                raster.pixels.size());
    return out;
}

}  // namespace foundation::rasterizer
