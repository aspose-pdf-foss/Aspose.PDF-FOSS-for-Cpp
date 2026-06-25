#include "transform.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace foundation::transform {

Matrix Translate(double tx, double ty) noexcept {
    return Matrix{1.0, 0.0, 0.0, 1.0, tx, ty};
}

Matrix Scale(double sx, double sy) noexcept {
    return Matrix{sx, 0.0, 0.0, sy, 0.0, 0.0};
}

Matrix Rotate(double radians) noexcept {
    // PDF row-vector convention (PDF 32000-1:2008 §8.3.4):
    // a = cos θ, b = +sin θ, c = -sin θ, d = cos θ.
    // The +sin/-sin pairing is the row-vector form — column-
    // vector implementations would swap signs on b and c.
    const double s = std::sin(radians);
    const double c = std::cos(radians);
    return Matrix{c, s, -s, c, 0.0, 0.0};
}

Matrix Skew(double alpha_x_radians, double beta_y_radians) noexcept {
    return Matrix{1.0,
                  std::tan(alpha_x_radians),
                  std::tan(beta_y_radians),
                  1.0, 0.0, 0.0};
}

Matrix Compose(const Matrix& lhs, const Matrix& rhs) noexcept {
    // Row-vector multiplication of the embedded 3×3 affine
    // matrices: lhs · rhs. Third column of each operand is
    // (0, 0, 1) by construction (affine sub-group invariant);
    // the resulting product preserves it without storage.
    return Matrix{
        lhs.a * rhs.a + lhs.b * rhs.c,
        lhs.a * rhs.b + lhs.b * rhs.d,
        lhs.c * rhs.a + lhs.d * rhs.c,
        lhs.c * rhs.b + lhs.d * rhs.d,
        lhs.e * rhs.a + lhs.f * rhs.c + rhs.e,
        lhs.e * rhs.b + lhs.f * rhs.d + rhs.f,
    };
}

std::optional<Matrix> Inverse(const Matrix& m) noexcept {
    // 2×2 determinant of the linear sub-block.
    const double det = m.a * m.d - m.b * m.c;
    // Singularity threshold matches the anchor's stated 1e-12.
    if (std::abs(det) < 1e-12) {
        return std::nullopt;
    }
    const double inv_det = 1.0 / det;
    return Matrix{
         m.d * inv_det,
        -m.b * inv_det,
        -m.c * inv_det,
         m.a * inv_det,
        (m.c * m.f - m.d * m.e) * inv_det,
        (m.b * m.e - m.a * m.f) * inv_det,
    };
}

Point Apply(const Matrix& m, const Point& p) noexcept {
    // Row-vector application: [x y 1] · M projected to (x', y').
    return Point{
        m.a * p.x + m.c * p.y + m.e,
        m.b * p.x + m.d * p.y + m.f,
    };
}

Rect ApplyToRect(const Matrix& m, const Rect& r) noexcept {
    // Transform all four corners and bound them. Necessary because
    // a non-axis-aligned matrix (rotation, shear) sends the rect
    // to a parallelogram whose axis-aligned bbox is strictly larger
    // than the diagonal endpoints would suggest.
    const Point p1 = Apply(m, Point{r.xmin, r.ymin});
    const Point p2 = Apply(m, Point{r.xmax, r.ymin});
    const Point p3 = Apply(m, Point{r.xmin, r.ymax});
    const Point p4 = Apply(m, Point{r.xmax, r.ymax});
    return Rect{
        std::min({p1.x, p2.x, p3.x, p4.x}),
        std::min({p1.y, p2.y, p3.y, p4.y}),
        std::max({p1.x, p2.x, p3.x, p4.x}),
        std::max({p1.y, p2.y, p3.y, p4.y}),
    };
}

Matrix Viewport(const Rect& media_box, int rotate_degrees,
                std::uint32_t pixel_width,
                std::uint32_t pixel_height) {
    // Per PDF 32000-1:2008 §14.8.4.4 the only legal /Rotate values
    // are 0, 90, 180, 270; other values are programmer error.
    if (rotate_degrees != 0 && rotate_degrees != 90 &&
        rotate_degrees != 180 && rotate_degrees != 270) {
        throw std::invalid_argument(
            "Viewport: rotate_degrees must be one of 0, 90, 180, 270");
    }
    // A zero-pixel raster is meaningless; also avoids divide-by-zero
    // in the scale step below.
    if (pixel_width == 0 || pixel_height == 0) {
        throw std::invalid_argument(
            "Viewport: pixel_width and pixel_height must be non-zero");
    }

    const double mx = media_box.xmin;
    const double my = media_box.ymin;
    const double mw = media_box.xmax - mx;  // page x-extent
    const double mh = media_box.ymax - my;  // page y-extent
    const double W  = static_cast<double>(pixel_width);
    const double H  = static_cast<double>(pixel_height);

    // Per /Rotate value, build the viewport as a left-to-right
    // composition of five factors:
    //
    //   T_origin  · R(−rotate_rad) · T_reanchor · S · T_dev
    //
    // where:
    //   T_origin    shifts the mediabox origin to (0, 0).
    //   R(−rad)     rotates the user-space content by the negative
    //               of the /Rotate value (since PDF /Rotate is the
    //               CW angle the displayed page is rotated, and our
    //               Rotate function is CCW; rotating user-space by
    //               −rotate yields a buffer that, displayed natively,
    //               appears as the requested rotation).
    //   T_reanchor  shifts the rotated content back into the first
    //               quadrant so the subsequent scale factors apply
    //               cleanly. The rotated domain depends on rotate:
    //                 rotate=0:    no rotation, no re-anchor.
    //                 rotate=90:   domain becomes [0, mh] × [-mw, 0],
    //                              so T(0, mw).
    //                 rotate=180:  domain becomes [-mw, 0] × [-mh, 0],
    //                              so T(mw, mh).
    //                 rotate=270:  domain becomes [-mh, 0] × [0, mw],
    //                              so T(mh, 0).
    //   S           scales user-extent → pixel-extent and flips Y
    //               (PDF user space is y-up, device buffer is y-down).
    //               For rotate ∈ {0, 180}: S(W/mw, -H/mh).
    //               For rotate ∈ {90, 270}: S(W/mh, -H/mw) — extents
    //               swapped because the displayed page is now landscape.
    //   T_dev       shifts the y-flipped content's origin from (0, -H)
    //               back to (0, 0): T(0, H).
    //
    // The compositions below were derived in the project spec
    // transform/generate_fixtures.py against PDF 32000 §8.3 Table 4
    // and verified by both NumPy 3×3 matmul AND the closed-form
    // 6-tuple expressions in the same file.
    Matrix accumulated = Translate(-mx, -my);

    if (rotate_degrees == 0) {
        // Skip rotation and re-anchor; go straight to scale + dev.
        accumulated = Compose(accumulated, Scale(W / mw, -H / mh));
        accumulated = Compose(accumulated, Translate(0.0, H));
        return accumulated;
    }

    constexpr double kPi = 3.14159265358979323846;
    if (rotate_degrees == 90) {
        accumulated = Compose(accumulated, Rotate(-kPi / 2.0));
        accumulated = Compose(accumulated, Translate(0.0, mw));
        accumulated = Compose(accumulated, Scale(W / mh, -H / mw));
        accumulated = Compose(accumulated, Translate(0.0, H));
        return accumulated;
    }
    if (rotate_degrees == 180) {
        accumulated = Compose(accumulated, Rotate(-kPi));
        accumulated = Compose(accumulated, Translate(mw, mh));
        accumulated = Compose(accumulated, Scale(W / mw, -H / mh));
        accumulated = Compose(accumulated, Translate(0.0, H));
        return accumulated;
    }
    // rotate_degrees == 270, the only remaining value after the
    // input check at the top of this function.
    accumulated = Compose(accumulated, Rotate(kPi / 2.0));  // = Rotate(-3π/2)
    accumulated = Compose(accumulated, Translate(mh, 0.0));
    accumulated = Compose(accumulated, Scale(W / mh, -H / mw));
    accumulated = Compose(accumulated, Translate(0.0, H));
    return accumulated;
}

} // namespace foundation::transform
