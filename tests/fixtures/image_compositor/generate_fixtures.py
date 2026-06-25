#!/usr/bin/env python3
"""Generate the .bin / .pixels fixture corpus for foundation::image_compositor.

Single-oracle authoring (matches rasterizer / glyph_rasterizer /
dctdecode / jpx): the sidecar is produced by a NumPy reference that
implements the EXACT algorithm specified in
the project spec — inverse-CTM, 4×4 centred
sub-pixel grid, nearest-neighbour source lookup with clamp,
divide-by-16 averaging, src-over composite, round-half-away-from-zero
quantisation.

The committed `.pixels` IS this oracle's output; the gate measures
the C++ adapter against the algorithm we specified. Real-library
agreement (PIL / Cairo / Skia) at PSNR ≥ 40 dB is not guaranteed
even for nearest-neighbour with an axis-aligned identity CTM —
edge handling and sub-pixel offset differ. See LibForge-caveats §2
for the under-determined-spec rationale.

Re-run from this directory:

    python3 generate_fixtures.py
"""

from __future__ import annotations

import math
import struct
from pathlib import Path

import numpy as np


# ---------------------------------------------------------------------------
# Binary fixture format (see image_compositor.yaml).
# ---------------------------------------------------------------------------
#
# Header (LE-packed, no alignment padding):
#   u8  magic = 0xCC, u8 version = 1, u8 reserved = 0, u8 reserved = 0,
#   u32 dst_width, u32 dst_height,
#   u32 src_width, u32 src_height,
#   6×f64 ctm,                              48 bytes
#   f64 alpha,                              8 bytes
#   src_width*src_height*4 bytes RGBA8 source pixels.

MAGIC = 0xCC
VERSION = 1


def encode_fixture(dst_w, dst_h, src_w, src_h, ctm, alpha, src_pixels_bytes):
    """Serialize a fixture into the gate's binary format.

    src_pixels_bytes: bytes of length src_w*src_h*4, RGBA8 row-major.
    ctm: 6-tuple of doubles (a, b, c, d, e, f).
    alpha: float in [0, 1].
    """
    if len(src_pixels_bytes) != src_w * src_h * 4:
        raise ValueError(
            f"src pixels length {len(src_pixels_bytes)} != "
            f"{src_w}*{src_h}*4 = {src_w*src_h*4}")
    out = bytearray()
    out += struct.pack('<BBBB', MAGIC, VERSION, 0, 0)
    out += struct.pack('<II', dst_w, dst_h)
    out += struct.pack('<II', src_w, src_h)
    out += struct.pack('<6d', *ctm)
    out += struct.pack('<d', alpha)
    out += src_pixels_bytes
    return bytes(out)


# ---------------------------------------------------------------------------
# Matrix inverse — mirrors foundation::transform::Inverse exactly.
# ---------------------------------------------------------------------------


def matrix_inverse(ctm):
    """PDF row-vector affine matrix inverse. Returns None if singular.

    For matrix M = (a, b, c, d, e, f) representing
        | a  b  0 |
        | c  d  0 |
        | e  f  1 |
    the inverse is computed by the standard 2×2 inverse for the
    linear part plus translation re-projection. Mirrors
    foundation::transform::Inverse: rejects NaN-bearing input
    (det is NaN, fails the != 0 check) and singular matrices
    (det == 0).
    """
    a, b, c, d, e, f = ctm
    det = a * d - b * c
    # Match foundation::transform::Inverse exactly: reject NaN and any
    # det whose magnitude falls below the documented 1e-12 floor (a
    # det of 5e-15 represents a numerically-singular matrix even
    # though Python's `det == 0.0` would say otherwise).
    if math.isnan(det) or abs(det) < 1e-12:
        return None
    inv_det = 1.0 / det
    inv_a = d * inv_det
    inv_b = -b * inv_det
    inv_c = -c * inv_det
    inv_d = a * inv_det
    # Translation: the row-vector convention places (e, f) in the
    # bottom row, so the inverse translation is -(e, f) transformed
    # by the inverse linear part.
    inv_e = -(e * inv_a + f * inv_c)
    inv_f = -(e * inv_b + f * inv_d)
    return (inv_a, inv_b, inv_c, inv_d, inv_e, inv_f)


def apply_ctm(ctm, x, y):
    """PDF row-vector application: [x y 1] · M = (x', y')."""
    a, b, c, d, e, f = ctm
    return (a * x + c * y + e, b * x + d * y + f)


# ---------------------------------------------------------------------------
# Quantisation (matches foundation::colorspace::Quantize).
# ---------------------------------------------------------------------------


def quantize(v):
    """round-half-AWAY-from-zero, NaN → 0, clamp to [0, 255]."""
    if math.isnan(v):
        return 0
    if v <= 0.0:
        return 0
    if v >= 1.0:
        return 255
    return int(v * 255.0 + 0.5)


# ---------------------------------------------------------------------------
# NumPy reference — the spec'd algorithm, line by line.
# ---------------------------------------------------------------------------


def composite_numpy(dst_w, dst_h, src_w, src_h, ctm, alpha, src_pixels):
    """Reference compositor.

    src_pixels: H_src × W_src × 4 uint8 array.
    Returns: H_dst × W_dst × 4 uint8 array (destination starts
    freshly zeroed, transparent black).
    """
    dst = np.zeros((dst_h, dst_w, 4), dtype=np.uint8)
    inv = matrix_inverse(ctm)
    if inv is None:
        return dst  # singular CTM → no-op
    if dst_w == 0 or dst_h == 0:
        return dst
    if src_w == 0 or src_h == 0:
        return dst

    # Clamp global alpha exactly the way colorspace::Quantize would.
    if math.isnan(alpha) or alpha < 0.0:
        alpha_clamped = 0.0
    elif alpha > 1.0:
        alpha_clamped = 1.0
    else:
        alpha_clamped = alpha

    sub_offsets = [(i + 0.5) / 4.0 for i in range(4)]

    for py in range(dst_h):
        for px in range(dst_w):
            sum_r = 0.0
            sum_g = 0.0
            sum_b = 0.0
            sum_a = 0.0
            for jo in sub_offsets:
                sy = py + jo
                for io in sub_offsets:
                    sx = px + io
                    u, v = apply_ctm(inv, sx, sy)
                    if 0.0 <= u < 1.0 and 0.0 <= v < 1.0:
                        u_pix = int(u * src_w)
                        v_pix = int(v * src_h)
                        # Drift-safety clamp matching the C++ adapter.
                        if u_pix < 0:
                            u_pix = 0
                        elif u_pix >= src_w:
                            u_pix = src_w - 1
                        if v_pix < 0:
                            v_pix = 0
                        elif v_pix >= src_h:
                            v_pix = src_h - 1
                        s = src_pixels[v_pix, u_pix]
                        sum_r += s[0] / 255.0
                        sum_g += s[1] / 255.0
                        sum_b += s[2] / 255.0
                        sum_a += s[3] / 255.0
                    # else: outside the unit rect → contributes zero
                    # to BOTH numerator AND alpha.

            avg_r = sum_r / 16.0
            avg_g = sum_g / 16.0
            avg_b = sum_b / 16.0
            avg_a = sum_a / 16.0

            eff_a = avg_a * alpha_clamped
            if eff_a == 0.0:
                continue  # nothing to composite, leave dst as-is

            d = dst[py, px]
            dr = d[0] / 255.0
            dg = d[1] / 255.0
            db = d[2] / 255.0
            da = d[3] / 255.0
            inv_a = 1.0 - eff_a

            # avg_* are coverage-premultiplied (sum / 16), so the
            # source colour composites with the GLOBAL alpha only;
            # multiplying by eff_a would double-count coverage and
            # darken partial-coverage edges. inv_a = 1 - avg_a*alpha
            # is the correct src-over weight for the dst term.
            out_r = avg_r * alpha_clamped + dr * inv_a
            out_g = avg_g * alpha_clamped + dg * inv_a
            out_b = avg_b * alpha_clamped + db * inv_a
            out_a = eff_a + da * inv_a

            dst[py, px, 0] = quantize(out_r)
            dst[py, px, 1] = quantize(out_g)
            dst[py, px, 2] = quantize(out_b)
            dst[py, px, 3] = quantize(out_a)
    return dst


# ---------------------------------------------------------------------------
# Per-fixture authoring.
# ---------------------------------------------------------------------------


def emit_fixture(out_dir, name, dst_w, dst_h, src_pixels, ctm, alpha):
    """Render via the NumPy reference, emit `.bin` + `.pixels`."""
    src_h, src_w = src_pixels.shape[0], src_pixels.shape[1]
    if src_pixels.shape[2] != 4 or src_pixels.dtype != np.uint8:
        raise ValueError("src_pixels must be H×W×4 uint8")
    src_bytes = src_pixels.tobytes()
    rendered = composite_numpy(dst_w, dst_h, src_w, src_h, ctm, alpha,
                               src_pixels)
    bin_bytes = encode_fixture(dst_w, dst_h, src_w, src_h, ctm, alpha,
                               src_bytes)
    (out_dir / f"{name}.bin").write_bytes(bin_bytes)
    (out_dir / f"{name}.pixels").write_bytes(rendered.tobytes())


# ---------------------------------------------------------------------------
# Source-image builders.
# ---------------------------------------------------------------------------


def solid_rgba(w, h, rgba):
    """Constant-colour source, RGBA8."""
    img = np.zeros((h, w, 4), dtype=np.uint8)
    img[:, :, 0] = rgba[0]
    img[:, :, 1] = rgba[1]
    img[:, :, 2] = rgba[2]
    img[:, :, 3] = rgba[3]
    return img


def checkerboard(w, h, cell, c_a, c_b):
    """A two-colour checkerboard. Tests that source-pixel sampling
    actually walks both axes (a row-major / column-major mix-up
    would scramble the pattern visibly)."""
    img = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        for x in range(w):
            cx = (x // cell) % 2
            cy = (y // cell) % 2
            c = c_a if (cx ^ cy) == 0 else c_b
            img[y, x] = c
    return img


def horizontal_stripes(w, h, c_top, c_bot):
    """Top half c_top, bottom half c_bot."""
    img = np.zeros((h, w, 4), dtype=np.uint8)
    half = h // 2
    for y in range(h):
        c = c_top if y < half else c_bot
        img[y, :] = c
    return img


def vertical_stripes(w, h, c_left, c_right):
    """Left half c_left, right half c_right."""
    img = np.zeros((h, w, 4), dtype=np.uint8)
    half = w // 2
    for x in range(w):
        c = c_left if x < half else c_right
        img[:, x] = c
    return img


def gradient(w, h):
    """A ramp from (0,0,0,255) at (0,0) to (255,255,255,255) at the
    far corner. Forces colour interpolation to be consistent with
    coordinate orientation — a transposed CTM produces a visibly
    rotated gradient."""
    img = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        for x in range(w):
            t = (x + y) / max(1, (w - 1) + (h - 1))
            v = quantize(t)
            img[y, x] = (v, v, v, 255)
    return img


# ---------------------------------------------------------------------------
# Top-level corpus assembly.
# ---------------------------------------------------------------------------


def main():
    here = Path(__file__).resolve().parent
    for stale in here.glob("*.bin"):
        stale.unlink()
    for stale in here.glob("*.pixels"):
        stale.unlink()

    # ---- Empty / trivial -------------------------------------------------
    # Singular CTM (collapses x to zero) — Composite must no-op.
    emit_fixture(here, "image_singular_ctm",
                 8, 8,
                 solid_rgba(4, 4, (255, 0, 0, 255)),
                 (0.0, 0.0, 0.0, 8.0, 0.0, 0.0),    # det = 0
                 1.0)

    # ---- Axis-aligned 1:1 mapping ---------------------------------------
    # Source 8×8 mapped exactly onto destination 8×8 via the unit-rect
    # CTM. Every destination pixel reads exactly one source pixel — the
    # output should be a near-perfect copy. Tests the right/bottom-edge
    # clamp at the boundary (catches off-by-one).
    emit_fixture(here, "image_axis_aligned",
                 8, 8,
                 checkerboard(8, 8, 1,
                              (255, 0, 0, 255),
                              (0, 0, 255, 255)),
                 (8.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 1.0)

    # Same shape, larger source — exercises nearest-neighbour
    # downsampling. Source 16×16 mapped onto 8×8 destination → each
    # destination pixel averages 4 source samples by virtue of the
    # 4×4 sub-pixel grid.
    emit_fixture(here, "image_downsample_2x",
                 8, 8,
                 checkerboard(16, 16, 1,
                              (255, 200, 0, 255),
                              (0, 100, 200, 255)),
                 (8.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 1.0)

    # Upsample: 4×4 source mapped onto 16×16 destination. Each source
    # pixel covers a 4×4 destination block; nearest-neighbour produces
    # sharp blocks.
    emit_fixture(here, "image_upsample_4x",
                 16, 16,
                 checkerboard(4, 4, 1,
                              (255, 255, 255, 255),
                              (0, 0, 0, 255)),
                 (16.0, 0.0, 0.0, 16.0, 0.0, 0.0),
                 1.0)

    # ---- Translation -----------------------------------------------------
    # Source painted onto the right half of the destination. Tests that
    # destination pixels OUTSIDE the unit-rect mapping are not touched
    # (they remain transparent black, i.e. (0,0,0,0)).
    emit_fixture(here, "image_translated",
                 16, 8,
                 solid_rgba(8, 8, (0, 200, 0, 255)),
                 (8.0, 0.0, 0.0, 8.0, 8.0, 0.0),
                 1.0)

    # Half-overlap at the destination's right edge: source straddles
    # the boundary. The trap "outside_subsample_alpha_credit" target —
    # destination pixels at the cut edge should get reduced alpha
    # because some sub-samples fall outside the unit rect.
    emit_fixture(here, "image_translated_half",
                 16, 8,
                 solid_rgba(8, 8, (255, 0, 255, 255)),
                 (8.0, 0.0, 0.0, 8.0, 12.0, 0.0),  # right half off-canvas
                 1.0)

    # ---- Scaling ---------------------------------------------------------
    emit_fixture(here, "image_scaled_2x",
                 16, 16,
                 horizontal_stripes(8, 8,
                                    (255, 0, 0, 255),
                                    (0, 0, 255, 255)),
                 (16.0, 0.0, 0.0, 16.0, 0.0, 0.0),
                 1.0)

    # Asymmetric scale catches an axis-swap CTM bug. A 4×4 source
    # painted as a wide-and-short image (a, b, c, d, e, f) =
    # (16, 0, 0, 4, 0, 6). The vertical-stripes pattern in source
    # space appears as wide vertical bands in destination.
    emit_fixture(here, "image_scaled_asymmetric",
                 16, 16,
                 vertical_stripes(4, 4,
                                  (200, 200, 0, 255),
                                  (0, 200, 200, 255)),
                 (16.0, 0.0, 0.0, 4.0, 0.0, 6.0),
                 1.0)

    # ---- Rotation --------------------------------------------------------
    # 45° rotation centred on the destination's middle. Source 8×8
    # gradient becomes a rotated diamond. Catches a transposed CTM
    # (semantic_traps.ctm_direction_swap) AND a bytes/transposed
    # source-pixel sampling (semantic_traps.row_major_byte_order).
    cos45 = math.cos(math.pi / 4)
    sin45 = math.sin(math.pi / 4)
    s = 8.0
    # Compose: translate(8,8) ∘ rotate(45°) ∘ scale(s,s) ∘ translate(-0.5,-0.5)
    # In row-vector form, [u v 1] · M_total maps unit-space → device.
    # We build M_total = M_uniscale_to_centred ∘ M_rotate ∘ M_translate
    # by hand:
    #   centred-source: shift unit rect [0,1]² → [-0.5, 0.5]²
    #     T1 = (1, 0, 0, 1, -0.5, -0.5)
    #   scale uniformly by s:
    #     S = (s, 0, 0, s, 0, 0)
    #   rotate 45°:
    #     R = (cos, sin, -sin, cos, 0, 0)
    #   translate to centre of dst:
    #     T2 = (1, 0, 0, 1, 8, 8)
    # Composed (T1 then S then R then T2) — apply Compose once at a
    # time:
    def compose(lhs, rhs):
        a1, b1, c1, d1, e1, f1 = lhs
        a2, b2, c2, d2, e2, f2 = rhs
        # Same multiplication order as foundation::transform::Compose:
        # the right-hand matrix is applied AFTER the left.
        return (
            a1 * a2 + b1 * c2,
            a1 * b2 + b1 * d2,
            c1 * a2 + d1 * c2,
            c1 * b2 + d1 * d2,
            e1 * a2 + f1 * c2 + e2,
            e1 * b2 + f1 * d2 + f2,
        )
    M = compose((1.0, 0.0, 0.0, 1.0, -0.5, -0.5),
                (s, 0.0, 0.0, s, 0.0, 0.0))
    M = compose(M, (cos45, sin45, -sin45, cos45, 0.0, 0.0))
    M = compose(M, (1.0, 0.0, 0.0, 1.0, 8.0, 8.0))
    emit_fixture(here, "image_rotated_45",
                 16, 16,
                 gradient(8, 8),
                 M,
                 1.0)

    # ---- Oblong source (row-major byte-order trap) ----------------------
    # 32-wide, 8-tall source painted onto a 32×8 destination. A
    # column-major / row-major mix-up rotates the result 90°.
    emit_fixture(here, "image_oblong",
                 32, 8,
                 horizontal_stripes(32, 8,
                                    (255, 100, 0, 255),
                                    (0, 100, 255, 255)),
                 (32.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 1.0)

    # ---- Alpha + compositing -------------------------------------------
    # Source has its OWN alpha (semi-transparent red). Destination
    # starts fully transparent so the composite produces avg_a = 0.5
    # at every covered pixel.
    emit_fixture(here, "image_alpha_half",
                 8, 8,
                 solid_rgba(8, 8, (255, 0, 0, 128)),
                 (8.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 1.0)

    # Source fully opaque, but global alpha multiplier = 0.5. Should
    # produce exactly the same output as image_alpha_half (modulo
    # sub-byte quantisation). Catches "ignore the alpha argument"
    # bugs.
    emit_fixture(here, "image_global_alpha_half",
                 8, 8,
                 solid_rgba(8, 8, (255, 0, 0, 255)),
                 (8.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 0.5)

    # Global alpha = 0 → composite is a no-op.
    emit_fixture(here, "image_global_alpha_zero",
                 8, 8,
                 solid_rgba(8, 8, (255, 0, 0, 255)),
                 (8.0, 0.0, 0.0, 8.0, 0.0, 0.0),
                 0.0)

    # ---- Out-of-canvas placement ---------------------------------------
    # CTM places the source partly outside the destination canvas. The
    # off-canvas region is silently skipped (the destination loop
    # bounds clamp); the on-canvas region renders normally.
    emit_fixture(here, "image_out_of_bounds",
                 16, 16,
                 solid_rgba(8, 8, (0, 255, 255, 255)),
                 (8.0, 0.0, 0.0, 8.0, -4.0, -4.0),
                 1.0)

    # ---- Empty source / destination ------------------------------------
    # Zero-sized source: all sub-samples fall outside (the unit rect
    # is empty), avg_a = 0, dst stays all-zero.
    # NumPy would barf on a 0×0 array, so emit a 1-wide source with
    # src_w=0 in the encoded fixture — but the C++ no-op runs before
    # any allocation so the fixture's pixel buffer is empty.
    emit_fixture(here, "image_empty_dst",
                 0, 0,
                 solid_rgba(4, 4, (255, 0, 0, 255)),
                 (4.0, 0.0, 0.0, 4.0, 0.0, 0.0),
                 1.0)

    n_bin = len(list(here.glob("*.bin")))
    n_pix = len(list(here.glob("*.pixels")))
    print(f"wrote {n_bin} fixtures + {n_pix} sidecars to {here}")


if __name__ == "__main__":
    main()
