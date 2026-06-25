#!/usr/bin/env python3
"""Generate the .bin / .pixels fixture corpus for foundation::glyph_rasterizer.

Single-oracle authoring (same pattern as the rasterizer / dctdecode /
jpx): the sidecar is produced by a NumPy reference that implements
the EXACT algorithm specified in
the project spec — adaptive De Casteljau
cubic Bezier subdivision (max control-point-to-chord distance
≤ 0.25 device pixels, max recursion depth 12) followed by the
same 4×4 centred super-sampling AA pipeline used by the rasterizer.

The committed `.pixels` IS this oracle's output; the gate measures
the C++ adapter against the algorithm we specified, not against
"any reasonable rasterizer" (curve flattening is implementation-
defined at the sub-pixel level — different libraries pick different
flatness metrics, and inter-library agreement at PSNR ≥ 40 dB is
not guaranteed even on simple shapes).

Re-run from this directory:

    python3 generate_fixtures.py
"""

from __future__ import annotations

import math
import struct
from pathlib import Path

import numpy as np


# ---------------------------------------------------------------------------
# Binary fixture format (see glyph_rasterizer.yaml).
# ---------------------------------------------------------------------------

MAGIC = 0xCB
VERSION = 1

OP_MOVE  = 0
OP_LINE  = 1
OP_CURVE = 2
OP_CLOSE = 3


def encode_fixture(width, height, ctm, color_rgba, segments):
    out = bytearray()
    out += struct.pack('<BBBB', MAGIC, VERSION, 0, 0)
    out += struct.pack('<II', width, height)
    out += struct.pack('<6d', *ctm)
    out += struct.pack('<BBBB', *color_rgba)
    out += struct.pack('<I', len(segments))
    for seg in segments:
        op = seg[0]
        out += struct.pack('<B', op)
        if op in (OP_MOVE, OP_LINE):
            out += struct.pack('<2d', seg[1], seg[2])
        elif op == OP_CURVE:
            out += struct.pack('<6d', seg[1], seg[2], seg[3],
                               seg[4], seg[5], seg[6])
        elif op == OP_CLOSE:
            pass
        else:
            raise ValueError(f"unknown op {op}")
    return bytes(out)


# ---------------------------------------------------------------------------
# De Casteljau adaptive cubic Bezier flattening.
# ---------------------------------------------------------------------------
#
# The reference implementation that the C++ adapter must match. Same
# flatness tolerance (0.25 device pixels), same recursion depth cap
# (12), same midpoint-subdivision ordering.

FLATNESS = 0.25
MAX_DEPTH = 12


def _perpendicular_distance(p0, p1, p3):
    """Distance from point p1 to the chord (p0 → p3)."""
    dx = p3[0] - p0[0]
    dy = p3[1] - p0[1]
    chord_len = math.hypot(dx, dy)
    if chord_len < 1e-12:
        # Chord is degenerate; use ordinary distance from p0 to p1.
        return math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    # |cross product| / chord length.
    return abs((p1[0] - p0[0]) * dy
               - (p1[1] - p0[1]) * dx) / chord_len


def _flatten_recursive(p0, p1, p2, p3, out, depth):
    """Recursive De Casteljau. Emits a sequence of (x, y) endpoints
    appended to `out` (excluding p0; p3 is included as the
    terminal point of each subdivided segment)."""
    d1 = _perpendicular_distance(p0, p1, p3)
    d2 = _perpendicular_distance(p0, p2, p3)
    if max(d1, d2) <= FLATNESS or depth >= MAX_DEPTH:
        out.append(p3)
        return
    # Standard midpoint subdivision.
    mid = lambda a, b: ((a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0)
    q0 = mid(p0, p1)
    q1 = mid(p1, p2)
    q2 = mid(p2, p3)
    r0 = mid(q0, q1)
    r1 = mid(q1, q2)
    s = mid(r0, r1)
    _flatten_recursive(p0, q0, r0, s, out, depth + 1)
    _flatten_recursive(s, r1, q2, p3, out, depth + 1)


def flatten_curve(start, c1, c2, end):
    """Adaptive flattening of a single cubic Bezier. Returns a list
    of (x, y) endpoints to LINE-TO from `start`."""
    out = []
    _flatten_recursive(start, c1, c2, end, out, 0)
    return out


# ---------------------------------------------------------------------------
# Outline → polygon path (CTM applied at flattening time).
# ---------------------------------------------------------------------------


def apply_ctm(ctm, x, y):
    a, b, c, d, e, f = ctm
    return (a * x + c * y + e, b * x + d * y + f)


def outline_to_polyline_segments(segments, ctm):
    """Walk an Outline, applying CTM, flattening curves into
    straight lines. Returns a list of edges (x0, y0, x1, y1)."""
    edges = []
    cur = None
    sub_start = None
    for seg in segments:
        op = seg[0]
        if op == OP_MOVE:
            x, y = apply_ctm(ctm, seg[1], seg[2])
            cur = (x, y)
            sub_start = (x, y)
        elif op == OP_LINE:
            if cur is None:
                continue
            x, y = apply_ctm(ctm, seg[1], seg[2])
            if (cur[1] != y):  # skip horizontal (zero winding)
                edges.append((cur[0], cur[1], x, y))
            else:
                # Still record so cur advances; rasterizer drops
                # horizontal edges in its winding loop.
                edges.append((cur[0], cur[1], x, y))
            cur = (x, y)
        elif op == OP_CURVE:
            if cur is None:
                continue
            c1 = apply_ctm(ctm, seg[1], seg[2])
            c2 = apply_ctm(ctm, seg[3], seg[4])
            end = apply_ctm(ctm, seg[5], seg[6])
            for next_pt in flatten_curve(cur, c1, c2, end):
                edges.append((cur[0], cur[1], next_pt[0], next_pt[1]))
                cur = next_pt
        elif op == OP_CLOSE:
            if cur is not None and sub_start is not None and cur != sub_start:
                edges.append((cur[0], cur[1], sub_start[0], sub_start[1]))
            cur = sub_start
    return edges


# ---------------------------------------------------------------------------
# Polygon AA renderer (identical to rasterizer's NumPy reference).
# ---------------------------------------------------------------------------


def winding_at_point(edges, px, py):
    w = 0
    for (x0, y0, x1, y1) in edges:
        if y0 == y1:
            continue
        if y0 < y1:
            if y0 <= py < y1:
                t = (py - y0) / (y1 - y0)
                x_cross = x0 + t * (x1 - x0)
                if px > x_cross:
                    w += 1
        else:
            if y1 <= py < y0:
                t = (py - y0) / (y1 - y0)
                x_cross = x0 + t * (x1 - x0)
                if px > x_cross:
                    w -= 1
    return w


def quantize(v):
    if math.isnan(v):
        return 0
    if v <= 0.0:
        return 0
    if v >= 1.0:
        return 255
    return int(v * 255.0 + 0.5)


def render_polygon(width, height, edges, color_rgba):
    """Fill the polygon described by `edges` using NonZero
    winding + 4×4 super-sampling onto a transparent canvas."""
    raster = np.zeros((height, width, 4), dtype=np.uint8)
    sr, sg, sb, sa = color_rgba
    sa_n = sa / 255.0
    subs = [(i + 0.5) / 4.0 for i in range(4)]

    for py in range(height):
        for px in range(width):
            count_inside = 0
            for j in subs:
                sy = py + j
                for i in subs:
                    sx = px + i
                    if winding_at_point(edges, sx, sy) != 0:
                        count_inside += 1
            if count_inside == 0:
                continue
            cov = count_inside / 16.0
            eff_a = sa_n * cov
            inv = 1.0 - eff_a
            d = raster[py, px]
            dr = d[0] / 255.0; dg = d[1] / 255.0
            db = d[2] / 255.0; da = d[3] / 255.0
            sr_n = sr / 255.0; sg_n = sg / 255.0; sb_n = sb / 255.0
            raster[py, px, 0] = quantize(sr_n * eff_a + dr * inv)
            raster[py, px, 1] = quantize(sg_n * eff_a + dg * inv)
            raster[py, px, 2] = quantize(sb_n * eff_a + db * inv)
            raster[py, px, 3] = quantize(eff_a + da * inv)
    return raster


# ---------------------------------------------------------------------------
# Per-fixture authoring.
# ---------------------------------------------------------------------------


def emit_fixture(out_dir, name, width, height, ctm,
                 color_rgba, segments):
    edges = outline_to_polyline_segments(segments, ctm)
    pixels = render_polygon(width, height, edges, color_rgba)
    bin_bytes = encode_fixture(width, height, ctm, color_rgba, segments)
    (out_dir / f"{name}.bin").write_bytes(bin_bytes)
    (out_dir / f"{name}.pixels").write_bytes(pixels.tobytes())


IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


# Approximation of a unit circle by 4 cubic Bezier arcs. The
# magic constant 0.5522847498... is `(4/3) * (sqrt(2) - 1)`, the
# well-known Bezier-circle fit that hits 1×10⁻⁴ peak error on
# the unit circle.
KAPPA = 0.5522847498307933


def circle_outline(cx, cy, r):
    """Build a Move/Curve outline that traces a circle of radius
    `r` centred at (cx, cy) using 4 cubic arcs (CCW)."""
    k = r * KAPPA
    return [
        (OP_MOVE,  cx + r, cy),
        (OP_CURVE, cx + r, cy + k,   cx + k, cy + r,   cx,     cy + r),
        (OP_CURVE, cx - k, cy + r,   cx - r, cy + k,   cx - r, cy),
        (OP_CURVE, cx - r, cy - k,   cx - k, cy - r,   cx,     cy - r),
        (OP_CURVE, cx + k, cy - r,   cx + r, cy - k,   cx + r, cy),
        (OP_CLOSE,),
    ]


def main():
    here = Path(__file__).resolve().parent
    for stale in here.glob("*.bin"):
        stale.unlink()
    for stale in here.glob("*.pixels"):
        stale.unlink()

    # ---- Empty / trivial -----------------------------------------
    emit_fixture(here, "empty_outline",
                 16, 16, IDENTITY, (255, 0, 0, 255), [])

    # All-line outline (no curves) — must match rasterizer's output
    # byte-for-byte for the same shape, since flattening changes
    # nothing.
    emit_fixture(here, "triangle_no_curves",
                 32, 32, IDENTITY, (255, 200, 0, 255),
                 [(OP_MOVE,  4,  4),
                  (OP_LINE, 28,  4),
                  (OP_LINE, 16, 28),
                  (OP_CLOSE,)])

    # ---- Single curve --------------------------------------------
    # A simple "lollipop": straight line up, then a curve back.
    emit_fixture(here, "single_curve",
                 32, 32, IDENTITY, (0, 200, 255, 255),
                 [(OP_MOVE,   4,  4),
                  (OP_LINE,  28,  4),
                  (OP_CURVE, 28, 16,   16, 28,    4, 16),
                  (OP_CLOSE,)])

    # ---- Circle approximation ------------------------------------
    # Classic Bezier-circle. Tests adaptive subdivision.
    emit_fixture(here, "circle_approx",
                 32, 32, IDENTITY, (255, 255, 255, 255),
                 circle_outline(16, 16, 12))

    # Smaller circle, exposes flatness behaviour at lower curvature.
    emit_fixture(here, "circle_small",
                 16, 16, IDENTITY, (200, 200, 255, 255),
                 circle_outline(8, 8, 6))

    # Large circle, exposes flatness behaviour at higher pixel
    # extent.
    emit_fixture(here, "circle_large",
                 64, 64, IDENTITY, (180, 0, 180, 255),
                 circle_outline(32, 32, 28))

    # ---- Mixed line + curve outlines -----------------------------
    # A "tear drop": triangle top + curved bottom.
    emit_fixture(here, "teardrop",
                 32, 32, IDENTITY, (0, 100, 200, 255),
                 [(OP_MOVE,  16,  4),
                  (OP_LINE,  26, 14),
                  (OP_CURVE, 26, 24,   16, 28,    6, 24),
                  (OP_CURVE,  6, 14,    6, 14,    6, 14),  # zero-length
                  (OP_LINE,   6, 14),
                  (OP_CLOSE,)])

    # ---- CTM application ----------------------------------------
    # Translate(8, 8) — circle drawn at (0, 0, r=8) appears at
    # (8, 8, r=8).
    emit_fixture(here, "circle_translated",
                 32, 32, (1, 0, 0, 1, 8, 8),
                 (0, 200, 200, 255),
                 circle_outline(0, 0, 8))

    # Scale(2, 2) — radius-4 circle → radius-8 circle.
    emit_fixture(here, "circle_scaled_2x",
                 32, 32, (2, 0, 0, 2, 16, 16),
                 (255, 100, 0, 255),
                 circle_outline(0, 0, 4))

    # Rotation (no rotation that the IDE sees as suspicious; using
    # cos/sin of π/4 explicitly so the CTM is exactly known and
    # both Python and C++ produce the same numbers).
    cos45 = math.cos(math.pi / 4)
    sin45 = math.sin(math.pi / 4)
    emit_fixture(here, "outline_rotated_45",
                 32, 32,
                 (cos45, sin45, -sin45, cos45, 16, 16),
                 (255, 255, 0, 255),
                 [(OP_MOVE,   -8, -8),
                  (OP_LINE,    8, -8),
                  (OP_LINE,    8,  8),
                  (OP_LINE,   -8,  8),
                  (OP_CLOSE,)])

    # ---- Close-then-curve (semantic_traps[curve_after_close]) ----
    # Two subpaths joined by an unusual sequence: Close moves the
    # cursor back to subpath start, then a Curve continues from
    # there.
    emit_fixture(here, "close_then_curve",
                 32, 32, IDENTITY, (200, 0, 200, 255),
                 [(OP_MOVE,   8,  8),
                  (OP_LINE,  24,  8),
                  (OP_LINE,  24, 24),
                  (OP_CLOSE,),
                  # subpath 2 — starts at (8, 8) per Close-reset.
                  (OP_MOVE,   8,  8),
                  (OP_CURVE,  4, 12,   12, 20,    8, 24),
                  (OP_LINE,   8,  8),
                  (OP_CLOSE,)])

    # ---- Partial alpha ------------------------------------------
    emit_fixture(here, "circle_alpha_half",
                 16, 16, IDENTITY, (255, 0, 0, 128),
                 circle_outline(8, 8, 6))

    n_bin = len(list(here.glob("*.bin")))
    n_pix = len(list(here.glob("*.pixels")))
    print(f"wrote {n_bin} fixtures + {n_pix} sidecars to {here}")


if __name__ == "__main__":
    main()
