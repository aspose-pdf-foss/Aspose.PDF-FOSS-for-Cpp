#include "image_compositor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace foundation::image_compositor {

namespace {

// 4×4 centred sub-pixel grid offsets, identical to the convention
// foundation::rasterizer uses. Sub-sample i is at (i + 0.5) / 4 of
// the destination pixel — kept as a constexpr table so the inner
// loop can see the values immediately.
constexpr double kSubOffsets[4] = {0.125, 0.375, 0.625, 0.875};

// Clamp [0, 1] using the exact same convention as
// foundation::colorspace::Quantize: NaN → 0, < 0 → 0, > 1 → 1.
// Callers feed this to the global-alpha multiplier so out-of-domain
// inputs behave identically to the rasterizer's clamp.
inline double Clamp01(double v) noexcept {
    if (std::isnan(v)) return 0.0;
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

}  // namespace

void Composite(foundation::rasterizer::Raster& dst,
               const ImageSource& src,
               const foundation::transform::Matrix& ctm,
               double alpha,
               const foundation::rasterizer::ClipRect* clip) {
    // Zero-sized destination → no work, no allocation. The pixel
    // loop would have zero iterations anyway, but checking up front
    // skips the inverse-CTM computation too.
    if (dst.width == 0 || dst.height == 0) return;
    if (dst.pixels.size() !=
        static_cast<std::size_t>(dst.width) * dst.height * 4) {
        return;  // ill-formed dst; caller error, no-op for safety
    }

    // Zero-sized source → every sub-sample falls outside [0, 1)²
    // (the unit rect maps to an empty source); avg_a stays 0 for
    // every pixel, leaving dst untouched.
    if (src.width == 0 || src.height == 0) return;
    if (src.pixels.size() !=
        static_cast<std::size_t>(src.width) * src.height * 4) {
        return;  // ill-formed src; same policy as ill-formed dst
    }

    // Inverse CTM precompute. The CTM maps source unit-space to
    // destination pixels; we walk destination pixels and need the
    // backward map. A singular CTM (det == 0, or det whose magnitude
    // falls below transform::Inverse's 1e-12 floor) means the source
    // collapses to a line or point — there is no well-defined source
    // coordinate for any destination point, so Composite is a no-op.
    const auto inv_opt = foundation::transform::Inverse(ctm);
    if (!inv_opt.has_value()) return;
    const foundation::transform::Matrix inv = *inv_opt;

    const double alpha_clamped = Clamp01(alpha);
    if (alpha_clamped == 0.0) return;  // global-alpha=0 short-circuit

    const double inv_src_w = static_cast<double>(src.width);
    const double inv_src_h = static_cast<double>(src.height);

    // Device-space bounding box of the image quad — the unit square
    // [0, 1]² mapped FORWARD through the CTM. Every destination
    // sub-sample outside this box inverse-maps to (u, v) outside
    // [0, 1)² and so contributes zero coverage (the uv-range test
    // below already `continue`s it), leaving dst untouched. Bounding
    // the pixel loop to the box is therefore output-preserving and
    // turns Composite from O(dst-pixels) into O(image-device-pixels):
    // tiling a small image cell across a large page no longer pays
    // for the whole page on every tile. One pixel of padding on each
    // side keeps every boundary sub-sample inside the iterated range.
    double qminx = 1e300, qminy = 1e300, qmaxx = -1e300, qmaxy = -1e300;
    for (const auto& corner : {
             foundation::transform::Point{0.0, 0.0},
             foundation::transform::Point{1.0, 0.0},
             foundation::transform::Point{1.0, 1.0},
             foundation::transform::Point{0.0, 1.0}}) {
        const auto p = foundation::transform::Apply(ctm, corner);
        qminx = std::min(qminx, p.x);
        qmaxx = std::max(qmaxx, p.x);
        qminy = std::min(qminy, p.y);
        qmaxy = std::max(qmaxy, p.y);
    }
    auto clamp_axis = [](double v, std::uint32_t hi) -> std::uint32_t {
        if (v < 0.0) return 0;
        if (v >= static_cast<double>(hi)) return hi;
        return static_cast<std::uint32_t>(v);
    };
    std::uint32_t px_begin =
        clamp_axis(std::floor(qminx) - 1.0, dst.width);
    std::uint32_t px_end =
        clamp_axis(std::ceil(qmaxx) + 1.0, dst.width);
    std::uint32_t py_begin =
        clamp_axis(std::floor(qminy) - 1.0, dst.height);
    std::uint32_t py_end =
        clamp_axis(std::ceil(qmaxy) + 1.0, dst.height);

    // Intersect with the optional clip rectangle (§8.5.4). A pixel
    // outside the clip is simply not visited; output-preserving when
    // the clip covers the raster.
    if (clip != nullptr) {
        auto clamp_i = [](std::int32_t v, std::uint32_t hi)
            -> std::uint32_t {
            if (v < 0) return 0;
            if (static_cast<std::uint32_t>(v) > hi) return hi;
            return static_cast<std::uint32_t>(v);
        };
        px_begin = std::max(px_begin, clamp_i(clip->x0, dst.width));
        px_end = std::min(px_end, clamp_i(clip->x1, dst.width));
        py_begin = std::max(py_begin, clamp_i(clip->y0, dst.height));
        py_end = std::min(py_end, clamp_i(clip->y1, dst.height));
    }
    if (px_begin >= px_end || py_begin >= py_end) return;

    for (std::uint32_t py = py_begin; py < py_end; ++py) {
        for (std::uint32_t px = px_begin; px < px_end; ++px) {
            // Accumulate normalised RGBA over 16 sub-samples. Sub-
            // samples that fall outside [0, 1)² contribute zero to
            // BOTH the colour numerator AND the alpha — the divisor
            // stays 16, which is what gives partial-coverage edges
            // their correct softened alpha.
            double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0, sum_a = 0.0;

            for (int j = 0; j < 4; ++j) {
                const double sy = static_cast<double>(py)
                                  + kSubOffsets[j];
                for (int i = 0; i < 4; ++i) {
                    const double sx = static_cast<double>(px)
                                      + kSubOffsets[i];
                    const auto uv = foundation::transform::Apply(
                        inv,
                        foundation::transform::Point{sx, sy});
                    if (uv.x < 0.0 || uv.x >= 1.0) continue;
                    if (uv.y < 0.0 || uv.y >= 1.0) continue;

                    // Nearest-neighbour, integer-floor with a drift
                    // clamp at the right/bottom edge. The clamp is a
                    // belt-and-suspenders for IEEE-754 drift in the
                    // Inverse × Apply composition; for u, v strictly
                    // less than 1.0 the floor never reaches the
                    // dimension count.
                    int u_pix = static_cast<int>(uv.x * inv_src_w);
                    int v_pix = static_cast<int>(uv.y * inv_src_h);
                    if (u_pix < 0) u_pix = 0;
                    else if (static_cast<std::uint32_t>(u_pix) >=
                             src.width) {
                        u_pix = static_cast<int>(src.width) - 1;
                    }
                    if (v_pix < 0) v_pix = 0;
                    else if (static_cast<std::uint32_t>(v_pix) >=
                             src.height) {
                        v_pix = static_cast<int>(src.height) - 1;
                    }

                    const std::size_t s_idx =
                        (static_cast<std::size_t>(v_pix) * src.width
                         + static_cast<std::size_t>(u_pix)) * 4;
                    const std::uint8_t sr = std::to_integer<
                        std::uint8_t>(src.pixels[s_idx + 0]);
                    const std::uint8_t sg = std::to_integer<
                        std::uint8_t>(src.pixels[s_idx + 1]);
                    const std::uint8_t sb = std::to_integer<
                        std::uint8_t>(src.pixels[s_idx + 2]);
                    const std::uint8_t sa = std::to_integer<
                        std::uint8_t>(src.pixels[s_idx + 3]);
                    sum_r += sr / 255.0;
                    sum_g += sg / 255.0;
                    sum_b += sb / 255.0;
                    sum_a += sa / 255.0;
                }
            }

            const double avg_r = sum_r / 16.0;
            const double avg_g = sum_g / 16.0;
            const double avg_b = sum_b / 16.0;
            const double avg_a = sum_a / 16.0;

            const double eff_a = avg_a * alpha_clamped;
            if (eff_a == 0.0) continue;  // no source coverage

            const std::size_t d_idx =
                (static_cast<std::size_t>(py) * dst.width + px) * 4;
            const double dr = dst.pixels[d_idx + 0] / 255.0;
            const double dg = dst.pixels[d_idx + 1] / 255.0;
            const double db = dst.pixels[d_idx + 2] / 255.0;
            const double da = dst.pixels[d_idx + 3] / 255.0;
            const double inv_eff = 1.0 - eff_a;

            // avg_* are coverage-premultiplied (sum of in-bounds
            // samples / 16), so the source colour is composited with
            // the GLOBAL alpha only — coverage is already baked into
            // avg_*. Multiplying by eff_a (= avg_a * alpha) here would
            // apply coverage a SECOND time, darkening partial-coverage
            // edges toward black (a 1-px rim on every image edge, and
            // a visible seam at tiling-pattern cell boundaries). The
            // destination term uses inv_eff = 1 - avg_a*alpha, the
            // correct src-over coverage for the (premultiplied) source.
            const double or_ = avg_r * alpha_clamped + dr * inv_eff;
            const double og  = avg_g * alpha_clamped + dg * inv_eff;
            const double ob  = avg_b * alpha_clamped + db * inv_eff;
            const double oa  = eff_a + da * inv_eff;

            dst.pixels[d_idx + 0] =
                foundation::colorspace::Quantize(or_);
            dst.pixels[d_idx + 1] =
                foundation::colorspace::Quantize(og);
            dst.pixels[d_idx + 2] =
                foundation::colorspace::Quantize(ob);
            dst.pixels[d_idx + 3] =
                foundation::colorspace::Quantize(oa);
        }
    }
}

namespace {

// Tiny LE-byte reader. Same shape as rasterizer's. memcpy on every
// multi-byte field for portability across alignment-sensitive
// architectures.
struct ByteReader {
    const std::byte* p;
    const std::byte* end;

    void need(std::size_t n) {
        if (static_cast<std::size_t>(end - p) < n) {
            throw std::invalid_argument(
                "image_compositor Decode: truncated input");
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

    if (r.U8() != 0xCC) {
        throw std::invalid_argument(
            "image_compositor Decode: bad magic byte");
    }
    if (r.U8() != 1) {
        throw std::invalid_argument(
            "image_compositor Decode: unsupported version");
    }
    (void)r.U8();  // reserved
    (void)r.U8();  // reserved

    const std::uint32_t dst_w = r.U32();
    const std::uint32_t dst_h = r.U32();
    const std::uint32_t src_w = r.U32();
    const std::uint32_t src_h = r.U32();

    foundation::transform::Matrix ctm{};
    ctm.a = r.F64();
    ctm.b = r.F64();
    ctm.c = r.F64();
    ctm.d = r.F64();
    ctm.e = r.F64();
    ctm.f = r.F64();

    const double alpha = r.F64();

    const std::size_t src_bytes =
        static_cast<std::size_t>(src_w) * src_h * 4;
    r.need(src_bytes);

    // Owning buffer for the fixture's source pixels; ImageSource is a
    // non-owning view, so this must outlive the Composite call below.
    std::vector<std::byte> src_buf(src_bytes);
    if (src_bytes > 0) {
        std::memcpy(src_buf.data(), r.p, src_bytes);
        r.p += src_bytes;
    }
    ImageSource src{};
    src.width = src_w;
    src.height = src_h;
    src.pixels = src_buf;

    foundation::rasterizer::Raster raster{};
    raster.width = dst_w;
    raster.height = dst_h;
    raster.pixels.assign(
        static_cast<std::size_t>(dst_w) * dst_h * 4, 0);
    Composite(raster, src, ctm, alpha);

    DecodedImage out{};
    out.width = dst_w;
    out.height = dst_h;
    out.components = 4;
    out.pixels.resize(raster.pixels.size());
    if (!raster.pixels.empty()) {
        std::memcpy(out.pixels.data(), raster.pixels.data(),
                    raster.pixels.size());
    }
    return out;
}

}  // namespace foundation::image_compositor
