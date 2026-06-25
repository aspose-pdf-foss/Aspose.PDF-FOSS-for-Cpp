#!/usr/bin/env python3
"""Generate the .bin / .pixels fixture corpus for foundation::jbig2.

Pipeline per fixture:

  1. Author a PBM bitmap (1 bpp packed MSB-first) describing the
     intended bilevel content.
  2. Encode the PBM into a JBIG2 embedded stream via the
     `jbig2` (jbig2enc) CLI in PDF mode (-p). PDF mode emits
     segments only, no JBIG2 file header — exactly what PDF
     /JBIG2Decode wraps and exactly what the C++ adapter expects.
  3. Decode the resulting JBIG2 stream via the `jbig2dec` CLI
     (the canonical reference decoder) back to a PBM, just to
     validate the encoder produced something well-formed before
     we commit it. The decoded PBM should byte-match the
     original PBM since JBIG2 generic-region coding is
     lossless.
  4. Strip the PBM header and emit:
       - .bin     = raw JBIG2 embedded stream bytes
       - .pixels  = packed MSB-first bilevel bytes
                     (((width + 7) / 8) * height bytes), the
                     same data jbig2dec produced minus PBM
                     framing.
     The C++ Decode walks the .bin and produces a DecodedImage
     whose .pixels matches this sidecar byte-for-byte.

Single-oracle authoring (matches dctdecode / icc_profile / jpx
/ rasterizer / glyph_rasterizer / image_compositor): jbig2dec
is the only oracle. JBIG2 output is bit-deterministic by ITU-T
T.88 + T.82 Annex E — independent decoders agree to the bit
on well-formed input. The "lossy" gate name and PSNR ≥ 50 dB
threshold are formalities; in practice every fixture is bit-
identical (PSNR=∞).

Re-run from this directory:

    python3 generate_fixtures.py

Both jbig2 (encoder, BSD-3) and jbig2dec (decoder, AGPL) are
invoked at fixture-authoring time only — the shipped foundation
library does not link against either.
"""

from __future__ import annotations

import struct
import subprocess
import tempfile
from pathlib import Path


# Encoder + decoder binaries. Both ship with their respective
# Homebrew formulas (`jbig2enc` and `jbig2dec`). The encoder
# binary is named `jbig2` (not `jbig2enc`) by the upstream
# package — historical artefact.
JBIG2_ENC = "jbig2"
JBIG2_DEC = "jbig2dec"


# ---------------------------------------------------------------------------
# PBM helpers — PBM format is the natural intermediate for both
# CLIs. A "PBM" file is `P4\n<width> <height>\n` followed by
# raw bilevel bytes packed MSB-first, ((w+7)/8) bytes per row.
# That packing matches our DecodedImage.pixels convention
# exactly, so the PBM body IS the .pixels sidecar.
# ---------------------------------------------------------------------------


def write_pbm(path: Path, width: int, height: int,
              packed_bits: bytes) -> None:
    """Write a P4 PBM file with the given packed-MSB-first bytes."""
    expected = ((width + 7) // 8) * height
    if len(packed_bits) != expected:
        raise ValueError(
            f"packed bits length {len(packed_bits)} != "
            f"expected {expected} for {width}x{height}")
    header = f"P4\n{width} {height}\n".encode()
    path.write_bytes(header + packed_bits)


def read_pbm(path: Path) -> tuple[int, int, bytes]:
    """Parse a P4 PBM file. Returns (width, height, packed_bits)."""
    raw = path.read_bytes()
    if not raw.startswith(b"P4\n"):
        raise ValueError(f"{path}: not a P4 PBM (header={raw[:8]!r})")
    # Skip the magic "P4\n", then read the width/height line, then
    # any comments. Real PBMs may have multiple whitespace-
    # separated tokens before the body; this parser handles only
    # the canonical "P4\n<w> <h>\n<bytes>" form jbig2dec emits.
    rest = raw[3:]
    nl = rest.index(b"\n")
    dims = rest[:nl].decode().split()
    width, height = int(dims[0]), int(dims[1])
    body = rest[nl + 1:]
    expected = ((width + 7) // 8) * height
    if len(body) != expected:
        raise ValueError(
            f"{path}: body length {len(body)} != "
            f"((({width}+7)/8) * {height}) = {expected}")
    return width, height, body


# ---------------------------------------------------------------------------
# Encoder + decoder shellouts.
# ---------------------------------------------------------------------------


def encode_jbig2(pbm_path: Path) -> bytes:
    """Run `jbig2 -p <pbm>` and return the embedded JBIG2 stream
    bytes from the encoder's stdout. -p selects "PDF mode": no
    JBIG2 file header, segments only — exactly the form PDF
    /JBIG2Decode wraps."""
    result = subprocess.run(
        [JBIG2_ENC, "-p", str(pbm_path)],
        capture_output=True, check=True)
    return result.stdout


def decode_jbig2(jb2_bytes: bytes) -> tuple[int, int, bytes]:
    """Round-trip via jbig2dec: feed jbig2 bytes (embedded form)
    to jbig2dec -e -o <out.pbm>, parse the resulting PBM."""
    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        in_path = tdp / "in.jb2"
        out_path = tdp / "out.pbm"
        in_path.write_bytes(jb2_bytes)
        subprocess.run(
            [JBIG2_DEC, "-e", "-q",
             "-t", "pbm",
             "-o", str(out_path), str(in_path)],
            check=True, capture_output=True)
        return read_pbm(out_path)


# ---------------------------------------------------------------------------
# Per-fixture authoring.
# ---------------------------------------------------------------------------


def emit_fixture(out_dir: Path, name: str,
                 width: int, height: int, packed_bits: bytes) -> None:
    """Encode the bitmap to JBIG2, validate via jbig2dec, write
    .bin (raw JBIG2 stream) + .pixels (packed bilevel bytes)."""
    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        pbm_in = tdp / f"{name}.pbm"
        write_pbm(pbm_in, width, height, packed_bits)
        jb2_bytes = encode_jbig2(pbm_in)

    # Sanity-check the round-trip. A real bug in jbig2enc would
    # produce a stream jbig2dec can't decode; better to fail at
    # fixture-authoring time than ship a corrupt fixture.
    rt_w, rt_h, rt_bits = decode_jbig2(jb2_bytes)
    if (rt_w, rt_h) != (width, height):
        raise RuntimeError(
            f"{name}: round-trip dimension mismatch "
            f"({width}x{height} → {rt_w}x{rt_h})")
    if rt_bits != packed_bits:
        raise RuntimeError(
            f"{name}: round-trip pixel mismatch — "
            f"jbig2enc/jbig2dec disagree on the bilevel content")

    (out_dir / f"{name}.bin").write_bytes(jb2_bytes)
    (out_dir / f"{name}.pixels").write_bytes(packed_bits)


# ---------------------------------------------------------------------------
# Bitmap pattern generators.
# ---------------------------------------------------------------------------


def all_color(width: int, height: int, black: bool) -> bytes:
    """Solid-colour bitmap. black=True produces all-1 bits;
    black=False produces all-0. Last byte of each row is masked
    so the bits past `width` are clamped to 0 per JBIG2's
    convention."""
    bytes_per_row = (width + 7) // 8
    extra_bits = bytes_per_row * 8 - width
    last_mask = (0xFF << extra_bits) & 0xFF if extra_bits > 0 else 0xFF
    out = bytearray()
    for _ in range(height):
        for col in range(bytes_per_row):
            v = 0xFF if black else 0x00
            if col == bytes_per_row - 1:
                v &= last_mask
            out.append(v)
    return bytes(out)


def horizontal_split(width: int, height: int, top_black: bool) -> bytes:
    """Top half one colour, bottom half the other."""
    half = height // 2
    return (all_color(width, half, top_black) +
            all_color(width, height - half, not top_black))


def vertical_split(width: int, height: int, left_black: bool) -> bytes:
    """Left half one colour, right half the other."""
    bytes_per_row = (width + 7) // 8
    half = width // 2
    out = bytearray()
    for _ in range(height):
        row_bits = []
        for x in range(width):
            in_left = x < half
            black = (in_left and left_black) or (not in_left and not left_black)
            row_bits.append(1 if black else 0)
        # Pack MSB-first.
        for col in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = col * 8 + bit
                if pixel_x < width and row_bits[pixel_x]:
                    byte |= 1 << (7 - bit)
            out.append(byte)
    return bytes(out)


def checkerboard(width: int, height: int, cell: int) -> bytes:
    """Two-colour checkerboard with `cell`-pixel squares."""
    bytes_per_row = (width + 7) // 8
    out = bytearray()
    for y in range(height):
        for col in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = col * 8 + bit
                if pixel_x >= width:
                    break
                cx = (pixel_x // cell) & 1
                cy = (y // cell) & 1
                black = (cx ^ cy) == 0
                if black:
                    byte |= 1 << (7 - bit)
            out.append(byte)
    return bytes(out)


def diagonal_lines(width: int, height: int, period: int) -> bytes:
    """Black diagonal lines repeating every `period` pixels."""
    bytes_per_row = (width + 7) // 8
    out = bytearray()
    for y in range(height):
        for col in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = col * 8 + bit
                if pixel_x >= width:
                    break
                if (pixel_x + y) % period == 0:
                    byte |= 1 << (7 - bit)
            out.append(byte)
    return bytes(out)


def alternating_columns(width: int, height: int) -> bytes:
    """Alternating black / white single-pixel-wide columns. Each
    byte is exactly 0xAA — the high-frequency pattern that
    catches MSB / LSB byte-packing mix-ups."""
    bytes_per_row = (width + 7) // 8
    out = bytearray()
    for _ in range(height):
        for col in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = col * 8 + bit
                if pixel_x >= width:
                    break
                if pixel_x % 2 == 0:  # even cols are black
                    byte |= 1 << (7 - bit)
            out.append(byte)
    return bytes(out)


def single_pixel(width: int, height: int,
                 px: int, py: int, black: bool) -> bytes:
    """One pixel set to `black`, all others the opposite. Tests
    sparse high-entropy regions."""
    base = 0x00 if black else 0xFF
    bytes_per_row = (width + 7) // 8
    extra_bits = bytes_per_row * 8 - width
    last_mask = (0xFF << extra_bits) & 0xFF if extra_bits > 0 else 0xFF
    out = bytearray()
    for y in range(height):
        for col in range(bytes_per_row):
            v = base
            if col == bytes_per_row - 1:
                v &= last_mask
            out.append(v)

    if 0 <= px < width and 0 <= py < height:
        col = px // 8
        bit = 7 - (px % 8)
        idx = py * bytes_per_row + col
        if black:
            out[idx] |= 1 << bit
        else:
            out[idx] &= 0xFF ^ (1 << bit)
    return bytes(out)


def stress_top_left(width: int, height: int) -> bytes:
    """Dense activity in the top-left corner where the JBIG2
    template references "above the image" virtual-white pixels.
    Surfaces the template's out-of-bounds pixel handling per
    T.88 §6.2.5.3."""
    out = bytearray()
    bytes_per_row = (width + 7) // 8
    for y in range(height):
        for col in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = col * 8 + bit
                if pixel_x >= width:
                    break
                if y < 4 and pixel_x < 4 and (pixel_x + y) % 2 == 0:
                    byte |= 1 << (7 - bit)
                elif y < 8 and pixel_x < 8:
                    byte |= 1 << (7 - bit) if (pixel_x ^ y) & 1 else 0
            out.append(byte)
    return bytes(out)


# ---------------------------------------------------------------------------
# Top-level corpus assembly.
# ---------------------------------------------------------------------------


def main() -> None:
    here = Path(__file__).resolve().parent
    for stale in here.glob("*.bin"):
        stale.unlink()
    for stale in here.glob("*.pixels"):
        stale.unlink()

    # ---- Solid colour boundary cases -----------------------------
    # All-white: lowest-entropy generic region; tests the MQ
    # coder's "MPS run" optimisation path.
    emit_fixture(here, "solid_white_16x16",
                 16, 16, all_color(16, 16, black=False))
    # All-black.
    emit_fixture(here, "solid_black_16x16",
                 16, 16, all_color(16, 16, black=True))
    # Larger all-white (exercises multi-row arithmetic-coded
    # state).
    emit_fixture(here, "solid_white_64x64",
                 64, 64, all_color(64, 64, black=False))

    # ---- Half-half splits ----------------------------------------
    # Top half black, bottom half white. Tests scanline
    # boundary handling.
    emit_fixture(here, "horizontal_split_16x16",
                 16, 16, horizontal_split(16, 16, top_black=True))
    # Left half black, right half white. Tests within-scanline
    # transitions.
    emit_fixture(here, "vertical_split_16x16",
                 16, 16, vertical_split(16, 16, left_black=True))

    # ---- High-entropy patterns -----------------------------------
    # 2-pixel checkerboard. High-entropy region with predictable
    # context patterns.
    emit_fixture(here, "checker_2px_16x16",
                 16, 16, checkerboard(16, 16, cell=2))
    # 4-pixel checkerboard at larger scale.
    emit_fixture(here, "checker_4px_32x32",
                 32, 32, checkerboard(32, 32, cell=4))
    # Alternating-column 0xAA pattern. Catches MSB / LSB byte-
    # packing mix-ups (semantic_traps.msb_first_packing_endianness).
    emit_fixture(here, "alternating_columns_16x16",
                 16, 16, alternating_columns(16, 16))

    # ---- Diagonal lines ------------------------------------------
    # Sparse diagonal pattern, period 4.
    emit_fixture(here, "diagonal_4_32x32",
                 32, 32, diagonal_lines(32, 32, period=4))

    # ---- Sparse single-pixel cases -------------------------------
    # Single black pixel near image origin.
    emit_fixture(here, "single_pixel_top_left",
                 16, 16, single_pixel(16, 16, 0, 0, black=True))
    # Single white pixel on black background — tests the
    # complement context path.
    emit_fixture(here, "single_white_pixel_on_black",
                 16, 16, single_pixel(16, 16, 7, 7, black=False))

    # ---- Top-left stress -----------------------------------------
    # Dense activity right where the generic-region template
    # references virtual-white pixels above the image
    # (semantic_traps.pixel_above_image_is_white).
    emit_fixture(here, "stress_top_left_16x16",
                 16, 16, stress_top_left(16, 16))

    # ---- Larger sizes (multi-segment / non-byte-aligned widths) --
    # Width that's NOT a multiple of 8 — exercises the trailing-
    # bits clamp in the MSB-first packing.
    emit_fixture(here, "checker_3px_27x27",
                 27, 27, checkerboard(27, 27, cell=3))

    n_bin = len(list(here.glob("*.bin")))
    n_pix = len(list(here.glob("*.pixels")))
    print(f"wrote {n_bin} fixtures + {n_pix} sidecars to {here}")


if __name__ == "__main__":
    main()
