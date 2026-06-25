#!/usr/bin/env python3
"""Generate the TIFF fixture corpus + .pixels sidecars for the
foundation/tiff_decoder primitive.

Each fixture is a (small, deterministic) TIFF file produced by
Pillow from a synthetic source pattern; the matching `.pixels`
sidecar is the oracle-authored RGBA8 pixel buffer the freeze-
gate driver compares against by PSNR. Fixtures and sidecars
are public-domain — the only inputs are integers from this file.

Pipeline per fixture:

  1. Pillow builds a source image from a deterministic pattern
     function. Source mode depends on the fixture's photometric
     intent: 'L' for grayscale / WhiteIsZero / BlackIsZero,
     'P' for palette, 'RGB' for RGB, 'RGBA' for RGBA.
  2. Pillow saves it as a TIFF (`<name>.tif`) using the chosen
     compression / photometric / byte order. Compress=None
     (compression=1), 'lzw' (5), 'tiff_deflate' (8),
     'packbits' (32773) — the four lossless compressions in v1
     scope.
  3. Pillow re-reads the saved TIFF and produces an RGBA8
     buffer (`.pixels` sidecar) — same shape every other lossy
     primitive uses (width*height*4 bytes, top-to-bottom,
     R G B A).

PIL is the declared oracle (see tiff_decoder.yaml). We use it
both to author the source and to read the encoded TIFF back —
the adapter's job at gate time is to produce the same RGBA8
bytes PIL produces. A second oracle (e.g. imagemagick) could
be added later for byte-identical agreement gating; for v1
PIL alone is sufficient given the lossless-only compression
scope.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
from pathlib import Path

from PIL import Image, ImageDraw


# ---------------------------------------------------------------------------
# Source-image authoring
# ---------------------------------------------------------------------------


def _gradient_gray(w: int, h: int) -> Image.Image:
    """Horizontal grayscale ramp, Y = round(255 * x / (w-1)).
    Mode 'L' — single 8-bit channel. PIL's 'L' TIFF write yields
    Photometric=BlackIsZero by default; passing photometric=
    'min-is-white' inverts to WhiteIsZero on write."""
    img = Image.new("L", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = int(255 * x / max(1, w - 1))
    return img


def _checker_rgb(w: int, h: int, cell: int) -> Image.Image:
    """RGB checkerboard. Cell-on = (255,128,0), cell-off =
    (0,128,255). Mode 'RGB' — 3 channels, each 8-bit. PIL writes
    SamplesPerPixel=3, Photometric=RGB by default for 'RGB'."""
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            px[x, y] = (255, 128, 0) if on else (0, 128, 255)
    return img


def _gradient_rgba(w: int, h: int) -> Image.Image:
    """RGBA gradient. R varies horizontally, G varies vertically,
    B = (R + G) // 2, A varies horizontally too. Mode 'RGBA' — 4
    channels. PIL writes SamplesPerPixel=4, ExtraSamples=2 for
    'RGBA'. Round-trips our own TiffEncoder's RGBA-pass-through
    output."""
    img = Image.new("RGBA", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            r = int(255 * x / max(1, w - 1))
            g = int(255 * y / max(1, h - 1))
            b = (r + g) // 2
            a = 255 - r // 2
            px[x, y] = (r, g, b, a)
    return img


def _palette_circles(w: int, h: int) -> Image.Image:
    """Palette image, 8-bit indices into a 256-entry RGB palette.
    Three concentric filled circles in different palette
    colours; the rest is palette index 0 (black). Mode 'P' —
    PIL writes SamplesPerPixel=1, Photometric=Palette, with the
    ColorMap tag carrying the palette."""
    img = Image.new("P", (w, h), 0)
    palette = [0] * (256 * 3)
    palette[1*3 + 0] = 255  # palette index 1 = red
    palette[2*3 + 0] = 0    # palette index 2 = green
    palette[2*3 + 1] = 200
    palette[3*3 + 0] = 60   # palette index 3 = blue-ish
    palette[3*3 + 1] = 80
    palette[3*3 + 2] = 220
    img.putpalette(palette)
    draw = ImageDraw.Draw(img)
    cx, cy = w // 2, h // 2
    draw.ellipse((cx-12, cy-12, cx+12, cy+12), fill=1)
    draw.ellipse((cx-7, cy-7, cx+7, cy+7), fill=2)
    draw.ellipse((cx-3, cy-3, cx+3, cy+3), fill=3)
    return img


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


# Each entry: (filename, source factory, save kwargs). Save
# kwargs are passed through to PIL's Image.save(format='TIFF',
# **kwargs). Compression values: 'raw' (compression=1, none),
# 'tiff_lzw' (5), 'tiff_deflate' (8), 'packbits' (32773).
_FIXTURES = [
    # Uncompressed RGB — the baseline path. 32x16 to keep the
    # fixture small while giving enough pixels for a checker
    # pattern to exercise multiple TIFF rows.
    ("rgb_uncompressed_32.tif",
     lambda: _checker_rgb(32, 16, 4),
     {"compression": "raw"}),

    # Uncompressed RGBA — round-trips our own TiffEncoder
    # (ExtraSamples=2 path).
    ("rgba_uncompressed_32.tif",
     lambda: _gradient_rgba(32, 16),
     {"compression": "raw"}),

    # Grayscale BlackIsZero. PIL's default for 'L' mode.
    # WhiteIsZero is excluded from the corpus because PIL's
    # `convert('RGBA')` does not honour the polarity-flip on
    # read; an oracle-agreement gate cannot pin the inversion
    # against a single oracle that gets it wrong. Adding a
    # second oracle (e.g. ImageMagick) would let WhiteIsZero
    # back in.
    ("gray_blackiszero_32.tif",
     lambda: _gradient_gray(32, 16),
     {"compression": "raw"}),

    # Palette image (Photometric=3) — exercises ColorMap tag
    # path.
    ("palette_32.tif",
     lambda: _palette_circles(32, 16),
     {"compression": "raw"}),

    # LZW-compressed RGB. PIL's TIFF LZW writer is mature and
    # produces a stream the foundation::lzw decoder agrees
    # with byte-for-byte.
    ("rgb_lzw_32.tif",
     lambda: _checker_rgb(32, 16, 4),
     {"compression": "tiff_lzw"}),

    # Deflate-compressed RGB. TIFF tag value 8; identical wire
    # format to PDF /FlateDecode.
    ("rgb_deflate_32.tif",
     lambda: _checker_rgb(32, 16, 4),
     {"compression": "tiff_deflate"}),

    # PackBits-compressed RGB. TIFF tag value 32773; identical
    # to RunLength encoding.
    ("rgb_packbits_32.tif",
     lambda: _checker_rgb(32, 16, 4),
     {"compression": "packbits"}),
]


# ---------------------------------------------------------------------------
# Oracle pipeline: PIL re-read → RGBA sidecar
# ---------------------------------------------------------------------------


def _pil_to_rgba(tif_path: Path) -> tuple[bytes, int, int]:
    """Re-read the saved TIFF through PIL, convert to RGBA, and
    return the raw RGBA8 bytes alongside the dimensions PIL
    reports.

    PIL's `convert('RGBA')` does the photometric expansion the
    foundation primitive will do at gate time:
      L → R=G=B=Y, A=255
      P → ColorMap lookup, A=255
      RGB → R G B 0xFF
      RGBA → pass through

    PIL is the declared oracle; the gate compares the foundation
    primitive's RGBA8 output against this buffer by PSNR ≥ 50dB.
    All in-scope compressions are lossless so the real-world
    PSNR is infinity (byte-identical) on a correct decoder.
    """
    img = Image.open(tif_path)
    rgba = img.convert("RGBA")
    return rgba.tobytes(), rgba.width, rgba.height


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def main() -> int:
    here = Path(__file__).parent

    for name, make_source, save_kwargs in _FIXTURES:
        tif_path = here / name
        sidecar_path = tif_path.with_suffix(".pixels")

        src = make_source()
        # Pillow's TIFF writer. Compression default is 'raw'
        # (uncompressed); the per-fixture kwargs override.
        src.save(tif_path, format="TIFF", **save_kwargs)
        tif_bytes = tif_path.read_bytes()

        rgba, w, h = _pil_to_rgba(tif_path)
        if len(rgba) != w * h * 4:
            print(f"ERROR: {name} sidecar length {len(rgba)} != "
                  f"{w}*{h}*4 = {w*h*4}")
            return 1
        sidecar_path.write_bytes(rgba)

        tif_digest = hashlib.sha256(tif_bytes).hexdigest()[:16]
        px_digest = hashlib.sha256(rgba).hexdigest()[:16]
        print(f"wrote {name:32s} "
              f"{len(tif_bytes):6d} B tif (sha256:{tif_digest}…), "
              f"{w}x{h} rgba ({len(rgba)} B, sha256:{px_digest}…)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
