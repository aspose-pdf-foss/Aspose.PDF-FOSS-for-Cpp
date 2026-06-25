#include "glyph_rasterizer.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace foundation::glyph_rasterizer {

namespace {

// Flatness tolerance + recursion depth cap. MUST match the
// values in the test fixtures
// generate_fixtures.py (FLATNESS = 0.25, MAX_DEPTH = 12) so
// the C++ adapter and the NumPy reference produce byte-identical
// flattened polygons.
constexpr double kFlatness = 0.25;
constexpr int kMaxDepth = 12;

using foundation::transform::Point;

// Perpendicular distance from `p` to the chord defined by
// (a → b). When the chord has near-zero length, fall back to
// the ordinary Euclidean distance from `a` to `p` (the chord
// is degenerate, so the "perpendicular" notion is ill-defined).
double PerpendicularDistance(const Point& a, const Point& p,
                             const Point& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double chord = std::sqrt(dx * dx + dy * dy);
    if (chord < 1e-12) {
        const double ddx = p.x - a.x;
        const double ddy = p.y - a.y;
        return std::sqrt(ddx * ddx + ddy * ddy);
    }
    return std::abs((p.x - a.x) * dy - (p.y - a.y) * dx) / chord;
}

inline Point Mid(const Point& a, const Point& b) {
    return Point{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5};
}

// Recursive De Casteljau cubic Bezier flattener. Appends
// segment endpoints (excluding the start P0; including each
// terminal P3) to `out`. Recursion stops when both control
// points fall within kFlatness of the chord OR when depth
// reaches kMaxDepth.
void Flatten(const Point& p0, const Point& p1,
             const Point& p2, const Point& p3,
             std::vector<Point>& out, int depth) {
    const double d1 = PerpendicularDistance(p0, p1, p3);
    const double d2 = PerpendicularDistance(p0, p2, p3);
    if (std::max(d1, d2) <= kFlatness || depth >= kMaxDepth) {
        out.push_back(p3);
        return;
    }
    const Point q0 = Mid(p0, p1);
    const Point q1 = Mid(p1, p2);
    const Point q2 = Mid(p2, p3);
    const Point r0 = Mid(q0, q1);
    const Point r1 = Mid(q1, q2);
    const Point s = Mid(r0, r1);
    Flatten(p0, q0, r0, s, out, depth + 1);
    Flatten(s, r1, q2, p3, out, depth + 1);
}

}  // namespace

void Draw(foundation::rasterizer::Raster& dst,
          const Outline& outline,
          const foundation::transform::Matrix& ctm,
          const foundation::colorspace::ColorRgb& color,
          double alpha,
          const foundation::rasterizer::ClipRect* clip) {
    if (outline.segments.empty()) return;

    // Build a rasterizer Path by walking the outline, applying
    // the CTM, and flattening Curve segments via De Casteljau.
    foundation::rasterizer::Path path;
    path.segments.reserve(outline.segments.size() * 2);

    Point cur{0.0, 0.0};
    Point sub_start{0.0, 0.0};
    bool have_cur = false;

    auto apply = [&](const Point& p) {
        return foundation::transform::Apply(ctm, p);
    };

    auto emit_move = [&](const Point& p) {
        foundation::rasterizer::PathSegment seg{};
        seg.kind = foundation::rasterizer::SegmentKind::Move;
        seg.pts[0] = p;
        path.segments.push_back(seg);
    };
    auto emit_line = [&](const Point& p) {
        foundation::rasterizer::PathSegment seg{};
        seg.kind = foundation::rasterizer::SegmentKind::Line;
        seg.pts[0] = p;
        path.segments.push_back(seg);
    };
    auto emit_close = [&]() {
        foundation::rasterizer::PathSegment seg{};
        seg.kind = foundation::rasterizer::SegmentKind::Close;
        path.segments.push_back(seg);
    };

    for (const auto& seg : outline.segments) {
        switch (seg.op) {
            case OutlineOp::Move: {
                const Point p = apply(seg.pts[0]);
                cur = p;
                sub_start = p;
                have_cur = true;
                emit_move(p);
                break;
            }
            case OutlineOp::Line: {
                if (!have_cur) break;
                const Point p = apply(seg.pts[0]);
                emit_line(p);
                cur = p;
                break;
            }
            case OutlineOp::Curve: {
                if (!have_cur) break;
                const Point c1 = apply(seg.pts[0]);
                const Point c2 = apply(seg.pts[1]);
                const Point end = apply(seg.pts[2]);
                std::vector<Point> flat;
                Flatten(cur, c1, c2, end, flat, 0);
                for (const Point& p : flat) {
                    emit_line(p);
                    cur = p;
                }
                break;
            }
            case OutlineOp::Close: {
                if (have_cur) {
                    emit_close();
                    cur = sub_start;
                }
                break;
            }
        }
    }

    // Glyphs always use NonZero winding — the universal
    // convention for font outlines (PostScript, CFF, TrueType
    // all assume nonzero fills). Pass an identity transform
    // because we already applied the CTM during flattening.
    foundation::rasterizer::Fill(
        dst, path,
        foundation::transform::Identity(),
        color, alpha,
        foundation::rasterizer::FillRule::NonZero,
        clip);
}

namespace {

// Tiny LE-byte reader. Same shape as rasterizer's. memcpy on
// every multi-byte field for portability.
struct ByteReader {
    const std::byte* p;
    const std::byte* end;

    void need(std::size_t n) {
        if (static_cast<std::size_t>(end - p) < n) {
            throw std::invalid_argument(
                "glyph_rasterizer Decode: truncated input");
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

    if (r.U8() != 0xCB) {
        throw std::invalid_argument(
            "glyph_rasterizer Decode: bad magic byte");
    }
    if (r.U8() != 1) {
        throw std::invalid_argument(
            "glyph_rasterizer Decode: unsupported version");
    }
    (void)r.U8();  // reserved
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

    const std::uint32_t num_segments = r.U32();
    Outline outline;
    outline.segments.reserve(num_segments);
    for (std::uint32_t i = 0; i < num_segments; ++i) {
        const std::uint8_t kind_byte = r.U8();
        if (kind_byte > 3) {
            throw std::invalid_argument(
                "glyph_rasterizer Decode: bad segment kind");
        }
        const OutlineOp op = static_cast<OutlineOp>(kind_byte);
        OutlineSegment seg{};
        seg.op = op;
        switch (op) {
            case OutlineOp::Move:
            case OutlineOp::Line:
                seg.pts[0].x = r.F64();
                seg.pts[0].y = r.F64();
                break;
            case OutlineOp::Curve:
                seg.pts[0].x = r.F64();
                seg.pts[0].y = r.F64();
                seg.pts[1].x = r.F64();
                seg.pts[1].y = r.F64();
                seg.pts[2].x = r.F64();
                seg.pts[2].y = r.F64();
                break;
            case OutlineOp::Close:
                break;
        }
        outline.segments.push_back(seg);
    }

    foundation::rasterizer::Raster raster{};
    raster.width = width;
    raster.height = height;
    raster.pixels.assign(
        static_cast<std::size_t>(width) * height * 4, 0);
    Draw(raster, outline, ctm, color, alpha);

    DecodedImage out{};
    out.width = width;
    out.height = height;
    out.components = 4;
    out.pixels.resize(raster.pixels.size());
    std::memcpy(out.pixels.data(), raster.pixels.data(),
                raster.pixels.size());
    return out;
}

}  // namespace foundation::glyph_rasterizer
