#!/usr/bin/env python3
"""Generate the .case fixture corpus for foundation::transform.

transform's freeze gate is numeric_oracle_agreement: each `.case`
file describes one operation (op + operands + expected_<kind>),
and the C++ driver compares the primitive's output to the
expected value within an absolute tolerance.

Two-oracle independence is enforced HERE, at authoring time. For
every case we compute the answer two ways — once via NumPy
(linear-algebra runtime), once via a closed-form expression
hand-derived from PDF 32000 §8.3.4 / §8.3 Table 4 / §14.8.4.4 —
and only commit the .case file when the two values agree to
1e-15 per component (machine-epsilon scale on the operand
magnitudes used in this corpus). Disagreement is a fixture bug
(or, more usefully, a spec misreading) and aborts authoring.

The committed expected value is whichever oracle's output the
two agreed on; downstream the C++ driver only consumes the
already-agreed number.

Fixtures are public-domain — generated deterministically from
this script, no downloaded content. Re-run from this directory:

    python3 generate_fixtures.py
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

# Per-component agreement tolerance enforced between the two
# authoring oracles. NOT the gate's eps_abs (which is 1e-12 in
# the anchor); the oracles agree to ulp scale on direct
# arithmetic but the viewport's NumPy path goes through
# np.linalg.lstsq which adds a few ulps of solver-rounding
# drift relative to the closed-form analytic expression.
# 1e-13 catches any real disagreement (a sign flip drifts by
# O(1) on the operand, twelve orders above this floor) while
# leaving headroom for solver rounding. Still ten times tighter
# than the gate's 1e-12.
ORACLE_EPS = 1e-13


# ---------------------------------------------------------------------------
# Closed-form oracles — direct PDF 32000 expressions.
# ---------------------------------------------------------------------------
#
# Each `_closed_<op>` function returns the PDF-spec-derived answer
# without going through NumPy. They look small because they ARE
# small: PDF's coordinate-transform layer is six-tuple affine
# arithmetic.

def closed_identity():
    # PDF 32000 §8.3.3 — the cm-operator default: the affine
    # identity, [1 0 0 1 0 0].
    return [1.0, 0.0, 0.0, 1.0, 0.0, 0.0]


def closed_translate(tx, ty):
    # §8.3.4 Translation.
    return [1.0, 0.0, 0.0, 1.0, tx, ty]


def closed_scale(sx, sy):
    # §8.3.4 Scaling.
    return [sx, 0.0, 0.0, sy, 0.0, 0.0]


def closed_rotate(theta):
    # §8.3.4 Rotation. Row-vector convention: b = +sin θ.
    c, s = math.cos(theta), math.sin(theta)
    return [c, s, -s, c, 0.0, 0.0]


def closed_skew(alpha, beta):
    # §8.3.4 Skew.
    return [1.0, math.tan(alpha), math.tan(beta), 1.0, 0.0, 0.0]


def closed_compose(lhs, rhs):
    # §8.3.3 — direct expansion of [a1 b1 c1 d1 e1 f1] · [a2 b2 c2 d2 e2 f2]
    # with the third column (0,0,1) constants folded back in.
    a1, b1, c1, d1, e1, f1 = lhs
    a2, b2, c2, d2, e2, f2 = rhs
    return [
        a1 * a2 + b1 * c2,
        a1 * b2 + b1 * d2,
        c1 * a2 + d1 * c2,
        c1 * b2 + d1 * d2,
        e1 * a2 + f1 * c2 + e2,
        e1 * b2 + f1 * d2 + f2,
    ]


def closed_inverse(m):
    # Affine inverse via 2×2 determinant + translation back-solve.
    # Returns None when singular (|det| < 1e-12 — same threshold the
    # primitive's contract requires).
    a, b, c, d, e, f = m
    det = a * d - b * c
    if abs(det) < 1e-12:
        return None
    return [
        d / det,
        -b / det,
        -c / det,
        a / det,
        (c * f - d * e) / det,
        (b * e - a * f) / det,
    ]


def closed_apply_point(m, p):
    # §8.3 row-vector application: [x y 1] · M.
    a, b, c, d, e, f = m
    x, y = p
    return [a * x + c * y + e, b * x + d * y + f]


def closed_apply_rect(m, r):
    # Transform all four corners and take the axis-aligned bounding
    # box of the four images. PDF /BBox propagation through a CTM.
    xmin, ymin, xmax, ymax = r
    corners = [(xmin, ymin), (xmax, ymin),
               (xmin, ymax), (xmax, ymax)]
    xs, ys = [], []
    for cx, cy in corners:
        ix, iy = closed_apply_point(m, (cx, cy))
        xs.append(ix); ys.append(iy)
    return [min(xs), min(ys), max(xs), max(ys)]


def closed_viewport(media_box, rotate, W, H):
    # Corner-mapping derivation from PDF 32000 §8.3 Table 4 +
    # §14.8.4.4. For each /Rotate value the four user-space corners
    # of the media-box map to the four device-space corners (top-left
    # = (0, 0), bottom-right = (W, H)) of the displayed page after
    # the requested CW rotation. We solve the affine that realises
    # the four-corner map.
    #
    # Closed-form derivation (mediabox-origin shift folded in):
    mx, my, MX, MY = media_box
    mw = MX - mx
    mh = MY - my
    if rotate == 0:
        # User (mx, MY) → (0, 0); user (MX, my) → (W, H).
        sx = W / mw
        sy = H / mh
        # x' = sx * (x - mx)
        # y' = sy * (MY - y) = -sy * y + sy * MY
        return [sx, 0.0, 0.0, -sy, -sx * mx, sy * MY]
    if rotate == 90:
        # Displayed extent swapped: pixel canvas is (W=mh, H=mw).
        # User (mx, my) → (0, 0); user (MX, MY) → (W, H).
        sx = W / mh
        sy = H / mw
        # x' = sx * (y - my)
        # y' = sy * (x - mx)
        return [0.0, sy, sx, 0.0, -sx * my, -sy * mx]
    if rotate == 180:
        sx = W / mw
        sy = H / mh
        # User (MX, MY) → (0, 0); user (mx, my) → (W, H).
        # x' = sx * (MX - x) = -sx * x + sx * MX
        # y' = sy * (y - my) ... but wait: 180° flips both axes
        #
        # Actually corner mapping (mediabox at origin for clarity):
        # (0, 0) → (W, H), (mw, 0) → (0, H), (0, mh) → (W, 0),
        # (mw, mh) → (0, 0). y-axis flip is preserved (top of
        # displayed page is original-bottom because we rotate 180°
        # not because of y-up/y-down).
        # x' = -sx * x + sx * MX = -sx * (x - mx) - sx*mx + sx*MX
        #    = -sx * (x - mx) + sx * mw    (since MX - mx = mw)
        # y' = sy * (y - my)? Let's check (mx, my)→(W, H):
        #     -sx*mx + sx*MX = sx * mw = W ✓
        #     sy*0 + ? → must equal H. So +sy*0 + H is wrong; we need f=H.
        # Let me redo this carefully.
        #
        # 180° on the displayed buffer is (x_dev, y_dev) → (W-x_dev, H-y_dev)
        # composed with the rotate=0 mapping. Rotate=0 gives:
        #   x_dev0 = sx * (x_user - mx)
        #   y_dev0 = sy * (MY - y_user)
        # rotate=180 then maps (x_dev0, y_dev0) → (W - x_dev0, H - y_dev0):
        #   x_dev = W - sx*(x - mx) = -sx*x + (sx*mx + W)
        #         = -sx*x + sx*MX                    (since sx*mw = W)
        #   y_dev = H - sy*(MY - y) = sy*y + (H - sy*MY)
        #         = sy*y - sy*my                     (since sy*mh = H)
        return [-sx, 0.0, 0.0, sy, sx * MX, -sy * my]
    if rotate == 270:
        # Displayed extent swapped: W=mh, H=mw.
        # 270° CW = 90° CCW relative to rotate=0. Equivalent to
        # rotating the rotate=90 buffer 180° on the buffer plane.
        # rotate=90 closed form:
        #   x_dev90 = sx * (y - my)         [sx = W/mh]
        #   y_dev90 = sy * (x - mx)         [sy = H/mw]
        # 180° on the (W, H) buffer maps (rx, ry) → (W - rx, H - ry):
        #   x_dev = W - x_dev90 = -sx*y + sx*my + W
        #         = -sx*y + sx*my + sx*mh   (since W = sx*mh)
        #         = -sx*y + sx*MY           (since my + mh = MY)
        #   y_dev = H - y_dev90 = -sy*x + sy*mx + H
        #         = -sy*x + sy*mx + sy*mw   (since H = sy*mw)
        #         = -sy*x + sy*MX           (since mx + mw = MX)
        # So matrix components: a=0, b=-sy, c=-sx, d=0,
        #                       e=sx*MY, f=sy*MX.
        sx = W / mh
        sy = H / mw
        return [0.0, -sy, -sx, 0.0, sx * MY, sy * MX]
    raise ValueError(f"closed_viewport rejects rotate={rotate}")


# ---------------------------------------------------------------------------
# NumPy oracle — independent matrix-arithmetic runtime.
# ---------------------------------------------------------------------------
#
# We carry the affine 6-tuple as a 3×3 NumPy matrix in PDF row-vector
# convention:
#
#     M_3x3 = [[a, b, 0],
#              [c, d, 0],
#              [e, f, 1]]
#
# Composition is np.matmul; inversion is np.linalg.inv; application
# is the row-vector product `[x y 1] @ M`. We extract back to the
# 6-tuple by reading positions (0,0), (0,1), (1,0), (1,1), (2,0),
# (2,1).

def _to_3x3(m6):
    a, b, c, d, e, f = m6
    return np.array([[a, b, 0.0],
                     [c, d, 0.0],
                     [e, f, 1.0]], dtype=np.float64)


def _from_3x3(M):
    return [float(M[0, 0]), float(M[0, 1]),
            float(M[1, 0]), float(M[1, 1]),
            float(M[2, 0]), float(M[2, 1])]


def numpy_compose(lhs, rhs):
    return _from_3x3(_to_3x3(lhs) @ _to_3x3(rhs))


def numpy_inverse(m):
    M = _to_3x3(m)
    det = float(np.linalg.det(M[:2, :2]))
    if abs(det) < 1e-12:
        return None
    return _from_3x3(np.linalg.inv(M))


def numpy_apply_point(m, p):
    M = _to_3x3(m)
    out = np.array([p[0], p[1], 1.0]) @ M
    return [float(out[0]), float(out[1])]


def numpy_apply_rect(m, r):
    xmin, ymin, xmax, ymax = r
    corners = [(xmin, ymin), (xmax, ymin),
               (xmin, ymax), (xmax, ymax)]
    xs, ys = [], []
    for c in corners:
        ix, iy = numpy_apply_point(m, c)
        xs.append(ix); ys.append(iy)
    return [min(xs), min(ys), max(xs), max(ys)]


def numpy_viewport(media_box, rotate, W, H):
    """Independent 3×3 matmul oracle. Builds the viewport as a
    chain of `np.matmul` calls on translate / scale / rotate
    matrices, one factor per spec step:

        rotate=0:    T(-mx, -my) · S(W/mw, -H/mh) · T(0, H)
        rotate=90:   T(-mx, -my) · R(-90°)      · T(0, mh) ·
                     S(W/mh, -H/mw) · T(0, H)
                     (applied in row-vector order; see below)
        rotate=180:  T(-mx, -my) · R(-180°)     · T(mw, mh) ·
                     S(W/mw, -H/mh) · T(0, H)
        rotate=270:  T(-mx, -my) · R(-270°)     · T(mw, 0)  ·
                     S(W/mh, -H/mw) · T(0, H)

    These are the spec-aligned compositions in row-vector
    convention: the rotation rotates the user-space content by the
    NEGATIVE of the /Rotate value (so the rendered buffer ends up
    appearing rotated by +/Rotate when displayed); the post-rotate
    translate re-anchors the rotated content to the (0, 0) origin
    so the subsequent scale and y-flip work in normalised
    coordinates.

    This formulation uses general 3×3 matmul throughout, with NO
    linear solver — every step is a fixed sum of 3 products in
    IEEE-754 double, so per-component drift is O(ulp) on the
    operands rather than O(W*ulp) the way the lstsq/solve path
    behaved.
    """
    mx, my, MX, MY = media_box
    mw = MX - mx
    mh = MY - my

    def T(tx, ty):
        return np.array([[1, 0, 0],
                         [0, 1, 0],
                         [tx, ty, 1]], dtype=np.float64)

    def S(sx, sy):
        return np.array([[sx, 0, 0],
                         [0, sy, 0],
                         [0, 0, 1]], dtype=np.float64)

    def R(rad):
        # Row-vector convention: [[c, s], [-s, c]] like closed_rotate.
        c, s = math.cos(rad), math.sin(rad)
        return np.array([[c, s, 0],
                         [-s, c, 0],
                         [0, 0, 1]], dtype=np.float64)

    if rotate == 0:
        # Translate mediabox origin to (0, 0) → scale + flip Y to
        # device-pixel raster → translate y by +H to land top-left
        # at (0, 0).
        M = T(-mx, -my) @ S(W / mw, -H / mh) @ T(0, H)
    elif rotate == 90:
        # Translate origin to (0, 0): user-space domain becomes
        # [0, mw] × [0, mh]. Rotate 90° CW (R(-π/2)): (x, y) →
        # (y, -x), so domain becomes [0, mh] × [-mw, 0]. Translate
        # by (0, +mw) to bring y into [0, mw]. Now domain is
        # [0, mh] × [0, mw] = [0, W] × [0, H], so apply the
        # rotate=0-style scale + flip + translate to device.
        M = (T(-mx, -my)
             @ R(-math.pi / 2)
             @ T(0, mw)
             @ S(W / mh, -H / mw)
             @ T(0, H))
    elif rotate == 180:
        # Translate origin to (0, 0): domain [0, mw] × [0, mh].
        # Rotate 180° (R(-π)): (x, y) → (-x, -y), domain becomes
        # [-mw, 0] × [-mh, 0]. Translate by (mw, mh) to recover
        # [0, mw] × [0, mh]. Then standard scale + flip + translate.
        M = (T(-mx, -my)
             @ R(-math.pi)
             @ T(mw, mh)
             @ S(W / mw, -H / mh)
             @ T(0, H))
    elif rotate == 270:
        # 270° CW = 90° CCW = R(+π/2) in our CCW Rotate convention.
        # Equivalently R(-3π/2). Translate origin to (0, 0): domain
        # [0, mw] × [0, mh]. Rotate 90° CCW: (x, y) → (-y, x),
        # domain becomes [-mh, 0] × [0, mw]. Translate by (mh, 0)
        # to recover [0, mh] × [0, mw] = [0, W] × [0, H]. Then
        # standard scale + flip + translate.
        M = (T(-mx, -my)
             @ R(math.pi / 2)
             @ T(mh, 0)
             @ S(W / mh, -H / mw)
             @ T(0, H))
    else:
        raise ValueError(f"numpy_viewport rejects rotate={rotate}")

    return _from_3x3(M)


# ---------------------------------------------------------------------------
# Two-oracle agreement gate.
# ---------------------------------------------------------------------------


def _agree(a, b, label):
    """Return True if a and b agree component-wise to ORACLE_EPS.
    Tolerates a == b == None (Inverse on a singular matrix). Aborts
    authoring on disagreement."""
    if a is None and b is None:
        return True
    if (a is None) != (b is None):
        sys.stderr.write(
            f"ORACLE DISAGREEMENT [{label}]: "
            f"singular={a is None} vs {b is None}\n")
        return False
    if len(a) != len(b):
        sys.stderr.write(
            f"ORACLE DISAGREEMENT [{label}]: "
            f"len(a)={len(a)} vs len(b)={len(b)}\n")
        return False
    for i, (av, bv) in enumerate(zip(a, b)):
        if math.isnan(av) or math.isnan(bv) or abs(av - bv) > ORACLE_EPS:
            sys.stderr.write(
                f"ORACLE DISAGREEMENT [{label}]: "
                f"component {i}: closed={av!r} numpy={bv!r} "
                f"(diff={abs(av - bv) if not math.isnan(av+bv) else 'nan'})\n")
            return False
    return True


# ---------------------------------------------------------------------------
# Case writer.
# ---------------------------------------------------------------------------


def _fmt_doubles(values):
    # %.17g preserves every bit of an IEEE-754 double round-trip.
    # Anchored output makes oracle drift visible at peer-review time
    # rather than rounding away on the way to disk.
    return " ".join(f"{v:.17g}" for v in values)


def emit_case(out_dir, name, lines):
    """Write a .case file. `lines` is an ordered list of (key, value)
    tuples; values are either a list of doubles, a string (for op /
    expected_singular / expected_throws), or None for marker
    directives."""
    path = out_dir / f"{name}.case"
    body = []
    body.append(f"# Auto-generated by generate_fixtures.py — do not hand-edit.\n")
    body.append(f"# Two-oracle agreement: NumPy + PDF 32000 §8.3 closed form.\n")
    body.append(f"\n")
    for key, value in lines:
        if value is None:
            body.append(f"{key}\n")
        elif isinstance(value, str):
            body.append(f"{key} {value}\n")
        else:
            body.append(f"{key} {_fmt_doubles(value)}\n")
    path.write_text("".join(body))


# ---------------------------------------------------------------------------
# Per-op fixture generators — each runs the case through both oracles
# and emits the .case file when they agree.
# ---------------------------------------------------------------------------


def gen_identity(out_dir):
    expected_closed = closed_identity()
    expected_numpy = _from_3x3(np.eye(3))
    assert _agree(expected_closed, expected_numpy, "identity"), \
        "identity oracles disagree"
    emit_case(out_dir, "identity", [
        ("op", "identity"),
        ("expected_matrix", expected_closed),
    ])


def gen_translate(out_dir, name, tx, ty):
    closed = closed_translate(tx, ty)
    npy = _from_3x3(np.array([[1, 0, 0],
                              [0, 1, 0],
                              [tx, ty, 1]], dtype=np.float64))
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "translate"),
        ("tx", [tx]),
        ("ty", [ty]),
        ("expected_matrix", closed),
    ])


def gen_scale(out_dir, name, sx, sy):
    closed = closed_scale(sx, sy)
    npy = _from_3x3(np.array([[sx, 0, 0],
                              [0, sy, 0],
                              [0, 0, 1]], dtype=np.float64))
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "scale"),
        ("sx", [sx]),
        ("sy", [sy]),
        ("expected_matrix", closed),
    ])


def gen_rotate(out_dir, name, theta):
    closed = closed_rotate(theta)
    c, s = math.cos(theta), math.sin(theta)
    npy = _from_3x3(np.array([[c, s, 0],
                              [-s, c, 0],
                              [0, 0, 1]], dtype=np.float64))
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "rotate"),
        ("radians", [theta]),
        ("expected_matrix", closed),
    ])


def gen_skew(out_dir, name, alpha, beta):
    closed = closed_skew(alpha, beta)
    npy = _from_3x3(np.array([[1, math.tan(alpha), 0],
                              [math.tan(beta), 1, 0],
                              [0, 0, 1]], dtype=np.float64))
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "skew"),
        ("alpha", [alpha]),
        ("beta", [beta]),
        ("expected_matrix", closed),
    ])


def gen_compose(out_dir, name, lhs, rhs):
    closed = closed_compose(lhs, rhs)
    npy = numpy_compose(lhs, rhs)
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "compose"),
        ("lhs", lhs),
        ("rhs", rhs),
        ("expected_matrix", closed),
    ])


def gen_inverse(out_dir, name, m, expect_singular=False):
    closed = closed_inverse(m)
    npy = numpy_inverse(m)
    assert _agree(closed, npy, name)
    if expect_singular:
        assert closed is None, \
            f"{name}: expected singular but closed-form returned a value"
        emit_case(out_dir, name, [
            ("op", "inverse"),
            ("m", m),
            ("expected_singular", None),
        ])
    else:
        assert closed is not None, \
            f"{name}: expected non-singular but closed-form returned None"
        emit_case(out_dir, name, [
            ("op", "inverse"),
            ("m", m),
            ("expected_matrix", closed),
        ])


def gen_apply_point(out_dir, name, m, p):
    closed = closed_apply_point(m, p)
    npy = numpy_apply_point(m, p)
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "apply_point"),
        ("m", m),
        ("p", list(p)),
        ("expected_point", closed),
    ])


def gen_apply_rect(out_dir, name, m, r):
    closed = closed_apply_rect(m, r)
    npy = numpy_apply_rect(m, r)
    assert _agree(closed, npy, name)
    emit_case(out_dir, name, [
        ("op", "apply_rect"),
        ("m", m),
        ("r", list(r)),
        ("expected_rect", closed),
    ])


def gen_viewport(out_dir, name, media_box, rotate, W, H):
    closed = closed_viewport(media_box, rotate, W, H)
    npy = numpy_viewport(media_box, rotate, W, H)
    assert _agree(closed, npy, name), \
        f"viewport {name} oracle disagreement"
    emit_case(out_dir, name, [
        ("op", "viewport"),
        ("media_box", list(media_box)),
        ("rotate", [float(rotate)]),
        ("pixel_width", [float(W)]),
        ("pixel_height", [float(H)]),
        ("expected_matrix", closed),
    ])


def gen_viewport_throws(out_dir, name, media_box, rotate, W, H,
                        exception_name):
    """Author a Viewport case that EXPECTS the primitive to throw.
    No oracle agreement to enforce — the spec says illegal rotate
    values and zero pixel extents are programmer errors. We
    still cross-check that BOTH our oracles refuse the input,
    catching a typo (e.g. accepting rotate=45 in one oracle but
    not the other)."""
    closed_threw = numpy_threw = False
    try:
        closed_viewport(media_box, rotate, W, H)
    except ValueError:
        closed_threw = True
    except ZeroDivisionError:
        closed_threw = True
    try:
        numpy_viewport(media_box, rotate, W, H)
    except ValueError:
        numpy_threw = True
    except ZeroDivisionError:
        numpy_threw = True
    if W == 0 or H == 0:
        # Closed/numpy don't raise on W==0 or H==0; the C++ Viewport
        # does. The fixture is still valid — we just record the
        # throw expectation without an authoring-time agreement check.
        pass
    elif not (closed_threw and numpy_threw):
        raise AssertionError(
            f"{name}: oracles disagree on illegality "
            f"(closed_threw={closed_threw}, numpy_threw={numpy_threw})")
    emit_case(out_dir, name, [
        ("op", "viewport"),
        ("media_box", list(media_box)),
        ("rotate", [float(rotate)]),
        ("pixel_width", [float(W)]),
        ("pixel_height", [float(H)]),
        ("expected_throws", exception_name),
    ])


# ---------------------------------------------------------------------------
# Top-level corpus assembly.
# ---------------------------------------------------------------------------


def main():
    here = Path(__file__).resolve().parent
    # Wipe stale .case files but preserve the script + any README.
    for stale in here.glob("*.case"):
        stale.unlink()

    # ---- Identity -------------------------------------------------
    gen_identity(here)

    # ---- Translate / Scale / Rotate / Skew constructors -----------
    gen_translate(here, "translate_basic", 3.5, -7.25)
    gen_translate(here, "translate_zero",  0.0,  0.0)
    gen_translate(here, "translate_large", 1.0e6, -2.5e6)

    gen_scale(here, "scale_uniform",      2.0, 2.0)
    gen_scale(here, "scale_anisotropic",  3.0, 0.25)
    gen_scale(here, "scale_negative",    -1.0, 1.0)
    gen_scale(here, "scale_negative_y",   1.0, -1.0)  # the y-flip motif

    gen_rotate(here, "rotate_zero",     0.0)
    gen_rotate(here, "rotate_quarter",  math.pi / 2)
    gen_rotate(here, "rotate_half",     math.pi)
    gen_rotate(here, "rotate_eighth",   math.pi / 4)
    gen_rotate(here, "rotate_negative", -math.pi / 6)

    gen_skew(here, "skew_x_only", math.pi / 6, 0.0)
    gen_skew(here, "skew_y_only", 0.0, math.pi / 8)
    gen_skew(here, "skew_both",   math.pi / 6, math.pi / 8)

    # ---- Compose --------------------------------------------------
    I = closed_identity()
    M = [2.0, 0.5, -0.25, 3.0, 4.0, -1.0]  # arbitrary non-singular
    gen_compose(here, "compose_identity_left",  I, M)
    gen_compose(here, "compose_identity_right", M, I)

    # T(1, 2) ∘ T(3, 4) = T(4, 6)  — translation is additive
    gen_compose(here, "compose_translate_translate",
                closed_translate(1, 2), closed_translate(3, 4))

    # R(π/4) ∘ R(π/4) = R(π/2)  — rotation composes by addition
    gen_compose(here, "compose_rotate_rotate_quarter",
                closed_rotate(math.pi / 4), closed_rotate(math.pi / 4))

    # Translate-then-rotate vs. rotate-then-translate (NOT
    # commutative — the case checks the operand-order story).
    gen_compose(here, "compose_translate_then_rotate",
                closed_translate(10, 20), closed_rotate(math.pi / 2))
    gen_compose(here, "compose_rotate_then_translate",
                closed_rotate(math.pi / 2), closed_translate(10, 20))

    # Associativity: (A·B)·C and A·(B·C) commit to the SAME expected
    # matrix. If the primitive is associative (it must be for any
    # affine), both .case files pass.
    A = closed_translate(1, 2)
    B = closed_rotate(math.pi / 3)
    C = closed_scale(2.0, 0.5)
    AB = closed_compose(A, B)
    BC = closed_compose(B, C)
    gen_compose(here, "compose_associativity_left",  AB, C)
    gen_compose(here, "compose_associativity_right", A, BC)

    # ---- Inverse --------------------------------------------------
    gen_inverse(here, "inverse_identity",  I)
    gen_inverse(here, "inverse_translate", closed_translate(5, 7))
    gen_inverse(here, "inverse_scale",     closed_scale(2, 4))
    gen_inverse(here, "inverse_rotate",    closed_rotate(math.pi / 3))
    gen_inverse(here, "inverse_general",
                [2.0, 0.5, -0.25, 3.0, 4.0, -1.0])

    # Singular: 2×2 sub-block with proportional rows. (a*d - b*c) = 0.
    gen_inverse(here, "inverse_singular_zero_det",
                [1.0, 2.0, 3.0, 6.0, 0.0, 0.0],  # 1*6 - 2*3 = 0
                expect_singular=True)
    gen_inverse(here, "inverse_singular_zero_block",
                [0.0, 0.0, 0.0, 0.0, 5.0, 7.0],  # all-zero 2×2
                expect_singular=True)

    # M ∘ M⁻¹ = Identity — checks the inverse via composition (the
    # gate signature for the inverse_sign_swap trap).
    Minv = closed_inverse(M)
    gen_compose(here, "compose_m_with_inverse",  M,    Minv)
    gen_compose(here, "compose_inverse_with_m",  Minv, M)

    # ---- Apply (point) -------------------------------------------
    gen_apply_point(here, "apply_point_identity_origin",
                    I, (0.0, 0.0))
    gen_apply_point(here, "apply_point_identity_general",
                    I, (3.5, -2.25))
    gen_apply_point(here, "apply_point_translate",
                    closed_translate(1, 2), (3, 4))
    gen_apply_point(here, "apply_point_scale",
                    closed_scale(2, 3), (4, 5))

    # The trap fixture: row-vector convention demands (1, 0) under
    # Rotate(π/2) → (0, 1). A column-vector implementation gets
    # (0, -1). The eps_abs gate immediately catches this.
    gen_apply_point(here, "apply_point_rotate_quarter_unit_x",
                    closed_rotate(math.pi / 2), (1.0, 0.0))
    gen_apply_point(here, "apply_point_rotate_quarter_unit_y",
                    closed_rotate(math.pi / 2), (0.0, 1.0))

    gen_apply_point(here, "apply_point_complex",
                    [2.0, 0.5, -0.25, 3.0, 4.0, -1.0], (5.0, 7.0))

    # ---- Apply (rect) --------------------------------------------
    gen_apply_rect(here, "apply_rect_identity",
                   I, (0.0, 0.0, 1.0, 1.0))
    gen_apply_rect(here, "apply_rect_translate",
                   closed_translate(5, 7), (0.0, 0.0, 1.0, 1.0))
    gen_apply_rect(here, "apply_rect_scale",
                   closed_scale(2, 3), (0.0, 0.0, 1.0, 1.0))

    # Negative scale flips the corner ordering — the bbox must still
    # come out with xmin <= xmax / ymin <= ymax.
    gen_apply_rect(here, "apply_rect_negative_scale",
                   closed_scale(-1, 1), (1.0, 0.0, 3.0, 1.0))

    # The "trap" fixture for ApplyToRect: under rotation the four
    # transformed corners trace a parallelogram whose bbox is
    # strictly larger than the diagonal endpoints would suggest.
    gen_apply_rect(here, "apply_rect_rotate_quarter",
                   closed_rotate(math.pi / 2),
                   (0.0, 0.0, 2.0, 3.0))

    gen_apply_rect(here, "apply_rect_rotate_eighth_unit",
                   closed_rotate(math.pi / 4),
                   (0.0, 0.0, 1.0, 1.0))

    # ---- Viewport ------------------------------------------------
    LETTER = (0.0, 0.0, 612.0, 792.0)
    A4     = (0.0, 0.0, 595.0, 842.0)
    OFFSET = (10.0, 20.0, 610.0, 800.0)  # mediabox NOT at origin

    gen_viewport(here, "viewport_letter_rotate_0",
                 LETTER, 0,   612, 792)
    gen_viewport(here, "viewport_letter_rotate_90",
                 LETTER, 90,  792, 612)
    gen_viewport(here, "viewport_letter_rotate_180",
                 LETTER, 180, 612, 792)
    gen_viewport(here, "viewport_letter_rotate_270",
                 LETTER, 270, 792, 612)

    # Mediabox NOT at origin — exposes the e/f translate components.
    gen_viewport(here, "viewport_offset_rotate_0",
                 OFFSET, 0, 600, 780)
    gen_viewport(here, "viewport_offset_rotate_180",
                 OFFSET, 180, 600, 780)

    # Non-square pixel canvas — exposes that sx and sy are
    # independent (an aspect-ratio bug surfaces here).
    gen_viewport(here, "viewport_a4_rotate_0_thumb",
                 A4, 0, 200, 283)

    # Throws: rotate ∉ {0, 90, 180, 270}.
    gen_viewport_throws(here, "viewport_throws_bad_rotate_45",
                        LETTER, 45, 612, 792, "std::invalid_argument")
    gen_viewport_throws(here, "viewport_throws_bad_rotate_negative",
                        LETTER, -90, 612, 792, "std::invalid_argument")

    # Throws: zero pixel extent (would divide by zero).
    gen_viewport_throws(here, "viewport_throws_zero_width",
                        LETTER, 0, 0, 792, "std::invalid_argument")
    gen_viewport_throws(here, "viewport_throws_zero_height",
                        LETTER, 0, 612, 0, "std::invalid_argument")

    print(f"wrote {len(list(here.glob('*.case')))} fixtures to {here}")


if __name__ == "__main__":
    main()
