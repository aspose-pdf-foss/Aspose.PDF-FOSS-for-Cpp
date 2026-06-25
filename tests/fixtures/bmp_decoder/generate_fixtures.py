#!/usr/bin/env python3
"""Generate the BMP fixture corpus + .pixels sidecars for the
foundation/bmp_decoder primitive.

Each fixture is a (small, deterministic) BMP file written by a
hand-rolled byte emitter in this script. PIL is not used as the
encoder because:

  1. PIL's BMP encoder picks the (biBitCount, biCompression)
     pair based on the input image mode (RGB → 24-bit BI_RGB;
     RGBA → may emit BITMAPV4HEADER / 32-bit BI_BITFIELDS but
     this varies by Pillow version). The decoder's freeze gate
     wants explicit coverage of both in-scope variants —
     deterministic fixture authoring requires the byte layout
     to be controlled by this script, not by Pillow's heuristic.

  2. Fixture authoring authority: the .pixels sidecar is the
     RGBA8 buffer the decoder must produce; using the source
     RGBA buffer directly (which is what we hand-encoded into
     the BMP) keeps the oracle as the authored truth, not a
     third-party library's interpretation.

For BMP, the SOURCE RGBA buffer is the oracle:

  - BMP is lossless on every in-scope (bit-depth, compression)
    pair, so a correct decoder must produce byte-identical
    output to the source we encoded.
  - PIL is a confirming witness on the variants it fully
    supports, but PIL's BMP reader does NOT preserve the alpha
    channel on 32-bit BI_BITFIELDS images (it widens alpha to
    0xFF on convert('RGBA')). For 32-bit BGRA fixtures with
    non-trivial alpha, PIL would diverge from the spec's
    "alpha byte at offset +3 is preserved" requirement; the
    decoder's contract follows the BMP file format, not PIL's
    coercion. Authoring the sidecar from source preserves the
    alpha test.
  - For 24-bit BI_RGB fixtures the source's alpha channel must
    be 0xFF since BMP 24-bit doesn't carry alpha and the
    decoder widens alpha to 0xFF on read; the source-as-oracle
    contract enforces this.

The freeze gate compares the decoder's RGBA output against
the source-authored sidecar by PSNR ≥ 50dB. The real-world
PSNR is infinity (byte-identical) on a correct decoder; the
50 dB safety net catches bottom-up row inversion bugs, 24-bit
row-padding miscalculations, and channel-order swaps.

Pipeline per fixture:

  1. Build the source RGBA pixel buffer from a deterministic
     pattern function (gradient / checker / solid).
  2. Hand-emit the BMP byte stream in the chosen
     (biBitCount, biCompression) variant — 24-bit BGR with
     BI_RGB, or 32-bit BGRA with BI_BITFIELDS at the
     canonical standard masks.
  3. Write the source RGBA buffer to the matching `.pixels`
     sidecar — that buffer IS the decoder's expected output
     by construction.
  4. Sanity check: re-read the saved BMP through PIL and
     report any pixel divergence with respect to the source.
     For 24-bit BI_RGB and 32-bit opaque-alpha BGRA cases PIL
     should agree; for 32-bit BGRA with non-trivial alpha PIL
     drops alpha to 0xFF on read so the divergence is
     expected and reported (not a fixture authoring error).

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

from PIL import Image


# BMP biCompression byte values.
_BI_RGB = 0
_BI_BITFIELDS = 3


# ---------------------------------------------------------------------------
# Source-image authoring (raw RGBA8 buffers)
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


def _checker_rgba_opaque(w: int, h: int, cell: int) -> bytes:
    """RGBA checkerboard with alpha=0xFF — suitable for 24-bit
    fixtures where the alpha channel is widened to 0xFF on read
    (matches the source)."""
    out = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            on = ((x // cell) + (y // cell)) % 2 == 0
            i = (y * w + x) * 4
            out[i:i + 4] = bytes(
                (255, 128, 0, 255) if on else (0, 128, 255, 255))
    return bytes(out)


def _solid_rgba(w: int, h: int, rgba: tuple[int, int, int, int]) -> bytes:
    """Solid RGBA fill — exercises zero-variance scanlines (the
    most common encoder bug-trigger for row-padding accounting)."""
    return bytes(rgba) * (w * h)


# ---------------------------------------------------------------------------
# Hand-rolled BMP writer
# ---------------------------------------------------------------------------
#
# Implements the BITMAPFILEHEADER + BITMAPINFOHEADER (biSize ==
# 40) layout for the two in-scope (biBitCount, biCompression)
# pairs:
#
#   - 24-bit BI_RGB: 3 bytes per pixel (B G R), rows padded up
#     to the next 4-byte boundary, no per-pixel alpha.
#   - 32-bit BI_BITFIELDS: 4 bytes per pixel (B G R A), 12-byte
#     channel mask block immediately after BITMAPINFOHEADER
#     (Red=0x00FF0000, Green=0x0000FF00, Blue=0x000000FF), no
#     row padding (every row is already a multiple of 4).
#
# Scanlines are stored bottom-to-top in the file (positive
# biHeight); the input RGBA buffer is row-major top-to-bottom
# so the writer reverses row order on emit.


_FILE_HEADER_SIZE = 14
_INFO_HEADER_SIZE = 40
_BITFIELDS_BLOCK_SIZE = 12

# Standard 32-bit BGRA channel masks. Red is in the high byte
# of the dword (byte offset +2 in little-endian), Green in the
# next byte (offset +1), Blue in the low byte (offset +0). Alpha
# is the byte at offset +3 — implicit in the 3-mask BITFIELDS
# form (BITMAPV4HEADER's 4-mask form with explicit AlphaMask is
# out of scope for v1).
_RED_MASK = 0x00FF0000
_GREEN_MASK = 0x0000FF00
_BLUE_MASK = 0x000000FF


def _write_bgr24_bmp(path: Path, width: int, height: int,
                     rgba: bytes) -> None:
    """Hand-emit a 24-bit BGR BMP at biCompression == BI_RGB.
    The alpha channel of the input RGBA buffer is dropped (BMP
    24-bit has no per-pixel alpha)."""
    if len(rgba) != width * height * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {width}*{height}*4 "
            f"= {width * height * 4}")

    row_data_bytes = width * 3
    row_padding = (4 - (row_data_bytes % 4)) % 4
    row_stride = row_data_bytes + row_padding
    pixel_data_bytes = height * row_stride
    bf_off_bits = _FILE_HEADER_SIZE + _INFO_HEADER_SIZE
    bf_size = bf_off_bits + pixel_data_bytes

    # BITMAPFILEHEADER (14 bytes).
    file_header = struct.pack("<2sIHHI",
                              b"BM",
                              bf_size,
                              0,                # bfReserved1
                              0,                # bfReserved2
                              bf_off_bits)

    # BITMAPINFOHEADER (40 bytes).
    info_header = struct.pack("<IiiHHIIiiII",
                              _INFO_HEADER_SIZE,    # biSize
                              width,                # biWidth
                              height,               # biHeight (+)
                              1,                    # biPlanes
                              24,                   # biBitCount
                              _BI_RGB,              # biCompression
                              pixel_data_bytes,     # biSizeImage
                              2835,                 # biXPelsPerMeter
                              2835,                 # biYPelsPerMeter
                              0,                    # biClrUsed
                              0)                    # biClrImportant

    # Pixel data — emit rows bottom-to-top with B G R per pixel
    # and zero padding bytes to align to 4 bytes.
    pad = bytes(row_padding)
    rows = bytearray(pixel_data_bytes)
    out_cursor = 0
    for src_y in range(height - 1, -1, -1):
        in_base = src_y * width * 4
        for x in range(width):
            i = in_base + x * 4
            r = rgba[i]
            g = rgba[i + 1]
            b = rgba[i + 2]
            rows[out_cursor]     = b
            rows[out_cursor + 1] = g
            rows[out_cursor + 2] = r
            out_cursor += 3
        rows[out_cursor:out_cursor + row_padding] = pad
        out_cursor += row_padding

    path.write_bytes(file_header + info_header + bytes(rows))


def _write_bgra32_bmp(path: Path, width: int, height: int,
                      rgba: bytes) -> None:
    """Hand-emit a 32-bit BGRA BMP at biCompression ==
    BI_BITFIELDS with the canonical standard channel masks. The
    input RGBA buffer's alpha byte is preserved on disk."""
    if len(rgba) != width * height * 4:
        raise ValueError(
            f"rgba length {len(rgba)} != {width}*{height}*4 "
            f"= {width * height * 4}")

    row_stride = width * 4   # 32-bit rows are always multiples of 4
    pixel_data_bytes = height * row_stride
    bf_off_bits = (_FILE_HEADER_SIZE + _INFO_HEADER_SIZE
                   + _BITFIELDS_BLOCK_SIZE)
    bf_size = bf_off_bits + pixel_data_bytes

    # BITMAPFILEHEADER (14 bytes).
    file_header = struct.pack("<2sIHHI",
                              b"BM",
                              bf_size,
                              0,
                              0,
                              bf_off_bits)

    # BITMAPINFOHEADER (40 bytes).
    info_header = struct.pack("<IiiHHIIiiII",
                              _INFO_HEADER_SIZE,
                              width,
                              height,
                              1,
                              32,
                              _BI_BITFIELDS,
                              pixel_data_bytes,
                              2835,
                              2835,
                              0,
                              0)

    # 12-byte BITFIELDS block — three uint32 LE channel masks.
    bitfields = struct.pack("<III",
                            _RED_MASK,
                            _GREEN_MASK,
                            _BLUE_MASK)

    # Pixel data — bottom-to-top with B G R A per pixel, no
    # row padding.
    rows = bytearray(pixel_data_bytes)
    out_cursor = 0
    for src_y in range(height - 1, -1, -1):
        in_base = src_y * width * 4
        for x in range(width):
            i = in_base + x * 4
            r = rgba[i]
            g = rgba[i + 1]
            b = rgba[i + 2]
            a = rgba[i + 3]
            rows[out_cursor]     = b
            rows[out_cursor + 1] = g
            rows[out_cursor + 2] = r
            rows[out_cursor + 3] = a
            out_cursor += 4

    path.write_bytes(file_header + info_header + bitfields + bytes(rows))


# ---------------------------------------------------------------------------
# PIL sanity-check witness — confirms the hand-emitted BMP
# parses through a third-party reader. Not the oracle; the
# oracle is the source RGBA buffer authored above. PIL drops
# alpha to 0xFF on 32-bit BI_BITFIELDS reads, which the
# decoder must NOT do — so PIL agreement is required only for
# variants where alpha widening to 0xFF matches the source.
# ---------------------------------------------------------------------------


def _pil_read_rgba(bmp_path: Path) -> tuple[bytes, int, int]:
    img = Image.open(bmp_path)
    rgba = img.convert("RGBA")
    return rgba.tobytes(), rgba.width, rgba.height


# ---------------------------------------------------------------------------
# Fixture catalog
# ---------------------------------------------------------------------------


# (filename, (width, height), kind, source_factory)
# kind 'bgr24'  → 24-bit BI_RGB, alpha widened to 0xFF on read.
# kind 'bgra32' → 32-bit BI_BITFIELDS, alpha preserved.
_FIXTURES = [
    # 24-bit BI_RGB — basic photometric coverage. Width 16 is
    # 48 data bytes per row (multiple of 4) → row_padding=0.
    ("bgr24_checker_16.bmp", (16, 16), "bgr24",
     lambda: _checker_rgba_opaque(16, 16, 4)),

    # 24-bit BI_RGB — solid color, exercises zero-variance
    # scanline edge case. Width 8 is 24 data bytes per row
    # (multiple of 4) → row_padding=0.
    ("bgr24_solid_8.bmp", (8, 8), "bgr24",
     lambda: _solid_rgba(8, 8, (200, 50, 75, 255))),

    # 24-bit BI_RGB — width 3 → 9 data bytes per row + 3
    # padding = 12 stride. Exercises the maximum-padding
    # row-stride path.
    ("bgr24_padded_3x4.bmp", (3, 4), "bgr24",
     lambda: _checker_rgba_opaque(3, 4, 1)),

    # 24-bit BI_RGB — width 2 → 6 data bytes per row + 2
    # padding = 8 stride. Exercises the 2-byte-padding case.
    ("bgr24_padded_2x3.bmp", (2, 3), "bgr24",
     lambda: _checker_rgba_opaque(2, 3, 1)),

    # 24-bit BI_RGB — width 5 → 15 data bytes per row + 1
    # padding = 16 stride. Exercises the 1-byte-padding case
    # (the easiest one to forget).
    ("bgr24_padded_5x4.bmp", (5, 4), "bgr24",
     lambda: _gradient_rgba(5, 4)),

    # 32-bit BI_BITFIELDS — non-trivial alpha gradient.
    # Verifies the BITFIELDS block is honoured AND alpha is
    # preserved through the BGRA channel translation.
    ("bgra32_gradient_16.bmp", (16, 16), "bgra32",
     lambda: _gradient_rgba(16, 16)),

    # 32-bit BI_BITFIELDS — opaque checker. Tests the basic
    # 32-bit decode path without the alpha-preservation
    # variable confounding the failure mode.
    ("bgra32_checker_8.bmp", (8, 8), "bgra32",
     lambda: _checker_rgba_opaque(8, 8, 2)),

    # 32-bit BI_BITFIELDS — solid color, zero-variance edge
    # case for the 32-bit branch.
    ("bgra32_solid_4.bmp", (4, 4), "bgra32",
     lambda: _solid_rgba(4, 4, (100, 150, 200, 64))),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------


def _expected_sidecar(rgba_src: bytes, kind: str) -> bytes:
    """Compute the .pixels sidecar bytes — the RGBA buffer the
    decoder must produce on a correct decode. For 24-bit BI_RGB
    the encoded BMP drops alpha and the decoder widens alpha to
    0xFF on read, so the expected sidecar is source.RGB with
    alpha forced to 0xFF. For 32-bit BI_BITFIELDS the encoded
    BMP preserves all 4 channels, so the expected sidecar is
    the source verbatim."""
    if kind == "bgr24":
        out = bytearray(rgba_src)
        for i in range(3, len(out), 4):
            out[i] = 0xFF
        return bytes(out)
    if kind == "bgra32":
        return rgba_src
    raise ValueError(f"unknown fixture kind {kind!r}")


def main() -> int:
    here = Path(__file__).parent

    for name, (w, h), kind, make_source in _FIXTURES:
        bmp_path = here / name
        sidecar_path = bmp_path.with_suffix(".pixels")

        rgba_src = make_source()
        if len(rgba_src) != w * h * 4:
            print(f"ERROR: {name}: source length {len(rgba_src)} "
                  f"!= {w}*{h}*4 = {w*h*4}")
            return 1

        if kind == "bgr24":
            _write_bgr24_bmp(bmp_path, w, h, rgba_src)
        elif kind == "bgra32":
            _write_bgra32_bmp(bmp_path, w, h, rgba_src)
        else:
            raise ValueError(f"unknown fixture kind {kind!r}")

        sidecar = _expected_sidecar(rgba_src, kind)
        sidecar_path.write_bytes(sidecar)

        # PIL sanity check — confirm a third-party reader can
        # parse the hand-emitted BMP. Divergences are tolerable
        # only on alpha bytes for 32-bit BGRA with non-trivial
        # alpha (PIL's BMP reader widens alpha to 0xFF), or
        # never for 24-bit (PIL handles BI_RGB perfectly).
        pil_bytes, pil_w, pil_h = _pil_read_rgba(bmp_path)
        if (pil_w, pil_h) != (w, h):
            print(f"ERROR: {name}: PIL reports {pil_w}x{pil_h}, "
                  f"expected {w}x{h}")
            return 1
        diffs = [i for i in range(len(sidecar))
                 if sidecar[i] != pil_bytes[i]]
        tolerable = (kind == "bgra32"
                     and all(i % 4 == 3 for i in diffs))
        bmp_bytes = bmp_path.read_bytes()
        bmp_digest = hashlib.sha256(bmp_bytes).hexdigest()[:16]
        px_digest = hashlib.sha256(sidecar).hexdigest()[:16]
        if diffs and not tolerable:
            print(f"ERROR: {name}: PIL diverges from source on "
                  f"{len(diffs)} non-alpha bytes; first diff at "
                  f"byte {diffs[0]} (src=0x{sidecar[diffs[0]]:02X}, "
                  f"pil=0x{pil_bytes[diffs[0]]:02X})")
            return 1
        note = " [PIL alpha-only divergence — tolerated]" if diffs else ""
        print(f"wrote {name:32s} "
              f"{len(bmp_bytes):6d} B bmp (sha256:{bmp_digest}…), "
              f"{w}x{h} rgba ({len(sidecar)} B, "
              f"sha256:{px_digest}…){note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
