#!/usr/bin/env python3
"""Generate the PNG fixture corpus + .pixels sidecars for the
foundation/png_decoder primitive.

Each fixture is a (small, deterministic) PNG file authored either
by Pillow (for the photometric coverage fixtures) or by a
hand-rolled byte writer in this script (for the per-scanline
filter coverage fixtures — Pillow's PNG writer picks a filter
heuristic per scanline and does not expose a "force this filter
type" knob, so explicit filter coverage requires hand-emit).
The matching `.pixels` sidecar is the oracle-authored RGBA8
pixel buffer the freeze-gate driver compares against by PSNR.
PIL is the declared oracle for the read side — every fixture
is re-read through `PIL.Image.open(...).convert('RGBA')` to
produce the sidecar bytes. PNG is lossless on every in-scope
color type so the real-world PSNR is infinity (byte-identical)
on a correct decoder; the gate's 50 dB safety net catches
filter-inverse off-by-one bugs and photometric expansion bugs.

Pipeline per fixture:

  1. Build the source RGBA pixel buffer from a deterministic
     pattern function (gradient / checker / solid).
  2. Either:
       a. (photometric coverage) Save through Pillow as a PNG
          in the chosen color type (Grayscale / RGB / RGBA);
          OR
       b. (filter coverage) Hand-emit a PNG with explicit
          filter byte per scanline (None / Sub / Up / Average
          / Paeth) wrapping the same source data.
  3. Re-read the saved PNG through PIL; convert to RGBA8;
     write the byte buffer to the matching `.pixels` sidecar.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import struct
import zlib
from pathlib import Path

from PIL import Image


# ---------------------------------------------------------------------------
# Source-image authoring (raw RGBA8 buffers — channel layout is
# always RGBA so the same pattern can be saved as Grayscale / RGB
# / RGBA via Pillow `convert(...)` or projected channel-by-
# channel for the hand-rolled writer)
# ---------------------------------------------------------------------------


def _gradient_rgba(w: int, h: int) -> bytes:
    """RGBA gradient. R varies horizontally, G varies vertically,
    B = (R + G) // 2, A varies horizontally too."""
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


def _checker_rgba(w: int, h: int, cell: int) -> bytes:
    """RGBA checkerboard. Cell-on = (255,128,0,255),
    cell-off = (0,128,255,255)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            i = (y * w + x) * 4
            out[i:i + 4] = bytes(
                (255, 128, 0, 255) if on else (0, 128, 255, 255))
    return bytes(out)


def _solid_rgba(w: int, h: int, rgba: tuple[int, int, int, int]) -> bytes:
    """Solid RGBA fill — exercises all-zeros / all-equal scanline
    edge cases for the filter-inverse paths."""
    return bytes(rgba) * (w * h)


# ---------------------------------------------------------------------------
# Hand-rolled PNG writer with explicit per-scanline filter type
# ---------------------------------------------------------------------------
#
# Implements PNG 1.2 §3 (signature + chunk layout), §4 (IHDR /
# IDAT / IEND), §6 (filter byte + 5 filter algorithms), §10
# (zlib-wrapped Deflate via stdlib `zlib`), §13 (CRC32 IEEE
# 802.3 polynomial 0xEDB88320 reversed via stdlib `zlib.crc32`).
#
# Fixed configuration — color type 6 (RGBA), bit depth 8, no
# interlace, deflate compression, filter set 0. Color type is
# fixed because filter-coverage fixtures only need to exercise
# the filter inverse paths in the decoder; pairing every filter
# with every color type would multiply the corpus 5× without
# adding decoder coverage. Photometric coverage rides on the
# Pillow-authored fixtures (color type 0 + 2 + 6).


_PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def _png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    """Emit one PNG chunk: 4-byte length + 4-byte type + data
    + 4-byte CRC32 over (type + data)."""
    if len(chunk_type) != 4:
        raise ValueError(f"chunk type must be 4 bytes, got {chunk_type!r}")
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return (struct.pack(">I", len(data)) + chunk_type + data
            + struct.pack(">I", crc))


def _paeth_predictor(a: int, b: int, c: int) -> int:
    """Per PNG 1.2 §9.4."""
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _filter_scanline(filter_type: int, current: bytes, prior: bytes,
                     bpp: int) -> bytes:
    """Forward scanline filter — produces the filtered byte
    stream the decoder will undo in step 5 of the spec yaml's
    Algorithm. `current` and `prior` are raw (un-filtered)
    `width * bpp` byte buffers; `prior` is all zeros for the
    first scanline. `bpp` is bytes per pixel (4 for RGBA8).
    The result is the filtered buffer (without the leading
    filter-type byte; the caller prepends it)."""
    n = len(current)
    out = bytearray(n)
    if filter_type == 0:  # None
        return bytes(current)
    if filter_type == 1:  # Sub
        for i in range(n):
            left = current[i - bpp] if i >= bpp else 0
            out[i] = (current[i] - left) & 0xFF
        return bytes(out)
    if filter_type == 2:  # Up
        for i in range(n):
            up = prior[i]
            out[i] = (current[i] - up) & 0xFF
        return bytes(out)
    if filter_type == 3:  # Average
        for i in range(n):
            left = current[i - bpp] if i >= bpp else 0
            up = prior[i]
            out[i] = (current[i] - ((left + up) // 2)) & 0xFF
        return bytes(out)
    if filter_type == 4:  # Paeth
        for i in range(n):
            left = current[i - bpp] if i >= bpp else 0
            up = prior[i]
            up_left = prior[i - bpp] if i >= bpp else 0
            out[i] = (current[i] - _paeth_predictor(left, up, up_left)) & 0xFF
        return bytes(out)
    raise ValueError(f"filter type must be 0..4, got {filter_type}")


def _write_png_rgba_explicit_filter(path: Path, width: int, height: int,
                                    rgba: bytes, filter_type: int) -> None:
    """Hand-emit a PNG file with color type 6 (RGBA), bit depth
    8, non-interlaced, every scanline using `filter_type`. The
    decoder receives a stream that exercises exactly one filter
    inverse path."""
    if len(rgba) != width * height * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {width}*{height}*4 "
            f"= {width * height * 4}")

    # IHDR — 13 bytes: width(4) + height(4) + depth(1) + color(1)
    # + compression(0) + filter(0) + interlace(0).
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

    # Filtered scanline stream — per scanline, 1 filter byte
    # followed by `width * 4` filtered bytes.
    bpp = 4
    row_bytes = width * bpp
    filtered = bytearray()
    prior = bytes(row_bytes)  # first row uses zero prior
    for y in range(height):
        current = rgba[y * row_bytes:(y + 1) * row_bytes]
        filtered.append(filter_type)
        filtered.extend(_filter_scanline(filter_type, current, prior, bpp))
        prior = current

    # IDAT — single chunk carrying the zlib-wrapped Deflate of
    # the filtered scanline stream. zlib.compress writes the
    # standard zlib wrapper (CMF + FLG + Deflate stream + Adler32).
    idat_payload = zlib.compress(bytes(filtered), level=9)

    # Assemble the file: signature + IHDR + IDAT + IEND.
    out = (_PNG_SIGNATURE
           + _png_chunk(b"IHDR", ihdr)
           + _png_chunk(b"IDAT", idat_payload)
           + _png_chunk(b"IEND", b""))

    path.write_bytes(out)


# ---------------------------------------------------------------------------
# Pillow-authored fixtures (photometric coverage)
# ---------------------------------------------------------------------------


def _save_pil(path: Path, width: int, height: int, rgba: bytes,
              color_mode: str) -> None:
    """Save through Pillow in the requested color mode. Pillow
    picks a filter heuristic — fine for photometric coverage
    fixtures because the decoder accepts whatever filter Pillow
    chose; per-filter coverage is handled separately by the
    hand-emit path."""
    img_rgba = Image.frombytes("RGBA", (width, height), rgba)
    if color_mode == "RGBA":
        img = img_rgba
    elif color_mode == "RGB":
        img = img_rgba.convert("RGB")
    elif color_mode == "L":
        img = img_rgba.convert("L")
    else:
        raise ValueError(f"unsupported color mode {color_mode!r}")
    img.save(path, format="PNG", optimize=False)


# ---------------------------------------------------------------------------
# Oracle pipeline: PIL re-read → RGBA sidecar
# ---------------------------------------------------------------------------


def _pil_to_rgba(png_path: Path) -> tuple[bytes, int, int]:
    """Re-read the saved PNG through PIL, convert to RGBA, and
    return the raw RGBA8 bytes alongside the dimensions PIL
    reports.

    PIL's `convert('RGBA')` does the photometric expansion the
    foundation primitive will do at gate time:
      L    → R=G=B=Y, A=255
      RGB  → R G B 0xFF
      RGBA → pass through

    PIL is the declared oracle; the gate compares the foundation
    primitive's RGBA8 output against this buffer by PSNR ≥ 50dB.
    PNG is lossless so the real-world PSNR on a correct decoder
    is infinity (byte-identical).
    """
    img = Image.open(png_path)
    rgba = img.convert("RGBA")
    return rgba.tobytes(), rgba.width, rgba.height


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


# (name, kind, builder, knobs)
# kind 'pil' → Pillow-authored, knobs is the color_mode string.
# kind 'filter' → hand-emit RGBA, knobs is the filter type int.
_FIXTURES = [
    # Photometric coverage — Pillow-authored. Each exercises
    # the decoder's photometric expansion (Grayscale → RGBA,
    # RGB → RGBA, RGBA pass-through) but not necessarily a
    # specific filter type; Pillow's default heuristic is the
    # libpng "min sum of absolutes" picker.
    ("rgba_gradient_16.png", "pil",
     lambda: _gradient_rgba(16, 16), "RGBA"),
    ("rgb_checker_16.png", "pil",
     lambda: _checker_rgba(16, 16, 4), "RGB"),
    ("gray_gradient_16.png", "pil",
     lambda: _gradient_rgba(16, 16), "L"),

    # Filter coverage — hand-emit RGBA so each fixture
    # exercises exactly one of the five filter inverse paths.
    ("rgba_filter_none_8.png", "filter",
     lambda: _gradient_rgba(8, 8), 0),
    ("rgba_filter_sub_8.png", "filter",
     lambda: _gradient_rgba(8, 8), 1),
    ("rgba_filter_up_8.png", "filter",
     lambda: _gradient_rgba(8, 8), 2),
    ("rgba_filter_average_8.png", "filter",
     lambda: _gradient_rgba(8, 8), 3),
    ("rgba_filter_paeth_8.png", "filter",
     lambda: _gradient_rgba(8, 8), 4),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def main() -> int:
    here = Path(__file__).parent

    for name, kind, make_source, knobs in _FIXTURES:
        png_path = here / name
        sidecar_path = png_path.with_suffix(".pixels")

        rgba_src = make_source()
        # Source pattern is always 8x8 or 16x16 RGBA — the
        # filter-coverage fixtures use 8x8 to keep the corpus
        # small while giving multiple scanlines + multiple
        # pixels per scanline (so Sub / Up / Average / Paeth
        # all have non-zero left + prior contributions).
        side = 16 if "16" in name else 8

        if kind == "pil":
            _save_pil(png_path, side, side, rgba_src, knobs)
        elif kind == "filter":
            _write_png_rgba_explicit_filter(
                png_path, side, side, rgba_src, knobs)
        else:
            raise ValueError(f"unknown fixture kind {kind!r}")

        png_bytes = png_path.read_bytes()
        rgba, w, h = _pil_to_rgba(png_path)
        if len(rgba) != w * h * 4:
            print(f"ERROR: {name} sidecar length {len(rgba)} != "
                  f"{w}*{h}*4 = {w*h*4}")
            return 1
        sidecar_path.write_bytes(rgba)

        png_digest = hashlib.sha256(png_bytes).hexdigest()[:16]
        px_digest = hashlib.sha256(rgba).hexdigest()[:16]
        print(f"wrote {name:32s} "
              f"{len(png_bytes):6d} B png (sha256:{png_digest}…), "
              f"{w}x{h} rgba ({len(rgba)} B, sha256:{px_digest}…)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
