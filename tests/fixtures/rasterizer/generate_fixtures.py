#!/usr/bin/env python3
"""Generate the .bin / .pixels fixture corpus for foundation::rasterizer.

The rasterizer's freeze gate is lossy_oracle_agreement: each `.bin`
fixture encodes one rasterization request (canvas size + CTM +
path + colour + fill rule) in our internal binary format, and a
sibling `.pixels` sidecar carries the expected RGBA8 buffer.

Single-oracle authoring (matching dctdecode / jpx): the sidecar is
produced by the NumPy super-sampling reference defined below,
which implements the exact algorithm specified in
the project spec's intent block (4×4 centred
sub-pixel grid, left-inclusive / right-exclusive edge convention,
round-half-away-from-zero quantisation, src-over compositing).

Why one oracle. Path rasterization is implementation-defined at
sub-pixel precision: PIL, Cairo, Skia, AGG each pick a different
sub-pixel offset and disagree at PSNR < 25 dB on small canvases
even on simple shapes (we measured this — PIL vs the centred-grid
NumPy reference agrees only at 12–22 dB on the v1 corpus). The
gate's 40 dB PSNR floor absorbs the IEEE-754 rounding drift
between Python and C++ implementations of the SAME algorithm; a
real bug (wrong winding rule, transposed CTM, off-by-one
coverage) produces single or low-double-digit PSNR and is caught.
This is the rendering / under-specified-spec caveat documented
in LibForge-caveats §2 and matches the dctdecode pattern.

Re-run from this directory:

    python3 generate_fixtures.py
"""

from __future__ import annotations

import math
import struct
import sys
from pathlib import Path

import numpy as np


# ---------------------------------------------------------------------------
# Binary fixture format (see the project spec).
# ---------------------------------------------------------------------------
#
# Header (LE):
#   u8 magic=0xCA, u8 version=1, u8 fill_rule, u8 reserved=0,
#   u32 width, u32 height,
#   6×f64 ctm,
#   u8 r, u8 g, u8 b, u8 a,
#   u32 num_segments,
# Then per segment:
#   u8 kind, then payload (Move/Line: 2 doubles; Rect: 4 doubles;
#   Close: nothing). No alignment padding.

MAGIC = 0xCA
VERSION = 1

# Segment kinds (must match the C++ enum)
KIND_MOVE  = 0
KIND_LINE  = 1
KIND_RECT  = 2
KIND_CLOSE = 3

# Fill rules (must match the C++ enum)
FILL_NONZERO = 0
FILL_EVENODD = 1


def encode_fixture(width, height, fill_rule, ctm, color_rgba, segments):
    """Serialize a fixture into the gate's binary format.

    color_rgba: 4-tuple of uint8 (r, g, b, a).
    ctm: 6-tuple of doubles (a, b, c, d, e, f) per PDF row-vector
         convention.
    segments: list of (kind, *coords) tuples.
        Move:  (KIND_MOVE,  x, y)
        Line:  (KIND_LINE,  x, y)
        Rect:  (KIND_RECT,  xmin, ymin, xmax, ymax)
        Close: (KIND_CLOSE,)
    """
    out = bytearray()
    out += struct.pack('<BBBB', MAGIC, VERSION, fill_rule, 0)
    out += struct.pack('<II', width, height)
    out += struct.pack('<6d', *ctm)
    out += struct.pack('<BBBB', *color_rgba)
    out += struct.pack('<I', len(segments))
    for seg in segments:
        kind = seg[0]
        out += struct.pack('<B', kind)
        if kind in (KIND_MOVE, KIND_LINE):
            out += struct.pack('<2d', seg[1], seg[2])
        elif kind == KIND_RECT:
            out += struct.pack('<4d', seg[1], seg[2], seg[3], seg[4])
        elif kind == KIND_CLOSE:
            pass
        else:
            raise ValueError(f"unknown segment kind {kind}")
    return bytes(out)


# ---------------------------------------------------------------------------
# Path → polyline edges (CTM applied).
# ---------------------------------------------------------------------------


def apply_ctm(ctm, x, y):
    """PDF row-vector application: [x y 1] · M = (x', y'). Mirrors
    foundation::transform::Apply."""
    a, b, c, d, e, f = ctm
    return (a * x + c * y + e, b * x + d * y + f)


def flatten_path(segments, ctm):
    """Walk segments, applying CTM, emit a list of edges. Each edge
    is a 4-tuple (x0, y0, x1, y1). Move starts a subpath; Close
    emits a line back to the most recent Move."""
    edges = []
    cur = None
    sub_start = None
    for seg in segments:
        kind = seg[0]
        if kind == KIND_MOVE:
            x, y = apply_ctm(ctm, seg[1], seg[2])
            cur = (x, y)
            sub_start = (x, y)
        elif kind == KIND_LINE:
            if cur is None:
                continue
            x, y = apply_ctm(ctm, seg[1], seg[2])
            edges.append((*cur, x, y))
            cur = (x, y)
        elif kind == KIND_RECT:
            xmin, ymin, xmax, ymax = seg[1], seg[2], seg[3], seg[4]
            corners = [
                apply_ctm(ctm, xmin, ymin),
                apply_ctm(ctm, xmax, ymin),
                apply_ctm(ctm, xmax, ymax),
                apply_ctm(ctm, xmin, ymax),
            ]
            for i in range(4):
                a = corners[i]
                b = corners[(i + 1) % 4]
                edges.append((*a, *b))
            # Rect's ending current point is its origin (per the PDF
            # `re` operator's documented contract). cur and sub_start
            # update to that.
            cur = corners[0]
            sub_start = corners[0]
        elif kind == KIND_CLOSE:
            if cur is not None and sub_start is not None and cur != sub_start:
                edges.append((*cur, *sub_start))
            cur = sub_start
    return edges


# ---------------------------------------------------------------------------
# Oracle 1: NumPy super-sampling reference.
# ---------------------------------------------------------------------------


def winding_at_point(edges, px, py):
    """Compute the winding number of the edge set at point (px, py).
    Standard non-zero winding rule:
        for each edge (x0, y0) → (x1, y1):
            if y0 <= py < y1:  +1 if px is left of the edge
            if y1 <= py < y0:  -1 if px is left of the edge
    The "left of the edge" check uses the cross product to handle
    sloped edges; horizontal edges contribute zero.
    """
    w = 0
    for (x0, y0, x1, y1) in edges:
        if y0 == y1:
            # Horizontal edge contributes zero winding.
            continue
        if y0 <= py < y1:
            # Upward edge crossing scanline.
            t = (py - y0) / (y1 - y0)
            x_cross = x0 + t * (x1 - x0)
            if px > x_cross:
                w += 1
        elif y1 <= py < y0:
            t = (py - y0) / (y1 - y0)
            x_cross = x0 + t * (x1 - x0)
            if px > x_cross:
                w -= 1
    return w


def render_numpy(width, height, fill_rule, edges, color_rgba):
    """Reference renderer. 4×4 centred sub-pixel grid per device
    pixel; coverage = (count_inside) / 16. src-over composite of
    color_rgba * coverage onto a transparent background.

    Output: H×W×4 uint8 array (RGBA, row-major).
    """
    raster = np.zeros((height, width, 4), dtype=np.uint8)
    src_r, src_g, src_b, src_a = color_rgba
    src_a_norm = src_a / 255.0

    # Precompute sub-pixel offsets.
    subs = [(i + 0.5) / 4.0 for i in range(4)]

    for py in range(height):
        for px in range(width):
            count_inside = 0
            for j in subs:
                sy = py + j
                for i in subs:
                    sx = px + i
                    w = winding_at_point(edges, sx, sy)
                    if fill_rule == FILL_NONZERO:
                        inside = (w != 0)
                    else:  # EvenOdd
                        inside = (w % 2 != 0)
                    if inside:
                        count_inside += 1
            coverage = count_inside / 16.0
            if coverage == 0.0:
                continue
            effective_alpha = src_a_norm * coverage
            # src-over with linear (non-premultiplied) source.
            dst = raster[py, px]
            dst_r_n = dst[0] / 255.0
            dst_g_n = dst[1] / 255.0
            dst_b_n = dst[2] / 255.0
            dst_a_n = dst[3] / 255.0

            out_r = src_r * effective_alpha / 255.0 + dst_r_n * (1 - effective_alpha)
            out_g = src_g * effective_alpha / 255.0 + dst_g_n * (1 - effective_alpha)
            out_b = src_b * effective_alpha / 255.0 + dst_b_n * (1 - effective_alpha)
            out_a = effective_alpha + dst_a_n * (1 - effective_alpha)

            raster[py, px, 0] = quantize(out_r)
            raster[py, px, 1] = quantize(out_g)
            raster[py, px, 2] = quantize(out_b)
            raster[py, px, 3] = quantize(out_a)
    return raster


def quantize(v):
    """round-half-AWAY-from-zero, match foundation::colorspace::Quantize."""
    if math.isnan(v):
        return 0
    if v <= 0.0:
        return 0
    if v >= 1.0:
        return 255
    return int(v * 255.0 + 0.5)


# ---------------------------------------------------------------------------
# Per-fixture authoring (single-oracle).
# ---------------------------------------------------------------------------


def emit_fixture(out_dir, name, width, height, fill_rule, ctm,
                 color_rgba, segments):
    """Render via the NumPy reference, emit `.bin` + `.pixels`."""
    edges = flatten_path(segments, ctm)
    pixels = render_numpy(width, height, fill_rule, edges, color_rgba)
    bin_bytes = encode_fixture(width, height, fill_rule, ctm,
                               color_rgba, segments)
    (out_dir / f"{name}.bin").write_bytes(bin_bytes)
    (out_dir / f"{name}.pixels").write_bytes(pixels.tobytes())


# ---------------------------------------------------------------------------
# Top-level corpus assembly.
# ---------------------------------------------------------------------------


IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)


def main():
    here = Path(__file__).resolve().parent
    for stale in here.glob("*.bin"):
        stale.unlink()
    for stale in here.glob("*.pixels"):
        stale.unlink()

    # ---- Empty / trivial -----------------------------------------
    emit_fixture(here, "empty_canvas",
                 16, 16, FILL_NONZERO, IDENTITY, (255, 0, 0, 255),
                 [])

    # Solid fill across the whole canvas — coverage = 1 everywhere.
    emit_fixture(here, "solid_fill_full",
                 16, 16, FILL_NONZERO, IDENTITY, (255, 255, 255, 255),
                 [(KIND_RECT, 0, 0, 16, 16)])

    # ---- Axis-aligned rectangles ---------------------------------
    emit_fixture(here, "rect_axis_aligned",
                 32, 32, FILL_NONZERO, IDENTITY, (0, 128, 255, 255),
                 [(KIND_RECT, 4, 4, 28, 28)])

    emit_fixture(here, "rect_subpixel_aligned",
                 32, 32, FILL_NONZERO, IDENTITY, (0, 200, 0, 255),
                 [(KIND_RECT, 4.5, 4.25, 27.75, 27.5)])

    emit_fixture(here, "rect_color_red",
                 16, 16, FILL_NONZERO, IDENTITY, (255, 0, 0, 255),
                 [(KIND_RECT, 2, 2, 14, 14)])

    emit_fixture(here, "rect_color_blue",
                 16, 16, FILL_NONZERO, IDENTITY, (0, 0, 255, 255),
                 [(KIND_RECT, 2, 2, 14, 14)])

    # ---- Triangles -----------------------------------------------
    emit_fixture(here, "triangle_simple",
                 32, 32, FILL_NONZERO, IDENTITY, (255, 200, 0, 255),
                 [(KIND_MOVE,  4,  4),
                  (KIND_LINE, 28,  4),
                  (KIND_LINE, 16, 28),
                  (KIND_CLOSE,)])

    emit_fixture(here, "triangle_thin",
                 32, 32, FILL_NONZERO, IDENTITY, (200, 0, 200, 255),
                 [(KIND_MOVE,  4,  4),
                  (KIND_LINE, 28, 12),
                  (KIND_LINE, 16, 16),
                  (KIND_CLOSE,)])

    # ---- Adjacent rects (edge inclusion / exclusion check) -------
    emit_fixture(here, "adjacent_rects",
                 32, 32, FILL_NONZERO, IDENTITY, (128, 128, 128, 255),
                 [(KIND_RECT, 4, 4, 16, 28),
                  (KIND_RECT, 16, 4, 28, 28)])

    # ---- Pentagram (NonZero vs EvenOdd diverge in centre) --------
    # 5 outer points connected in star order (skipping every other
    # vertex). The path crosses itself, so the inner pentagon has
    # winding number 2: NonZero fills it, EvenOdd does not.
    outer_r = 12.0
    centres = []
    for k in range(5):
        ang = -math.pi / 2 + k * 2 * math.pi / 5
        centres.append((16 + outer_r * math.cos(ang),
                        16 + outer_r * math.sin(ang)))
    # Connect outer points in pentagram order: 0, 2, 4, 1, 3
    pentagram_order = [centres[i] for i in (0, 2, 4, 1, 3)]
    star_segments = [(KIND_MOVE, *pentagram_order[0])]
    for p in pentagram_order[1:]:
        star_segments.append((KIND_LINE, *p))
    star_segments.append((KIND_CLOSE,))

    emit_fixture(here, "star_nonzero",
                 32, 32, FILL_NONZERO, IDENTITY, (255, 255, 255, 255),
                 star_segments)
    emit_fixture(here, "star_evenodd",
                 32, 32, FILL_EVENODD, IDENTITY, (255, 255, 255, 255),
                 star_segments)

    # ---- Self-overlapping figure-8 (winding number > 1) ---------
    # Two squares sharing a centre, both wound CCW — combined
    # winding = 2 in the overlap. NonZero fills overlap; EvenOdd
    # does not.
    fig8_segments = [
        (KIND_RECT, 4, 4, 20, 20),
        (KIND_RECT, 12, 12, 28, 28),
    ]
    emit_fixture(here, "self_overlap_nonzero",
                 32, 32, FILL_NONZERO, IDENTITY, (255, 100, 100, 255),
                 fig8_segments)
    emit_fixture(here, "self_overlap_evenodd",
                 32, 32, FILL_EVENODD, IDENTITY, (255, 100, 100, 255),
                 fig8_segments)

    # ---- CTM application ----------------------------------------
    # Translate(8, 8) — rect drawn at (0..16, 0..16) appears at
    # (8..24, 8..24).
    emit_fixture(here, "rect_translated",
                 32, 32, FILL_NONZERO, (1, 0, 0, 1, 8, 8),
                 (0, 200, 200, 255),
                 [(KIND_RECT, 0, 0, 16, 16)])

    # Scale(2, 2) — 8×8 unit rect becomes 16×16 device rect.
    emit_fixture(here, "rect_scaled_2x",
                 32, 32, FILL_NONZERO, (2, 0, 0, 2, 4, 4),
                 (200, 200, 0, 255),
                 [(KIND_RECT, 0, 0, 8, 8)])

    # ---- Compositing partial alpha onto a background -------------
    # We can't pre-fill the gate's input raster (it's freshly
    # zeroed by Decode), so this fixture emits a SINGLE shape
    # with alpha=128. Full alpha=255 fixtures already exist
    # above; this exercises the partial-alpha path of src-over.
    emit_fixture(here, "rect_alpha_half",
                 16, 16, FILL_NONZERO, IDENTITY, (255, 0, 0, 128),
                 [(KIND_RECT, 2, 2, 14, 14)])

    # ---- Out-of-bounds path (clipped by canvas, not by code) ----
    emit_fixture(here, "rect_out_of_bounds",
                 16, 16, FILL_NONZERO, IDENTITY, (0, 255, 0, 255),
                 [(KIND_RECT, -4, -4, 8, 8)])

    # ---- Degenerate (zero area) ---------------------------------
    # Three collinear points → no area to fill.
    emit_fixture(here, "degenerate_collinear",
                 16, 16, FILL_NONZERO, IDENTITY, (255, 0, 0, 255),
                 [(KIND_MOVE, 2, 8),
                  (KIND_LINE, 14, 8),
                  (KIND_LINE, 8, 8),
                  (KIND_CLOSE,)])

    n_bin = len(list(here.glob("*.bin")))
    n_pixels = len(list(here.glob("*.pixels")))
    print(f"wrote {n_bin} fixtures + {n_pixels} sidecars to {here}")


if __name__ == "__main__":
    main()
