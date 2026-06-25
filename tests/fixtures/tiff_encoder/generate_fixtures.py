#!/usr/bin/env python3
"""Generate the .tencfix fixture corpus for foundation::tiff_encoder.

Each fixture is a self-describing binary blob bundling
(compression, photometric, samples_per_pixel, dpi, color_map,
pages) into one file. The per-target dispatch sibling
(TiffEncoderDispatch.cs / tiff_encoder_dispatch_pixel_encode_-
roundtrip.hpp) parses the blob, calls TiffEncoder.Encode with
the embedded options + RGBA pages, then calls TiffDecoder.Decode
on the encoded TIFF and returns one EncodeRoundtripResult per
page where ExpectedPixels = the input RGBA8 for that page and
DecodedPixels = the bytes TiffDecoder produced.

Wire format documented in the project spec
(``fixture_format:`` field). All multi-byte integers are
little-endian. Every input pixel's alpha is 0xFF for non-RGBA-
SPP fixtures so the round-trip is byte-identical (the gate is
in byte-equality mode); samples-per-pixel-3 fixtures drop alpha
on encode and TiffDecoder widens back to A=0xFF, samples-per-
pixel-1 grayscale collapses R=G=B=Y on encode and widens back,
and samples-per-pixel-1 palette uses an identity grayscale
ColorMap so the input's R-channel index round-trips to RGB
matching the input.

Filename convention:
  <name>.<comp>.<photo>.<spp>.<dpi>.tencfix

  <comp>  ∈ {none, lzw, deflate, packbits}
  <photo> ∈ {bzero, rgb, palette}
  <spp>   ∈ {1, 3, 4}
  <dpi>   integer

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

MAGIC = b"TENC"
VERSION = 2

# Wire-format compression codes (private; do NOT match TIFF
# tag codes — kept compact so the .tencfix header fits in 16
# bytes for non-palette fixtures). The dispatch sibling
# translates these to the encoder's Compression option.
COMP_NONE = 0
COMP_LZW = 1
COMP_DEFLATE = 2
COMP_PACKBITS = 3

# Wire-format photometric codes (private; match TIFF's
# Photometric-tag values 1/2/3 for {BlackIsZero, Rgb, Palette}
# coincidentally — WhiteIsZero=0 is out of v1 scope).
PHOTO_BZERO = 1
PHOTO_RGB = 2
PHOTO_PAL = 3


def _gradient_rgba(w: int, h: int) -> bytes:
    """Diagonal RGBA gradient. R varies horizontally, G
    vertically, B = (R+G)/2, A = 0xFF (so round-trip is byte-
    identical for SPP=3 too, since alpha gets dropped + widened
    back to 0xFF)."""
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


def _checker_rgba(w: int, h: int, cell: int,
                  on_rgb: tuple[int, int, int],
                  off_rgb: tuple[int, int, int]) -> bytes:
    """RGBA8 checkerboard. Alpha is 0xFF in both colours so
    SPP=3 fixtures round-trip byte-identical."""
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


def _gradient_gray_as_rgba(w: int, h: int) -> bytes:
    """Grayscale-as-RGBA — R = G = B = ramp(x), A = 0xFF.
    Encoder collapses R=G=B=Y to a single byte for SPP=1
    grayscale; TiffDecoder widens back to RGBA. Round-trip
    is byte-identical."""
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


def _palette_identity_rgba(w: int, h: int) -> bytes:
    """Palette-input-as-RGBA (8bpp variant; R-byte values 0..255).
    The input's R byte is the palette index; with the identity
    grayscale ColorMap we use, the decoder produces R=G=B=index.
    We construct the RGBA buffer so input bytes already match
    that round-trip: R=index, G=index, B=index, A=0xFF. Index
    varies as a checkerboard across two values so PackBits/LZW/
    Deflate compression has something to compress."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // 4) + (y // 4)) % 2 == 0
            v = 0x40 if on else 0xC0
            i = (y * w + x) * 4
            out[i + 0] = v
            out[i + 1] = v
            out[i + 2] = v
            out[i + 3] = 0xFF
    return bytes(out)


def _palette4_identity_rgba(w: int, h: int) -> bytes:
    """Palette-input-as-RGBA, 4bpp variant. The encoder masks
    the R byte to its low nibble for 4bpp packing, so we must
    keep R in [0, 15]; G/B mirror R so the decoded R=G=B=index
    round-trip is byte-identical with the identity 16-entry
    color_map. Two distinct values arranged as a checkerboard
    so LZW/Deflate compression has something to compress."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // 4) + (y // 4)) % 2 == 0
            v = 0x02 if on else 0x0D  # both in [0, 15]
            i = (y * w + x) * 4
            out[i + 0] = v
            out[i + 1] = v
            out[i + 2] = v
            out[i + 3] = 0xFF
    return bytes(out)


def _bw_checker_rgba(w: int, h: int, cell: int) -> bytes:
    """Pure-black / pure-white checkerboard for 1bpp BlackIsZero
    fixtures. Every pixel is either (0, 0, 0, 0xFF) or
    (255, 255, 255, 0xFF) — the only inputs that round-trip
    byte-identically through luma threshold + decode widening."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            v = 0xFF if on else 0x00
            i = (y * w + x) * 4
            out[i + 0] = v
            out[i + 1] = v
            out[i + 2] = v
            out[i + 3] = 0xFF
    return bytes(out)


def _identity_gray_color_map(n_entries: int) -> bytes:
    """3 * n_entries SHORTs, little-endian, encoding an identity
    grayscale palette: R[i] = G[i] = B[i] = i*257 (so the high
    byte of each SHORT is exactly i, matching TiffDecoder's
    `(entry >> 8) & 0xFF` widening). 96 bytes for n_entries=16
    (4bpp), 1536 bytes for n_entries=256 (8bpp)."""
    out = bytearray(3 * n_entries * 2)
    # All reds, then all greens, then all blues.
    for ch in range(3):
        for i in range(n_entries):
            v = i * 257  # high byte = i exactly
            off = (ch * n_entries + i) * 2
            out[off] = v & 0xFF
            out[off + 1] = (v >> 8) & 0xFF
    return bytes(out)


def pack(comp: int, photo: int, spp: int, bps: int, dpi: int,
         pages: list[tuple[int, int, bytes]],
         color_map: bytes = b"") -> bytes:
    """Build a .tencfix v2 byte buffer per the format documented
    in the project spec fixture_format."""
    if not 1 <= len(pages) <= 255:
        raise ValueError("page count must be in [1, 255]")
    if bps not in (1, 4, 8):
        raise ValueError(f"bits_per_sample must be 1/4/8 (got {bps})")
    if len(color_map) > 0xFFFF:
        raise ValueError("color_map byte count exceeds uint16")
    if photo == PHOTO_PAL:
        expected_cmap = 3 * (1 << bps) * 2
        if len(color_map) != expected_cmap:
            raise ValueError(
                f"palette photometric at bps={bps} requires "
                f"{expected_cmap}-byte ColorMap (got {len(color_map)})")
    elif len(color_map) != 0:
        raise ValueError(
            "non-palette photometric must have empty ColorMap")
    # Header: 4s magic, B version, B comp, B photo, B spp,
    #         I dpi, H cmap_byte_len, B page_count, B bits_per_sample.
    header = struct.pack(
        "<4sBBBBI HBB",
        MAGIC, VERSION,
        comp, photo, spp,
        dpi,
        len(color_map),
        len(pages),
        bps,
    )
    parts: list[bytes] = [header, color_map]
    for w, h, rgba in pages:
        if len(rgba) != w * h * 4:
            raise ValueError(
                f"page rgba length {len(rgba)} != "
                f"{w}*{h}*4 = {w*h*4}")
        parts.append(struct.pack("<II", w, h))
        parts.append(rgba)
    return b"".join(parts)


# Filename = <name>.<comp>.<photo>.<spp>.<dpi>.tencfix
# v2 matrix coverage:
#   compressions  : None / Lzw / Deflate / PackBits             (4)
#   photometrics  : Rgb-SPP4 / Rgb-SPP3 / BlackIsZero / Palette (4)
#   bits_per_sample : 1 / 4 / 8                                 (3)
#   multi-page    : 2 fixtures
# `bzero1` / `palette4` photometric tokens distinguish 1bpp
# BlackIsZero from 8bpp BlackIsZero, and 4bpp Palette from 8bpp
# Palette, in directory listings. The `bps` value lives in the
# wire header at byte offset 15 — filenames are diagnostic.
# Total: 16 fixtures.
_FIXTURES: list[tuple[str, dict]] = [
    # ---- Compression coverage on Rgb-SPP3 (the most common
    # production photometric for TIFF rendering output). ----

    # No compression, RGB drop-alpha. Default-DPI baseline.
    ("checker.none.rgb.3.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_RGB, spp=3, bps=8, dpi=72,
          pages=[(32, 16, _checker_rgba(
              32, 16, 4, (255, 128, 0), (0, 128, 255)))])),

    # LZW. Routes through foundation::lzw::Encode.
    ("checker.lzw.rgb.3.72.tencfix",
     dict(comp=COMP_LZW, photo=PHOTO_RGB, spp=3, bps=8, dpi=72,
          pages=[(32, 16, _checker_rgba(
              32, 16, 4, (255, 128, 0), (0, 128, 255)))])),

    # Deflate. Routes through foundation::flate::Encode.
    ("checker.deflate.rgb.3.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_RGB, spp=3, bps=8, dpi=72,
          pages=[(32, 16, _checker_rgba(
              32, 16, 4, (255, 128, 0), (0, 128, 255)))])),

    # PackBits. Routes through foundation::runlength::Encode.
    ("checker.packbits.rgb.3.72.tencfix",
     dict(comp=COMP_PACKBITS, photo=PHOTO_RGB, spp=3, bps=8, dpi=72,
          pages=[(32, 16, _checker_rgba(
              32, 16, 4, (255, 128, 0), (0, 128, 255)))])),

    # ---- Photometric coverage on the same compressions. ----

    # RGBA pass-through (SPP=4). Default options. Exercises the
    # ExtraSamples=2 IFD entry for unassociated alpha.
    ("gradient.none.rgba.4.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_RGB, spp=4, bps=8, dpi=72,
          pages=[(32, 16, _gradient_rgba(32, 16))])),

    # RGBA pass-through with Deflate. Exercises non-trivial
    # compression on an RGBA payload.
    ("gradient.deflate.rgba.4.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_RGB, spp=4, bps=8, dpi=72,
          pages=[(32, 16, _gradient_rgba(32, 16))])),

    # 8bpp Grayscale (SPP=1, BlackIsZero, BPS=8). Encoder
    # collapses R=G=B=Y to a single byte (strict R=G=B check).
    # TiffDecoder widens back. Non-72 DPI to exercise the
    # X/YResolution out-of-line tag emission path at a value
    # other than the default.
    ("ramp.none.bzero.1.96.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_BZERO, spp=1, bps=8, dpi=96,
          pages=[(32, 16, _gradient_gray_as_rgba(32, 16))])),

    # 8bpp Grayscale with LZW. Exercises LZW over a single-
    # channel data stream (vs the 3-channel rgb fixtures above).
    ("ramp.lzw.bzero.1.72.tencfix",
     dict(comp=COMP_LZW, photo=PHOTO_BZERO, spp=1, bps=8, dpi=72,
          pages=[(32, 16, _gradient_gray_as_rgba(32, 16))])),

    # 8bpp Palette (SPP=1, Photometric=Palette, BPS=8). Encoder
    # writes the input's R byte as palette index; identity
    # grayscale ColorMap means the round-trip is R=G=B=index.
    # Exercises ColorMap out-of-line tag emission (1536 bytes).
    ("swatch.none.palette.1.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_PAL, spp=1, bps=8, dpi=72,
          pages=[(32, 16, _palette_identity_rgba(32, 16))],
          color_map=_identity_gray_color_map(256))),

    # 8bpp Palette with Deflate. Exercises ColorMap + compressed
    # strip on the same fixture.
    ("swatch.deflate.palette.1.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_PAL, spp=1, bps=8, dpi=72,
          pages=[(32, 16, _palette_identity_rgba(32, 16))],
          color_map=_identity_gray_color_map(256))),

    # ---- 1bpp BlackIsZero coverage (BPS=1). ----

    # Pure-black/pure-white checker, no compression. Exercises
    # the luma-threshold + MSB-first bit-pack path. Width is
    # picked so row stride = (24+7)/8 = 3 bytes, which is NOT
    # a multiple of width — exercises the row-padding rule.
    ("bw_checker.none.bzero1.1.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_BZERO, spp=1, bps=1, dpi=72,
          pages=[(24, 12, _bw_checker_rgba(24, 12, 3))])),

    # 1bpp + Deflate. Exercises Deflate over a bit-packed
    # payload — common production case for fax-style scans
    # rendered via TiffDevice.
    ("bw_checker.deflate.bzero1.1.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_BZERO, spp=1, bps=1, dpi=72,
          pages=[(32, 16, _bw_checker_rgba(32, 16, 4))])),

    # ---- 4bpp Palette coverage (BPS=4). ----

    # 4bpp palette, no compression. R-byte's low nibble is the
    # index; identity 16-entry color_map round-trips R=G=B=index.
    # Width odd (15) so row stride = (15+1)/2 = 8 bytes
    # exercises the row-padding rule.
    ("swatch4.none.palette4.1.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_PAL, spp=1, bps=4, dpi=72,
          pages=[(15, 12, _palette4_identity_rgba(15, 12))],
          color_map=_identity_gray_color_map(16))),

    # 4bpp palette with Deflate. Exercises 16-entry ColorMap
    # out-of-line tag emission (96 bytes) + Deflate over the
    # nibble-packed strip.
    ("swatch4.deflate.palette4.1.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_PAL, spp=1, bps=4, dpi=72,
          pages=[(32, 16, _palette4_identity_rgba(32, 16))],
          color_map=_identity_gray_color_map(16))),

    # ---- Multi-page coverage. ----

    # Two RGBA pages (SPP=4), no compression. Exercises chained-
    # IFD encoding + per-page strip data placement + first-IFD-
    # offset back-patch.
    ("two_pages.none.rgba.4.72.tencfix",
     dict(comp=COMP_NONE, photo=PHOTO_RGB, spp=4, bps=8, dpi=72,
          pages=[
              (32, 16, _checker_rgba(
                  32, 16, 4, (255, 0, 0), (255, 255, 255))),
              (32, 16, _checker_rgba(
                  32, 16, 4, (0, 255, 0), (255, 255, 255))),
          ])),

    # Two RGB pages (SPP=3) with Deflate. Multi-page +
    # compression + alpha-drop on every page exercises the
    # whole hot path.
    ("two_pages.deflate.rgb.3.72.tencfix",
     dict(comp=COMP_DEFLATE, photo=PHOTO_RGB, spp=3, bps=8, dpi=72,
          pages=[
              (32, 16, _checker_rgba(
                  32, 16, 4, (200, 50, 50), (250, 250, 250))),
              (32, 16, _checker_rgba(
                  32, 16, 4, (50, 200, 50), (250, 250, 250))),
          ])),
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
