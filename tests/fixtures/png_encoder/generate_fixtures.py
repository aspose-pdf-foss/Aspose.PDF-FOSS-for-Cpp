#!/usr/bin/env python3
"""Generate the .pencfix fixture corpus for foundation::png_encoder.

Each fixture is a self-describing binary blob bundling
(color_type, page) into one file. The per-target dispatch
sibling (PngEncoderDispatch.cs) parses the blob, calls
PngEncoder.Encode with the embedded options + RGBA page, then
calls PngDecoder.Decode on the encoded PNG and returns one
EncodeRoundtripResult where ExpectedPixels = the input RGBA8
and DecodedPixels = the bytes PngDecoder produced.

Wire format documented in the project spec
(``fixture_format:`` field). All multi-byte integers are
little-endian. Single page per fixture (PNG has no multi-image
container concept).

The gate runs in byte-equality mode (PNG is lossless on every
in-scope color type). For ColorType=Grayscale and Rgb the
input alpha channel must be 0xFF since the encoder drops alpha
on those paths and the decoder widens to alpha=0xFF on read —
non-0xFF alpha would survive round-trip only on ColorType=Rgba.
For ColorType=Grayscale the input must satisfy R == G == B at
every pixel (the encoder errors otherwise).

Filename convention:
  <name>.<color_type>.pencfix

  <color_type> ∈ {gray, rgb, rgba}

The redundant filename encoding lets a human reading a corpus
listing see the option matrix at a glance; the canonical
options are still the in-file header bytes.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path


# PNG IHDR color_type byte values — passed through unchanged
# in the .pencfix wire byte.
_COLOR_TYPE_GRAY = 0
_COLOR_TYPE_RGB = 2
_COLOR_TYPE_RGBA = 6


# ---------------------------------------------------------------------------
# Source-image authoring (raw RGBA8 buffers)
# ---------------------------------------------------------------------------


def _gradient_rgba(w: int, h: int) -> bytes:
    """RGBA gradient. R varies horizontally, G varies vertically,
    B = (R + G) // 2, A varies horizontally. The alpha channel
    varies non-trivially — only suitable for ColorType=Rgba
    fixtures."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            r = (255 * x) // max(1, w - 1)
            g = (255 * y) // max(1, h - 1)
            b = (r + g) // 2
            a = 255 - r // 2
            i = (y * w + x) * 4
            out[i:i + 4] = bytes((r, g, b, a))
    return bytes(out)


def _checker_rgb_opaque(w: int, h: int, cell: int) -> bytes:
    """RGBA checkerboard with alpha=0xFF. Suitable for
    ColorType=Rgb or Rgba (Rgb path drops alpha; the round-
    trip widens back to 0xFF which matches the input)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            i = (y * w + x) * 4
            out[i:i + 4] = bytes(
                (255, 128, 0, 255) if on else (0, 128, 255, 255))
    return bytes(out)


def _gradient_gray_opaque(w: int, h: int) -> bytes:
    """RGBA buffer where R == G == B varies horizontally and
    alpha=0xFF. Suitable for ColorType=Grayscale (the encoder
    requires R == G == B; the round-trip widens grayscale back
    to RGBA with R=G=B=Y, A=0xFF which matches the input)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            v = (255 * x) // max(1, w - 1)
            i = (y * w + x) * 4
            out[i:i + 4] = bytes((v, v, v, 255))
    return bytes(out)


def _solid_rgba_opaque(w: int, h: int, rgb: tuple[int, int, int]) -> bytes:
    """Solid RGBA with alpha=0xFF — exercises a degenerate
    encoder input (zero-variance scanlines) for any color type."""
    return bytes((*rgb, 255)) * (w * h)


# ---------------------------------------------------------------------------
# .pencfix writer
# ---------------------------------------------------------------------------


def _pack_pencfix(color_type: int, width: int, height: int,
                  rgba: bytes) -> bytes:
    """Pack one .pencfix v1 bundle. Header layout per
    the project spec fixture_format."""
    if len(rgba) != width * height * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {width}*{height}*4 "
            f"= {width * height * 4}")
    if color_type not in (_COLOR_TYPE_GRAY, _COLOR_TYPE_RGB,
                          _COLOR_TYPE_RGBA):
        raise ValueError(f"color_type {color_type} not in 0/2/6")

    header = struct.pack("<4sBBBB",
                         b"PENC",
                         1,           # version
                         color_type,
                         0, 0)        # reserved
    page = struct.pack("<II", width, height) + rgba
    return header + page


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


# (filename, color_type, source_factory)
_FIXTURES = [
    # RGBA pass-through — exercises the simplest encoder path
    # (no sample reduction). Gradient covers varying R / G / B
    # / A values across the image.
    ("gradient.rgba.pencfix", _COLOR_TYPE_RGBA,
     lambda: _gradient_rgba(16, 16)),

    # RGB drop-alpha — alpha is 0xFF in the source so the
    # round-trip widens back to a matching value. Checker
    # exercises adjacent-row variation (any filter coverage
    # is implicit; encoder writes filter 0).
    ("checker.rgb.pencfix", _COLOR_TYPE_RGB,
     lambda: _checker_rgb_opaque(16, 16, 4)),

    # Grayscale — encoder requires R == G == B in the input;
    # alpha is 0xFF so the round-trip widens back via
    # Y → R=G=B=Y, A=0xFF.
    ("gradient.gray.pencfix", _COLOR_TYPE_GRAY,
     lambda: _gradient_gray_opaque(16, 16)),

    # Solid RGBA — degenerate single-color input. Tests that
    # the encoder doesn't choke on a zero-variance scanline
    # and that filter type 0 emits the raw bytes verbatim.
    ("solid.rgba.pencfix", _COLOR_TYPE_RGBA,
     lambda: _solid_rgba_opaque(8, 8, (32, 64, 128))),

    # Solid Rgb (alpha=0xFF) — degenerate path for the alpha-
    # drop encoder branch.
    ("solid.rgb.pencfix", _COLOR_TYPE_RGB,
     lambda: _solid_rgba_opaque(8, 8, (200, 50, 75))),

    # Solid Grayscale (R=G=B=Y) — degenerate path for the
    # grayscale encoder branch.
    ("solid.gray.pencfix", _COLOR_TYPE_GRAY,
     lambda: _solid_rgba_opaque(8, 8, (180, 180, 180))),

    # Tiny 1x1 — minimum-dimension boundary fixture.
    ("tiny_1x1.rgba.pencfix", _COLOR_TYPE_RGBA,
     lambda: _gradient_rgba(1, 1)),

    # Non-square — exercises rowBytes != height arithmetic.
    ("nonsquare_5x3.rgba.pencfix", _COLOR_TYPE_RGBA,
     lambda: _gradient_rgba(5, 3)),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def main() -> int:
    here = Path(__file__).parent

    for name, color_type, make_source in _FIXTURES:
        path = here / name

        rgba = make_source()
        # Filename embeds the dimensions for the documented
        # subset; for the rest infer width / height from the
        # source factory's intent. We extract from the source
        # buffer length using the convention that all fixtures
        # use either 16x16, 8x8, 1x1, or 5x3.
        if "tiny_1x1" in name:
            w, h = 1, 1
        elif "nonsquare_5x3" in name:
            w, h = 5, 3
        elif "solid" in name:
            w, h = 8, 8
        else:
            w, h = 16, 16

        if len(rgba) != w * h * 4:
            print(f"ERROR: {name}: dimension mismatch — "
                  f"rgba length {len(rgba)} != {w}*{h}*4")
            return 1

        blob = _pack_pencfix(color_type, w, h, rgba)
        path.write_bytes(blob)

        digest = hashlib.sha256(blob).hexdigest()[:16]
        print(f"wrote {name:36s} "
              f"{len(blob):6d} B (sha256:{digest}…), "
              f"{w}x{h} color_type={color_type}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
