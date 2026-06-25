#!/usr/bin/env python3
"""Generate the fixture corpus for the foundation/palette_quantiser
primitive.

palette_quantiser's freeze gate is contract_validation — for each
fixture the adapter must satisfy declared shape invariants
(palette size bound, indices length, indices in range, determinism,
exact-match self-identity, palette covers unique colours when
feasible). Fixtures cover the invariant axes:

  * single-pixel input — degenerate floor for palette_size_bound
    (palette must have 1 entry, indices must be [0])
  * flat-colour input — K=1 unique colour; palette_covers_unique_
    colors_when_feasible drives palette.size == 1
  * K <= max_colors — quantisation has nothing to do; palette
    must cover the input colours exactly (zero RGB distance)
  * K == max_colors — boundary between "no quantisation" and
    "quantisation"; palette still covers exactly
  * K > max_colors — forces real quantisation work; only
    palette_size_bound + indices_length_matches +
    indices_in_range + determinism + exact_match_self_identity
    are testable invariants
  * 4bpp budget (max_colors=16) — small palette stresses
    bucket-split / centroid algorithms
  * 8bpp budget (max_colors=256) — full BCL ColorPalette range
  * varying alpha with identical RGB — proves alpha is dropped
    before quantisation (K_rgb=1, K_rgba=many)
  * non-square dimensions — proves indexing is row-major and
    width*height is computed correctly (no width/height swap)
  * gradient — rich RGB cube traversal, exercises quantiser's
    bucket-splitting heuristic on smooth colour distributions

Wire format (little-endian; spec yaml documents the layout):

    +0   uint32  width
    +4   uint32  height
    +8   uint32  max_colors
   +12   uint8[w*h*4]  rgba (row-major top-left)

Fixtures are public-domain — generated deterministically from this
script, no downloaded content.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import struct
from pathlib import Path


# ---------------------------------------------------------------------------
# Wire format helper
# ---------------------------------------------------------------------------


def _pack(width: int, height: int, max_colors: int,
          rgba: bytes) -> bytes:
    """Pack a fixture into the wire format the dispatch sibling
    parses. Validates the rgba buffer has the declared dimensions
    so a bad fixture surfaces at generate-time, not at gate-time.
    """
    if width <= 0 or height <= 0:
        raise ValueError(
            f"width/height must be >= 1 (got {width}x{height})")
    if not (2 <= max_colors <= 256):
        raise ValueError(
            f"max_colors must be in [2, 256] (got {max_colors})")
    expected = width * height * 4
    if len(rgba) != expected:
        raise ValueError(
            f"rgba buffer is {len(rgba)} bytes; expected "
            f"{expected} for {width}x{height}")
    header = struct.pack("<III", width, height, max_colors)
    return header + rgba


# ---------------------------------------------------------------------------
# Pixel-buffer builders
# ---------------------------------------------------------------------------


def _solid(width: int, height: int,
           rgb: tuple[int, int, int]) -> bytes:
    """w×h RGBA8 buffer filled with one colour (alpha = 0xFF)."""
    r, g, b = rgb
    pixel = bytes((r, g, b, 0xFF))
    return pixel * (width * height)


def _palette_horizontal(width: int, height: int,
                        palette: list[tuple[int, int, int]]) -> bytes:
    """w×h RGBA8 buffer where each column picks a colour from
    palette by ``column_index % len(palette)``. Predictable layout
    for invariant inspection — the unique-colour count is exactly
    ``min(width, len(palette))``.
    """
    rows: list[bytes] = []
    for _ in range(height):
        row = b""
        for x in range(width):
            r, g, b = palette[x % len(palette)]
            row += bytes((r, g, b, 0xFF))
        rows.append(row)
    return b"".join(rows)


def _gradient(width: int, height: int) -> bytes:
    """w×h RGBA8 RGB cube traversal. R varies along x, G along y,
    B is fixed at 128. Produces width*height distinct colours when
    width and height are both <= 256 — well above any practical
    palette budget, so quantisation always has real work."""
    out = bytearray()
    for y in range(height):
        for x in range(width):
            r = (x * 255) // max(1, width - 1)
            g = (y * 255) // max(1, height - 1)
            b = 0x80
            out += bytes((r, g, b, 0xFF))
    return bytes(out)


def _alpha_varying_solid_rgb(width: int, height: int,
                              rgb: tuple[int, int, int]) -> bytes:
    """w×h RGBA8 buffer where every pixel has the same RGB but
    alpha varies row-by-row. After alpha is dropped, K_rgb = 1
    (the quantiser must see a single-colour input)."""
    r, g, b = rgb
    rows: list[bytes] = []
    for y in range(height):
        a = (y * 255) // max(1, height - 1)
        rows.append(bytes((r, g, b, a)) * width)
    return b"".join(rows)


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


def _single_pixel() -> bytes:
    """1×1 RGBA8, max_colors=16. Floor case: palette must have
    exactly 1 entry covering the input colour; indices must be
    [0]."""
    return _pack(1, 1, 16, _solid(1, 1, (0xC8, 0x32, 0x96)))


def _flat_color_8x8_4bpp() -> bytes:
    """8×8 RGBA8, all pixels colour (40, 200, 80), max_colors=16.
    K=1; palette.size must be 1, every index must be 0."""
    return _pack(8, 8, 16, _solid(8, 8, (40, 200, 80)))


def _two_colors_4x4_8bpp() -> bytes:
    """4×4 RGBA8, two alternating colours, max_colors=256.
    K=2 << max_colors=256; palette covers exactly those two, every
    indexed pixel matches its source RGB exactly."""
    palette = [(255, 0, 0), (0, 0, 255)]
    return _pack(4, 4, 256,
                 _palette_horizontal(4, 4, palette))


def _two_colors_4x4_4bpp() -> bytes:
    """4×4 RGBA8, two alternating colours, max_colors=16. Same
    test as above but with the 4bpp budget — no quantisation
    work because K=2 << 16."""
    palette = [(0, 192, 0), (192, 0, 0)]
    return _pack(4, 4, 16,
                 _palette_horizontal(4, 4, palette))


def _sixteen_colors_4bpp() -> bytes:
    """16×1 RGBA8, sixteen distinct colours, max_colors=16.
    Boundary case: K == max_colors. Quantiser has the budget to
    represent every colour exactly; palette must cover all 16."""
    palette = [
        (i * 16, (255 - i * 16) & 0xFF, (i * 32) & 0xFF)
        for i in range(16)
    ]
    return _pack(16, 1, 16, _palette_horizontal(16, 1, palette))


def _seventeen_colors_4bpp() -> bytes:
    """17×1 RGBA8, seventeen distinct colours, max_colors=16.
    Forces real quantisation: K > max_colors. The relaxable
    invariants (palette_covers_unique_colors_when_feasible) are
    skipped here — only palette_size_bound + indices_length +
    indices_in_range + determinism + exact_match_self_identity
    apply."""
    palette = [
        ((i * 15) & 0xFF, (i * 31) & 0xFF, (i * 47) & 0xFF)
        for i in range(17)
    ]
    return _pack(17, 1, 16, _palette_horizontal(17, 1, palette))


def _gradient_16x16_8bpp() -> bytes:
    """16×16 RGBA8 RGB-cube gradient, max_colors=256. K=256 (the
    16x16 distinct (R, G) pairs at fixed B) — meets the budget
    exactly. Stresses bucket-split heuristics on a smooth, dense
    colour distribution."""
    return _pack(16, 16, 256, _gradient(16, 16))


def _gradient_16x16_4bpp() -> bytes:
    """16×16 RGBA8 RGB-cube gradient, max_colors=16. Heavy
    quantisation: 256 unique colours into 16 palette slots.
    Stresses palette_size_bound + indices_in_range under
    real reduction."""
    return _pack(16, 16, 16, _gradient(16, 16))


def _alpha_drop_proof() -> bytes:
    """4×8 RGBA8 buffer with one RGB triple but alpha varying
    row-by-row. After alpha is dropped, K_rgb=1; palette.size
    must be 1. An implementation that quantises in RGBA space
    instead of RGB would produce a palette of 8 entries — caught
    by palette_covers_unique_colors_when_feasible."""
    return _pack(
        4, 8, 256,
        _alpha_varying_solid_rgb(4, 8, (180, 60, 240)))


def _wide_thin_2bpp() -> bytes:
    """32×1 RGBA8 with two alternating colours, max_colors=2.
    Smallest possible palette budget; proves the
    palette_size_bound floor (max_colors=2 is allowed and
    palette must have <= 2 entries)."""
    palette = [(0xFF, 0xFF, 0xFF), (0x00, 0x00, 0x00)]
    return _pack(32, 1, 2, _palette_horizontal(32, 1, palette))


def _tall_thin_8bpp() -> bytes:
    """1×16 RGBA8 single-column with 16 distinct colours,
    max_colors=256. Proves indexing is row-major (width=1
    means every pixel is its own row's only pixel; an
    implementation that swapped width/height would still pass
    the contract here, but combined with _wide_thin_2bpp gives
    coverage)."""
    palette = [
        ((i * 16) & 0xFF, 0x40, (255 - i * 16) & 0xFF)
        for i in range(16)
    ]
    rows = b"".join(
        bytes((r, g, b, 0xFF)) for r, g, b in palette)
    return _pack(1, 16, 256, rows)


# ---------------------------------------------------------------------------
# Driver
# ---------------------------------------------------------------------------


_FIXTURES: dict[str, callable] = {
    "single_pixel.bin": _single_pixel,
    "flat_color_8x8_4bpp.bin": _flat_color_8x8_4bpp,
    "two_colors_4x4_8bpp.bin": _two_colors_4x4_8bpp,
    "two_colors_4x4_4bpp.bin": _two_colors_4x4_4bpp,
    "sixteen_colors_4bpp.bin": _sixteen_colors_4bpp,
    "seventeen_colors_4bpp.bin": _seventeen_colors_4bpp,
    "gradient_16x16_8bpp.bin": _gradient_16x16_8bpp,
    "gradient_16x16_4bpp.bin": _gradient_16x16_4bpp,
    "alpha_drop_proof.bin": _alpha_drop_proof,
    "wide_thin_2bpp.bin": _wide_thin_2bpp,
    "tall_thin_8bpp.bin": _tall_thin_8bpp,
}


def main() -> None:
    here = Path(__file__).parent
    for name, builder in _FIXTURES.items():
        out = here / name
        out.write_bytes(builder())
        print(f"wrote {name} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
