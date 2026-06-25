#!/usr/bin/env python3
"""Generate the .bencfix fixture corpus for foundation::bmp_encoder.

Each fixture is a self-describing binary blob bundling
(color_type, page) into one file. The per-target dispatch
sibling (BmpEncoderDispatch.cs) parses the blob, calls
BmpEncoder.Encode with the embedded options + RGBA page, then
calls BmpDecoder.Decode on the encoded BMP and returns one
EncodeRoundtripResult where ExpectedPixels = the input RGBA8
and DecodedPixels = the bytes BmpDecoder produced.

Wire format documented in the project spec
(``fixture_format:`` field). All multi-byte integers are
little-endian. Single page per fixture (BMP has no multi-image
container concept).

The gate runs in byte-equality mode (BMP is lossless on every
in-scope (bit-depth, compression) pair). For ColorType=Bgr the
input alpha channel must be 0xFF since the encoder drops
alpha on that path and the decoder widens to alpha=0xFF on
read — non-0xFF alpha would survive round-trip only on
ColorType=Bgra.

Filename convention:
  <name>.<color_type>.bencfix

  <color_type> ∈ {bgr, bgra}

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


# BMP biBitCount byte values — passed through unchanged in
# the .bencfix wire byte.
_COLOR_TYPE_BGR = 24
_COLOR_TYPE_BGRA = 32


# ---------------------------------------------------------------------------
# Source-image authoring (raw RGBA8 buffers)
# ---------------------------------------------------------------------------


def _gradient_rgba(w: int, h: int) -> bytes:
    """RGBA gradient. R varies horizontally, G varies vertically,
    B = (R + G) // 2, A varies horizontally. The alpha channel
    varies non-trivially — only suitable for ColorType=Bgra
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


def _checker_rgba_opaque(w: int, h: int, cell: int) -> bytes:
    """RGBA checkerboard with alpha=0xFF. Suitable for
    ColorType=Bgr or Bgra (Bgr path drops alpha; the round-
    trip widens back to 0xFF which matches the input)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            i = (y * w + x) * 4
            out[i:i + 4] = bytes(
                (255, 128, 0, 255) if on else (0, 128, 255, 255))
    return bytes(out)


def _gradient_rgb_opaque(w: int, h: int) -> bytes:
    """RGBA buffer where R / G / B vary and alpha=0xFF.
    Suitable for ColorType=Bgr (alpha is dropped on encode
    and widened back to 0xFF on read so round-trip matches)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            r = (255 * x) // max(1, w - 1)
            g = (255 * y) // max(1, h - 1)
            b = (r + g) // 2
            i = (y * w + x) * 4
            out[i:i + 4] = bytes((r, g, b, 255))
    return bytes(out)


def _solid_rgba_opaque(w: int, h: int, rgb: tuple[int, int, int]) -> bytes:
    """Solid RGBA with alpha=0xFF — exercises a degenerate
    encoder input (zero-variance scanlines) for any color type."""
    return bytes((*rgb, 255)) * (w * h)


# ---------------------------------------------------------------------------
# .bencfix writer
# ---------------------------------------------------------------------------


def _pack_bencfix(color_type: int, width: int, height: int,
                  rgba: bytes) -> bytes:
    """Pack one .bencfix v1 bundle. Header layout per
    the project spec fixture_format."""
    if len(rgba) != width * height * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {width}*{height}*4 "
            f"= {width * height * 4}")
    if color_type not in (_COLOR_TYPE_BGR, _COLOR_TYPE_BGRA):
        raise ValueError(f"color_type {color_type} not in 24/32")

    header = struct.pack("<4sBBBB",
                         b"BENC",
                         1,           # version
                         color_type,
                         0, 0)        # reserved
    page = struct.pack("<II", width, height) + rgba
    return header + page


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


# (filename, color_type, (width, height), source_factory)
_FIXTURES = [
    # Bgra pass-through — exercises the simplest encoder path
    # (no alpha drop). Gradient covers varying R / G / B / A
    # values across the image; verifies BI_BITFIELDS block
    # emit + alpha preservation.
    ("gradient.bgra.bencfix", _COLOR_TYPE_BGRA, (16, 16),
     lambda: _gradient_rgba(16, 16)),

    # Bgra pass-through, opaque checker — adjacent-row
    # variation without the alpha-preservation variable
    # confounding the failure mode.
    ("checker.bgra.bencfix", _COLOR_TYPE_BGRA, (8, 8),
     lambda: _checker_rgba_opaque(8, 8, 2)),

    # Bgra solid — degenerate single-color input for the
    # 32-bit branch.
    ("solid.bgra.bencfix", _COLOR_TYPE_BGRA, (4, 4),
     lambda: _solid_rgba_opaque(4, 4, (32, 64, 128))),

    # Bgr drop-alpha — alpha is 0xFF in the source so the
    # round-trip widens back to a matching value. Width 16
    # is 48 data bytes per row (multiple of 4) → no row
    # padding; tests the basic 24-bit branch.
    ("gradient.bgr.bencfix", _COLOR_TYPE_BGR, (16, 16),
     lambda: _gradient_rgb_opaque(16, 16)),

    # Bgr solid — degenerate path for the alpha-drop encoder
    # branch.
    ("solid.bgr.bencfix", _COLOR_TYPE_BGR, (8, 8),
     lambda: _solid_rgba_opaque(8, 8, (200, 50, 75))),

    # Bgr 24-bit with width 3 → 9 data bytes + 3 padding =
    # 12 stride. Exercises the 3-byte-padding row-stride
    # path.
    ("padded_3x4.bgr.bencfix", _COLOR_TYPE_BGR, (3, 4),
     lambda: _checker_rgba_opaque(3, 4, 1)),

    # Bgr 24-bit with width 5 → 15 data bytes + 1 padding =
    # 16 stride. Exercises the 1-byte-padding case (the
    # easiest one to forget).
    ("padded_5x4.bgr.bencfix", _COLOR_TYPE_BGR, (5, 4),
     lambda: _gradient_rgb_opaque(5, 4)),

    # Tiny 1x1 — minimum-dimension boundary fixture for the
    # 32-bit branch (minimum-dimension 24-bit is implicitly
    # covered by the padded_3x4 fixture).
    ("tiny_1x1.bgra.bencfix", _COLOR_TYPE_BGRA, (1, 1),
     lambda: _gradient_rgba(1, 1)),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def main() -> int:
    here = Path(__file__).parent

    for name, color_type, (w, h), make_source in _FIXTURES:
        path = here / name
        rgba = make_source()

        if len(rgba) != w * h * 4:
            print(f"ERROR: {name}: dimension mismatch — "
                  f"rgba length {len(rgba)} != {w}*{h}*4")
            return 1

        # ColorType=Bgr requires alpha=0xFF in source so the
        # round-trip widens back to a matching value.
        if color_type == _COLOR_TYPE_BGR:
            for i in range(3, len(rgba), 4):
                if rgba[i] != 0xFF:
                    print(f"ERROR: {name}: ColorType=Bgr but "
                          f"source pixel {i // 4} has alpha "
                          f"{rgba[i]} != 0xFF — non-opaque alpha "
                          "would not survive the alpha-drop "
                          "round-trip")
                    return 1

        blob = _pack_bencfix(color_type, w, h, rgba)
        path.write_bytes(blob)

        digest = hashlib.sha256(blob).hexdigest()[:16]
        print(f"wrote {name:36s} "
              f"{len(blob):6d} B (sha256:{digest}…), "
              f"{w}x{h} color_type={color_type}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
