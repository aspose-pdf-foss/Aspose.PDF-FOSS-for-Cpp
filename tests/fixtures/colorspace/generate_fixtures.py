#!/usr/bin/env python3
"""Generate the .case fixture corpus for foundation::colorspace.

colorspace's freeze gate is componentwise_agreement: each `.case`
file describes a single function call (input_kind + input doubles
+ expected doubles) and the C++ driver compares the primitive's
output to expected within eps_abs.

Two-oracle independence is enforced HERE, at authoring time: every
case is computed via NumPy AND via a closed-form expression
hand-derived from PDF 32000-1:2008 §8.6.4 (or §10.3.5 for the
naïve CMYK→RGB convention). The two values must agree to
ORACLE_EPS = 1e-16 — well below the gate's 1e-15 threshold —
otherwise authoring aborts. Any disagreement is a fixture bug.

Input families for ToSrgb cases:
    DeviceGray  — 1 input component (gray)
    DeviceRGB   — 3 input components (R, G, B)
    DeviceCMYK  — 4 input components (C, M, Y, K)

Plus a synthetic Quantize family for the helper:
    Quantize    — 1 input component (double in [0, 1] or out-of-
                  range), 1 output component (uint8 cast to double
                  in [0, 255])

Fixtures are public-domain — generated deterministically from
this script. Re-run from this directory:

    python3 generate_fixtures.py
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

# Per-component agreement tolerance enforced between the two
# authoring oracles. Tighter than the gate's 1e-15 — the two
# oracles agree to ulp scale on direct arithmetic, and we want
# any real disagreement (typo in closed form) to surface here
# rather than slip through to the gate.
ORACLE_EPS = 1e-16


# ---------------------------------------------------------------------------
# Closed-form oracles — direct PDF 32000-1:2008 §8.6.4 expressions.
# ---------------------------------------------------------------------------


def _clamp01(v):
    """Manual clamp matching the primitive's contract: NaN → 0,
    value < 0 → 0, value > 1 → 1, otherwise the value unchanged.
    The primitive's noexcept Quantize and ToSrgb both implement
    this rule.

    std::clamp(NaN, 0, 1) is implementation-defined; both oracles
    must agree on NaN-handling explicitly."""
    if math.isnan(v):
        return 0.0
    if v < 0.0:
        return 0.0
    if v > 1.0:
        return 1.0
    return v


def closed_to_srgb_gray(components):
    # PDF 32000-1:2008 §8.6.4.2 — DeviceGray: R = G = B = gray.
    g = _clamp01(components[0])
    return [g, g, g]


def closed_to_srgb_rgb(components):
    # PDF 32000-1:2008 §8.6.4.3 — DeviceRGB: identity.
    return [_clamp01(components[0]),
            _clamp01(components[1]),
            _clamp01(components[2])]


def closed_to_srgb_cmyk(components):
    # PDF 32000-1:2008 §8.6.4.4 / §10.3.5 — naïve subtractive
    # conversion. R = (1-C)(1-K), G = (1-M)(1-K), B = (1-Y)(1-K).
    c = _clamp01(components[0])
    m = _clamp01(components[1])
    y = _clamp01(components[2])
    k = _clamp01(components[3])
    return [(1.0 - c) * (1.0 - k),
            (1.0 - m) * (1.0 - k),
            (1.0 - y) * (1.0 - k)]


def closed_to_srgb(family, components):
    if family == "DeviceGray":
        return closed_to_srgb_gray(components)
    if family == "DeviceRGB":
        return closed_to_srgb_rgb(components)
    if family == "DeviceCMYK":
        return closed_to_srgb_cmyk(components)
    raise ValueError(f"unknown family: {family}")


def closed_quantize(value):
    # Round-half-AWAY-from-zero on a [0, 1] input, scaled to [0,
    # 255]. Manual NaN/clamp handling to match the primitive's
    # noexcept contract.
    v = _clamp01(value)
    # int(x + 0.5) is exactly round-half-away-from-zero for the
    # non-negative range of [0, 255].
    return float(int(v * 255.0 + 0.5))


# ---------------------------------------------------------------------------
# NumPy oracle — independent code path.
# ---------------------------------------------------------------------------
#
# Strategy: NumPy uses np.where for the clamp (vectorised
# array-aware NaN handling) and np.subtract / np.multiply for
# the CMYK arithmetic. The control flow and the order of
# operations is genuinely different from the closed-form
# expressions above — they're not the same Python code in two
# spellings.


def _np_clamp01(v):
    arr = np.asarray([v], dtype=np.float64)
    # np.where evaluates both branches; we use it explicitly to
    # exercise NumPy's NaN handling. NaN is not >= 0 nor <= 1, so
    # the "in-range" branch never fires for NaN; the second where
    # then handles >1 (NaN is also not > 1). For NaN we end up
    # returning the original NaN, which we then convert to 0
    # explicitly below to match the primitive's contract.
    arr = np.where(np.isnan(arr), 0.0, arr)
    arr = np.where(arr < 0.0, 0.0, arr)
    arr = np.where(arr > 1.0, 1.0, arr)
    return float(arr[0])


def numpy_to_srgb_gray(components):
    g = _np_clamp01(components[0])
    return list(np.array([g, g, g], dtype=np.float64).tolist())


def numpy_to_srgb_rgb(components):
    return [_np_clamp01(components[0]),
            _np_clamp01(components[1]),
            _np_clamp01(components[2])]


def numpy_to_srgb_cmyk(components):
    cmyk = np.array([_np_clamp01(c) for c in components],
                    dtype=np.float64)
    # Vectorise the subtraction; do the multiply with K via
    # element-wise outer arithmetic.
    inv = 1.0 - cmyk[:3]            # (1-C, 1-M, 1-Y)
    inv_k = 1.0 - cmyk[3]           # (1-K)
    rgb = np.multiply(inv, inv_k)   # element-wise
    return rgb.tolist()


def numpy_to_srgb(family, components):
    if family == "DeviceGray":
        return numpy_to_srgb_gray(components)
    if family == "DeviceRGB":
        return numpy_to_srgb_rgb(components)
    if family == "DeviceCMYK":
        return numpy_to_srgb_cmyk(components)
    raise ValueError(f"unknown family: {family}")


def numpy_quantize(value):
    v = _np_clamp01(value)
    # np.rint uses round-half-to-even (banker's). To match the
    # primitive's round-half-AWAY-from-zero, we add 0.5 then
    # take floor, equivalent to int(x + 0.5) for x ≥ 0.
    scaled = v * 255.0
    rounded = np.floor(scaled + 0.5)
    # Cap at 255 in case round pushes to 256.
    rounded = np.minimum(rounded, 255.0)
    rounded = np.maximum(rounded, 0.0)
    return float(int(rounded))


# ---------------------------------------------------------------------------
# Two-oracle agreement gate.
# ---------------------------------------------------------------------------


def _agree(a, b, label):
    if len(a) != len(b):
        sys.stderr.write(
            f"ORACLE DISAGREEMENT [{label}]: "
            f"len(a)={len(a)} vs len(b)={len(b)}\n")
        return False
    for i, (av, bv) in enumerate(zip(a, b)):
        # NaN-tolerant: both NaN counts as equal (we don't
        # actually emit NaN cases, but the comparison logic is
        # robust).
        if math.isnan(av) and math.isnan(bv):
            continue
        if math.isnan(av) or math.isnan(bv):
            sys.stderr.write(
                f"ORACLE DISAGREEMENT [{label}]: "
                f"component {i}: closed={av!r} numpy={bv!r}\n")
            return False
        if abs(av - bv) > ORACLE_EPS:
            sys.stderr.write(
                f"ORACLE DISAGREEMENT [{label}]: "
                f"component {i}: closed={av!r} numpy={bv!r} "
                f"diff={abs(av - bv)!r}\n")
            return False
    return True


# ---------------------------------------------------------------------------
# Case writer.
# ---------------------------------------------------------------------------


def _fmt_doubles(values):
    return " ".join(f"{v:.17g}" for v in values)


def emit_case(out_dir, name, family, input_values, expected_values):
    """Write a .case file with the standard componentwise_agreement
    grammar: input_kind + input + expected lines."""
    path = out_dir / f"{name}.case"
    body = []
    body.append("# Auto-generated by generate_fixtures.py — do not hand-edit.\n")
    body.append("# Two-oracle agreement: NumPy + PDF 32000 §8.6.4 closed form.\n")
    body.append("\n")
    body.append(f"input_kind {family}\n")
    body.append(f"input {_fmt_doubles(input_values)}\n")
    body.append(f"expected {_fmt_doubles(expected_values)}\n")
    path.write_text("".join(body))


def gen_to_srgb(out_dir, name, family, components):
    closed = closed_to_srgb(family, components)
    npy = numpy_to_srgb(family, components)
    assert _agree(closed, npy, name), \
        f"to_srgb {name} oracle disagreement"
    emit_case(out_dir, name, family, list(components), closed)


def gen_quantize(out_dir, name, value):
    closed = [closed_quantize(value)]
    npy = [numpy_quantize(value)]
    assert _agree(closed, npy, name), \
        f"quantize {name} oracle disagreement"
    emit_case(out_dir, name, "Quantize", [value], closed)


# ---------------------------------------------------------------------------
# Top-level corpus assembly.
# ---------------------------------------------------------------------------


def main():
    here = Path(__file__).resolve().parent
    for stale in here.glob("*.case"):
        stale.unlink()

    # ---- DeviceGray --------------------------------------------
    gen_to_srgb(here, "gray_zero",        "DeviceGray", [0.0])
    gen_to_srgb(here, "gray_half",        "DeviceGray", [0.5])
    gen_to_srgb(here, "gray_one",         "DeviceGray", [1.0])
    gen_to_srgb(here, "gray_quarter",     "DeviceGray", [0.25])
    gen_to_srgb(here, "gray_three_quarter", "DeviceGray", [0.75])
    # Out-of-range clamping (the gray_to_rgb_luminance trap is
    # straight-copy; an implementer applying gamma would fail
    # gray_half because 0.5 → 0.5 under identity but ≈ 0.214
    # under sRGB EOTF^-1).
    gen_to_srgb(here, "gray_negative",    "DeviceGray", [-0.3])
    gen_to_srgb(here, "gray_above_one",   "DeviceGray", [1.7])

    # ---- DeviceRGB --------------------------------------------
    gen_to_srgb(here, "rgb_black",        "DeviceRGB", [0.0, 0.0, 0.0])
    gen_to_srgb(here, "rgb_white",        "DeviceRGB", [1.0, 1.0, 1.0])
    gen_to_srgb(here, "rgb_pure_red",     "DeviceRGB", [1.0, 0.0, 0.0])
    gen_to_srgb(here, "rgb_pure_green",   "DeviceRGB", [0.0, 1.0, 0.0])
    gen_to_srgb(here, "rgb_pure_blue",    "DeviceRGB", [0.0, 0.0, 1.0])
    gen_to_srgb(here, "rgb_mid_gray",     "DeviceRGB", [0.5, 0.5, 0.5])
    gen_to_srgb(here, "rgb_general",      "DeviceRGB", [0.2, 0.7, 0.4])
    # Out-of-range per channel — catches single-channel clamp bugs.
    gen_to_srgb(here, "rgb_negative_red", "DeviceRGB", [-0.5, 0.5, 0.5])
    gen_to_srgb(here, "rgb_above_one_blue", "DeviceRGB", [0.3, 0.3, 1.5])

    # ---- DeviceCMYK -------------------------------------------
    gen_to_srgb(here, "cmyk_white",       "DeviceCMYK", [0.0, 0.0, 0.0, 0.0])  # no ink
    gen_to_srgb(here, "cmyk_pure_black",  "DeviceCMYK", [0.0, 0.0, 0.0, 1.0])  # K=1
    gen_to_srgb(here, "cmyk_pure_red",    "DeviceCMYK", [0.0, 1.0, 1.0, 0.0])  # M+Y
    gen_to_srgb(here, "cmyk_pure_green",  "DeviceCMYK", [1.0, 0.0, 1.0, 0.0])  # C+Y
    gen_to_srgb(here, "cmyk_pure_blue",   "DeviceCMYK", [1.0, 1.0, 0.0, 0.0])  # C+M
    gen_to_srgb(here, "cmyk_pure_cyan",   "DeviceCMYK", [1.0, 0.0, 0.0, 0.0])  # C only
    gen_to_srgb(here, "cmyk_pure_magenta","DeviceCMYK", [0.0, 1.0, 0.0, 0.0])
    gen_to_srgb(here, "cmyk_pure_yellow", "DeviceCMYK", [0.0, 0.0, 1.0, 0.0])
    # K with non-zero C/M/Y — exercises the (1-K) factor.
    gen_to_srgb(here, "cmyk_warm_black",  "DeviceCMYK", [0.0, 0.4, 0.4, 0.8])
    gen_to_srgb(here, "cmyk_dark_red",    "DeviceCMYK", [0.0, 1.0, 1.0, 0.5])
    gen_to_srgb(here, "cmyk_general",     "DeviceCMYK", [0.3, 0.6, 0.2, 0.1])
    # Out-of-range — clamp_after_compute trap.
    gen_to_srgb(here, "cmyk_negative_input", "DeviceCMYK",
                [-0.2, 0.5, 0.5, 0.5])
    gen_to_srgb(here, "cmyk_above_one_input", "DeviceCMYK",
                [0.3, 1.5, 0.3, 0.3])
    # NaN K — must clamp to 0, NOT propagate NaN through the
    # multiplication. A buggy implementation that propagates NaN
    # produces NaN output, which the gate's CompareComponents
    # rejects (NaN diff is unordered).
    gen_to_srgb(here, "cmyk_nan_k",       "DeviceCMYK",
                [0.5, 0.5, 0.5, float("nan")])

    # ---- Quantize ---------------------------------------------
    gen_quantize(here, "quantize_zero",        0.0)
    gen_quantize(here, "quantize_one",         1.0)
    # Round-half-away-from-zero: 0.5/255 → round(0.5) = 1, NOT
    # banker's 0. The fixture name documents the intent.
    gen_quantize(here, "quantize_half_lsb",    0.5 / 255.0)  # = 0.001960784…
    gen_quantize(here, "quantize_three_halves_lsb", 1.5 / 255.0)
    # 0.5 → 127.5 → round-half-away → 128.
    gen_quantize(here, "quantize_half",        0.5)
    # Common 8-bit literal values exercising representative round-
    # trips.
    gen_quantize(here, "quantize_nominal_64",  64.0 / 255.0)
    gen_quantize(here, "quantize_nominal_128", 128.0 / 255.0)
    gen_quantize(here, "quantize_nominal_192", 192.0 / 255.0)
    # Boundary clamping.
    gen_quantize(here, "quantize_negative",   -0.1)
    gen_quantize(here, "quantize_above_one",   1.5)
    # NaN → 0 (per spec convention; safest for "missing channel").
    gen_quantize(here, "quantize_nan",         float("nan"))

    n = len(list(here.glob("*.case")))
    print(f"wrote {n} fixtures to {here}")


if __name__ == "__main__":
    main()
