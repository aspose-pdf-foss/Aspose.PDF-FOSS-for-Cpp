#!/usr/bin/env python3
"""Generate the JPEG 2000 fixture corpus + .pixels sidecars for the
foundation/jpx (JPXDecode) primitive.

Each fixture is a small, deterministic JPEG 2000 file produced by
Pillow's OpenJPEG backend from a synthetic integer source pattern;
the matching `.pixels` sidecar is the oracle-authored RGBA8 pixel
buffer the freeze-gate driver compares against by PSNR. Fixtures and
sidecars are public-domain — the only inputs are integers from this
file.

Pipeline per fixture:

  1. Pillow builds a source image (L grayscale or RGB) from a
     deterministic pattern function.
  2. Pillow saves it as JPEG 2000 via the OpenJPEG backend, choosing
     the wavelet per fixture:
       - 5/3 reversible (byte-exact gate): irreversible=False
       - 9/7 irreversible (PSNR gate):     irreversible=True
     The raw-codestream (`.j2k`, SOC 0xFF4F) vs boxed (`.jp2`,
     signature box) wrapping follows the file extension.
  3. The `.pixels` sidecar is written from the ORACLE decode:
     re-open the just-saved file with Pillow (`Image.open(path)`),
     which decodes through the OpenJPEG backend — an implementation
     entirely independent of our from-scratch decoder — convert to
     RGBA8 (grayscale widens Y into R=G=B; alpha is a constant 0xFF),
     and write the raw buffer.

OpenJPEG (via Pillow, PIL.features.check_codec("jpg_2000")) is the
declared oracle — the analogue of djpeg for dctdecode. opj_decompress
would serve identically but is not required: the libopenjp2-backed
Pillow decode is what we use.

All fixtures are authored within the jpx v1 scope: single tile, 8-bit,
1- or 3-component, no sub-sampling, LRCP progression, a single quality
layer, default code-block style, 5/3 or 9/7 wavelet. `mct=1` on the
RGB fixtures exercises the reversible/irreversible colour transform
(RCT/ICT) path.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

# Pillow with the OpenJPEG codec is the build-time oracle. If it is
# missing the script must fail loudly — this is a fixture-authoring
# tool, not a library degrade path.
from PIL import Image
import PIL.features


# ---------------------------------------------------------------------------
# Source-image authoring (deterministic integer patterns)
# ---------------------------------------------------------------------------


def _gray_ramp(w: int, h: int) -> Image.Image:
    """Grayscale horizontal ramp, L-mode (1 component). The decoder
    widens the single component into R=G=B; the oracle does the same
    on convert("RGBA"). A stride or level-shift error shows up as a
    skewed ramp."""
    img = Image.new("L", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = int(255 * x / max(1, w - 1))
    return img


def _gray_blocks(w: int, h: int) -> Image.Image:
    """Grayscale four-quadrant blocks with distinct flat levels. With
    num_resolutions=1 (no DWT) and lossless 5/3, the decoded tier-1
    coefficient at each sample is exactly (value - 2^(8-1)), so this
    fixture doubles as an independent tier-1 coefficient oracle: the
    expected code-block coefficients are recoverable from the .pixels
    sidecar as gray-128, with no wavelet or colour transform in the
    way."""
    img = Image.new("L", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            left, top = x < w // 2, y < h // 2
            if top and left:
                px[x, y] = 30
            elif top:
                px[x, y] = 200
            elif left:
                px[x, y] = 130
            else:
                px[x, y] = 90
    return img


def _rgb_blocks(w: int, h: int) -> Image.Image:
    """RGB four-quadrant flat blocks. Flat regions keep the lossless
    codestream tiny and expose channel-order / MCT-inversion errors
    (a swapped R/B turns the red quadrant blue)."""
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            left, top = x < w // 2, y < h // 2
            if top and left:
                px[x, y] = (220, 30, 30)
            elif top:
                px[x, y] = (30, 200, 60)
            elif left:
                px[x, y] = (40, 60, 210)
            else:
                px[x, y] = (180, 180, 40)
    return img


def _gray_gradient(w: int, h: int) -> Image.Image:
    """Smooth grayscale diagonal gradient — mid-frequency content for
    the 9/7 irreversible PSNR path. Float-DWT rounding is what the
    50 dB floor absorbs; a real transform error collapses PSNR well
    below it."""
    img = Image.new("L", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = int(255 * (x + y) / max(1, (w - 1) + (h - 1)))
    return img


def _rgb_sinusoids(w: int, h: int) -> Image.Image:
    """Smooth deterministic RGB content — three orthogonal sinusoids,
    one per channel. Representative low-to-mid frequency energy for
    the 9/7 + ICT path."""
    import math
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            r = 128 + int(60 * math.sin(2 * math.pi * x / 16))
            g = 128 + int(60 * math.sin(2 * math.pi * y / 12))
            b = 128 + int(60 * math.sin(2 * math.pi * (x + y) / 20))
            px[x, y] = (max(0, min(255, r)),
                        max(0, min(255, g)),
                        max(0, min(255, b)))
    return img


# ---------------------------------------------------------------------------
# Fixture registry — (filename, source-factory, JPEG2000 save kwargs)
# ---------------------------------------------------------------------------
#
# Pillow JPEG2000 save kwargs (see Pillow's Jpeg2KImagePlugin):
#   irreversible    — False => 5/3 reversible (lossless); True => 9/7
#   quality_mode    — "rates" or "dB"
#   quality_layers  — sequence; a single entry => one quality layer
#   num_resolutions — number of resolutions = decomposition levels + 1
#   mct             — 1 => apply the multiple-component (colour)
#                     transform (RCT for 5/3, ICT for 9/7); RGB only
#
# Axes exercised:
#   gray_1level_8       — 1 component, NO DWT (num_resolutions=1),
#                         lossless: simplest tier-1 target + coeff oracle
#   gray_5x3_32         — 1 component, 5/3 lossless, 2 levels  (byte-exact)
#   rgb_5x3_32          — 3 component, 5/3 lossless + RCT, 2 levels (byte-exact)
#   multilevel_5x3_64   — 3 component, 5/3 lossless + RCT, 3 levels (byte-exact)
#   gradient_9x7_64     — 1 component, 9/7 irreversible, 2 levels (PSNR)
#   rgb_9x7_64          — 3 component, 9/7 irreversible + ICT, 2 levels (PSNR)
#   jp2_boxed_rgb_32    — 3 component, 5/3 lossless, JP2 box container
#                         (drives the JP2 box-parser path; .jp2, so it
#                         is gated via the unit e2e test, not the
#                         .j2k freeze-gate glob)
_LOSSLESS_53 = {"irreversible": False, "quality_mode": "rates",
                "quality_layers": [0]}  # rate 0 => lossless, 1 layer
_LOSSY_97 = {"irreversible": True, "quality_mode": "dB",
             "quality_layers": [60]}    # single high-quality layer

_FIXTURES: list = [
    ("gray_1level_8.j2k", lambda: _gray_blocks(8, 8),
     {**_LOSSLESS_53, "num_resolutions": 1}),
    ("gray_5x3_32.j2k", lambda: _gray_ramp(32, 32),
     {**_LOSSLESS_53, "num_resolutions": 3}),
    ("rgb_5x3_32.j2k", lambda: _rgb_blocks(32, 32),
     {**_LOSSLESS_53, "num_resolutions": 3, "mct": 1}),
    ("multilevel_5x3_64.j2k", lambda: _rgb_blocks(64, 64),
     {**_LOSSLESS_53, "num_resolutions": 4, "mct": 1}),
    ("gradient_9x7_64.j2k", lambda: _gray_gradient(64, 64),
     {**_LOSSY_97, "num_resolutions": 3}),
    ("rgb_9x7_64.j2k", lambda: _rgb_sinusoids(64, 64),
     {**_LOSSY_97, "num_resolutions": 3, "mct": 1}),
    ("jp2_boxed_rgb_32.jp2", lambda: _rgb_blocks(32, 32),
     {**_LOSSLESS_53, "num_resolutions": 3, "mct": 1}),
]


# Negative fixtures: codestreams that are deliberately OUT of v1 scope,
# used only to prove the parser raises. They get NO .pixels sidecar (our
# decoder throws by design) and live under reject/ so the freeze-gate
# glob (jpx/*.j2k, non-recursive) never tries to decode them.
#   multitile_rgb_32 — 2x2 tiles (tile_size 16x16 over 32x32): the
#                      single-tile parser must reject it.
_REJECT_FIXTURES: list = [
    ("multitile_rgb_32.j2k", lambda: _rgb_blocks(32, 32),
     {**_LOSSLESS_53, "num_resolutions": 3, "mct": 1,
      "tile_size": (16, 16)}),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def main() -> int:
    here = Path(__file__).parent

    if not PIL.features.check_codec("jpg_2000"):
        print("ERROR: Pillow lacks the OpenJPEG (jpg_2000) codec — "
              "install a Pillow built against libopenjp2.", file=sys.stderr)
        return 1
    print("oracle: Pillow OpenJPEG backend (jpg_2000) available")

    for name, make_source, save_kwargs in _FIXTURES:
        out_path = here / name
        sidecar_path = out_path.with_suffix(".pixels")

        src = make_source()
        src.save(out_path, format="JPEG2000", **save_kwargs)
        cs_bytes = out_path.read_bytes()

        # Oracle decode: an independent OpenJPEG-backed decode of the
        # bytes we just wrote, widened to RGBA8.
        oracle = Image.open(out_path).convert("RGBA")
        rgba = oracle.tobytes()
        sidecar_path.write_bytes(rgba)

        w, h = oracle.size
        assert len(rgba) == w * h * 4, (
            f"{name}: sidecar {len(rgba)} != {w}*{h}*4")

        cs_digest = hashlib.sha256(cs_bytes).hexdigest()[:16]
        px_digest = hashlib.sha256(rgba).hexdigest()[:16]
        marker = "FF4F" if cs_bytes[:2] == b"\xff\x4f" else \
                 ("JP2" if cs_bytes[4:8] == b"jP  " else "????")
        print(f"wrote {name:24s} {w:3d}x{h:<3d} [{marker:4s}] "
              f"{len(cs_bytes):6d} B cs (sha256:{cs_digest}…), "
              f"{len(rgba):7d} B rgba (sha256:{px_digest}…)")

    reject_dir = here / "reject"
    reject_dir.mkdir(exist_ok=True)
    for name, make_source, save_kwargs in _REJECT_FIXTURES:
        out_path = reject_dir / name
        make_source().save(out_path, format="JPEG2000", **save_kwargs)
        cs_bytes = out_path.read_bytes()
        cs_digest = hashlib.sha256(cs_bytes).hexdigest()[:16]
        print(f"wrote reject/{name:15s} (no sidecar) "
              f"{len(cs_bytes):6d} B cs (sha256:{cs_digest}…)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
