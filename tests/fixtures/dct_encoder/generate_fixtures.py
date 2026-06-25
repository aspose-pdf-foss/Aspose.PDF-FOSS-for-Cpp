#!/usr/bin/env python3
"""Generate the .dencfix fixture corpus for foundation::dct_encoder.

Each fixture is a self-describing binary blob bundling
(quality, photometric, dpi, page) into one file. The per-target
dispatch sibling (DctEncoderDispatch.cs) parses the blob, calls
DctEncoder.Encode with the embedded options + RGBA page, then
calls Dctdecode.Decode on the encoded JPEG and returns one
EncodeRoundtripResult where ExpectedPixels = the input RGBA8
and DecodedPixels = the bytes Dctdecode produced.

Wire format documented in the project spec
(``fixture_format:`` field). All multi-byte integers are
little-endian. Single page per fixture (baseline JPEG has no
multi-image container concept).

PSNR floor at the gate is 30 dB (lossy mode of
pixel_encode_roundtrip). Content + quality combinations are
chosen so a correct T.81 baseline encoder body using the
Annex K reference quant + Huffman tables clears the 30 dB bar
on every fixture: smooth gradients clear comfortably at every
in-corpus quality (q=50 / 75 / 90); a deliberately block-
aligned 8×8 checker is near-lossless because each block is
internally uniform; sharp non-block-aligned high-frequency
content is excluded from the v1 corpus (would clear the bar
only at very high quality and adds little gate value over the
included fixtures).

Filename convention:
  <name>.<photo>.q<quality>.<dpi>.dencfix

  <photo>   ∈ {gray, rgb}
  <quality> 0..100
  <dpi>     integer

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

MAGIC = b"DENC"
VERSION = 1

# Wire-format photometric codes (private 1..2 values; the
# dispatch sibling translates these to the encoder's
# DctEncoder.Photometric option type).
PHOTO_GRAY = 1
PHOTO_RGB = 2


def _gradient_rgba(w: int, h: int) -> bytes:
    """Diagonal RGBA gradient. R varies horizontally, G varies
    vertically, B = (R+G)/2, A = 0xFF. Smooth content — every
    block's DCT is dominated by low-frequency coefficients,
    so quantisation is highly effective and PSNR sits well
    above the 30 dB floor at every in-corpus quality."""
    out = bytearray(w * h * 4)
    for y in range(h):
        gv = int(255 * y / max(1, h - 1))
        for x in range(w):
            rv = int(255 * x / max(1, w - 1))
            bv = (rv + gv) // 2
            i = (y * w + x) * 4
            out[i + 0] = rv
            out[i + 1] = gv
            out[i + 2] = bv
            out[i + 3] = 0xFF
    return bytes(out)


def _solid_rgba(w: int, h: int, rgb: tuple[int, int, int]) -> bytes:
    """Solid colour. Every 8×8 block is internally uniform so
    every AC coefficient is zero — quantisation is trivially
    near-lossless and PSNR is dominated only by the uniform
    DC error after RGB→YCbCr→RGB rounding."""
    r, g, b = rgb
    out = bytearray(w * h * 4)
    for i in range(w * h):
        out[i * 4 + 0] = r
        out[i * 4 + 1] = g
        out[i * 4 + 2] = b
        out[i * 4 + 3] = 0xFF
    return bytes(out)


def _checker_aligned_rgba(w: int, h: int, cell: int,
                          on_rgb: tuple[int, int, int],
                          off_rgb: tuple[int, int, int]) -> bytes:
    """RGBA checkerboard with cells aligned to JPEG 8×8 block
    boundaries (cell == 8 + w / h are multiples of 8). Each
    block is internally uniform so the encoder's DCT produces
    only a DC coefficient + zero AC coefficients per block —
    the round-trip is near-lossless even at low quality. Tests
    photometric=Rgb / Grayscale + multi-block layout without
    introducing high-frequency intra-block content."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            r, g, b = on_rgb if on else off_rgb
            i = (y * w + x) * 4
            out[i + 0] = r
            out[i + 1] = g
            out[i + 2] = b
            out[i + 3] = 0xFF
    return bytes(out)


def _gray_ramp_as_rgba(w: int, h: int) -> bytes:
    """Grayscale-as-RGBA — R = G = B = horizontal ramp,
    A = 0xFF. Smooth content; encoder collapses R=G=B=Y and
    emits a 1-component JPEG. Dctdecode widens the 1-component
    output back to RGBA (A=0xFF, R=G=B=Y)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            v = int(255 * x / max(1, w - 1))
            i = (y * w + x) * 4
            out[i + 0] = v
            out[i + 1] = v
            out[i + 2] = v
            out[i + 3] = 0xFF
    return bytes(out)


def pack(quality: int, photo: int, dpi: int,
         w: int, h: int, rgba: bytes) -> bytes:
    """Build a .dencfix byte buffer per the format documented
    in the project spec fixture_format."""
    if not 0 <= quality <= 100:
        raise ValueError(f"quality {quality} not in 0..100")
    if photo not in (PHOTO_GRAY, PHOTO_RGB):
        raise ValueError(f"photo {photo} not in 1..2")
    if dpi <= 0:
        raise ValueError(f"dpi {dpi} not positive")
    if w == 0 or h == 0:
        raise ValueError("width / height must be positive")
    if len(rgba) != w * h * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {w}*{h}*4 = {w*h*4}")
    # Header: 4s magic, B version, B quality, B photo, B reserved0,
    #         I dpi, 4s reserved1.
    header = struct.pack(
        "<4sBBBBI4s",
        MAGIC, VERSION,
        quality, photo, 0,
        dpi,
        b"\x00\x00\x00\x00",
    )
    page_header = struct.pack("<II", w, h)
    return header + page_header + rgba


# Filename = <name>.<photo>.q<quality>.<dpi>.dencfix
# v1 matrix coverage:
#   quality      : q=50, q=75 (default), q=90                     (3)
#   photometric  : rgb / grayscale                                 (2)
#   dpi carry    : 72 / 96 (no-op in v1 but exercises option-bag)  (2)
#   dimensions   : 32×16 / 16×16 / 31×17 / 8×8                     (4)
#   content      : gradient / solid / checker-aligned              (3)
# Total: 10 fixtures.
_FIXTURES: list[tuple[str, dict]] = [
    # ---- Quality coverage on RGBA gradient (smooth content;
    # always well above the 30 dB floor). ----

    # Default quality, default DPI. Baseline RGB fixture.
    ("gradient.rgb.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=72,
          w=32, h=16, rgba=_gradient_rgba(32, 16))),

    # Lower quality boundary. Smooth content still clears 30 dB
    # at q=50 with a wide margin.
    ("gradient.rgb.q50.72.dencfix",
     dict(quality=50, photo=PHOTO_RGB, dpi=72,
          w=32, h=16, rgba=_gradient_rgba(32, 16))),

    # Higher quality boundary. Higher precision DCT coefficients;
    # exercises the IJG quality-scaling formula's `quality < 50`
    # branch transition (different scale formula).
    ("gradient.rgb.q90.72.dencfix",
     dict(quality=90, photo=PHOTO_RGB, dpi=72,
          w=32, h=16, rgba=_gradient_rgba(32, 16))),

    # DPI variation. v1 doesn't emit JFIF APP0 so DPI is a no-op
    # carry-through; this fixture exercises that the option-bag
    # field is accepted at non-default values without affecting
    # the encoded byte stream.
    ("gradient.rgb.q75.96.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=96,
          w=32, h=16, rgba=_gradient_rgba(32, 16))),

    # ---- Photometric coverage. ----

    # Solid colour, RGB. Sanity check — every block's DCT has
    # only a DC coefficient; PSNR should be > 50 dB regardless
    # of quality (only the uniform DC error after
    # RGB→YCbCr→RGB rounding contributes).
    ("solid.rgb.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=72,
          w=32, h=16, rgba=_solid_rgba(32, 16, (128, 64, 192)))),

    # Block-aligned checker on RGB. Cells equal JPEG block size
    # (8×8) and image dims are multiples of 8, so each block is
    # internally uniform — near-lossless even at q=75.
    ("checker_aligned.rgb.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=72,
          w=16, h=16, rgba=_checker_aligned_rgba(
              16, 16, 8, (255, 64, 64), (64, 64, 255)))),

    # Grayscale gradient. Encoder emits 1-component JPEG;
    # Dctdecode widens back to RGBA. Smooth content — high PSNR.
    ("ramp.gray.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_GRAY, dpi=72,
          w=32, h=16, rgba=_gray_ramp_as_rgba(32, 16))),

    # Solid colour, Grayscale. Sanity check on the 1-component
    # SOF0 / SOS marker emit path with a uniform sample stream.
    ("solid.gray.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_GRAY, dpi=72,
          w=32, h=16, rgba=_solid_rgba(32, 16, (200, 200, 200)))),

    # ---- Dimension edge cases. ----

    # Non-multiple-of-8 dimensions. 31 wide × 17 tall — forces
    # both width AND height block padding (last column of blocks
    # has 7 valid columns + 1 padded; last row of blocks has 1
    # valid row + 7 padded). The encoder pads by replicating the
    # last in-image column / row so the trailing padded samples
    # don't introduce high-frequency edges that would tank PSNR
    # at the image boundary.
    ("nonmod8.rgb.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=72,
          w=31, h=17, rgba=_gradient_rgba(31, 17))),

    # Smallest possible image — one 8×8 block. Exercises the
    # single-block degenerate path (no AC zero-runs spanning
    # blocks; per-block DC is its own previous-DC reference;
    # Encode produces minimal SOI..EOI byte stream).
    ("tiny.rgb.q75.72.dencfix",
     dict(quality=75, photo=PHOTO_RGB, dpi=72,
          w=8, h=8, rgba=_gradient_rgba(8, 8))),
]


def main() -> int:
    here = Path(__file__).parent
    for name, args in _FIXTURES:
        path = here / name
        blob = pack(**args)
        path.write_bytes(blob)
        digest = hashlib.sha256(blob).hexdigest()[:16]
        print(f"wrote {name:42s} {len(blob):6d} B "
              f"(sha256:{digest}…)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
