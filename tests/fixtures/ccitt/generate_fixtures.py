#!/usr/bin/env python3
"""Generate the CCITT fixture corpus for the foundation/ccitt
primitive.

Each fixture is a self-describing ".ccfix" file: 16-byte CFAX
header (magic + version + Params) followed by the raw CCITT
body. The matching ".canonical" sidecar carries the agreed
text dump of the decoded bilevel raster. The C++ freeze gate
reads the .ccfix, decodes it, and byte-compares its
ToCanonical output against the sidecar.

Two oracles certify each .canonical at authoring time:

  * libtiff (via Pillow) — encodes the source bilevel as a
    single-strip G3/G4 TIFF, extracts the raw CCITT bytes from
    the strip, then re-decodes the same TIFF wrapper through
    Pillow's reader. Round-trip must equal the source bilevel.

  * mupdf (via mutool draw) — wraps the extracted CCITT body
    in a minimal one-page PDF that uses /CCITTFaxDecode with
    the matching /DecodeParms, renders the page to PBM at the
    image's pixel resolution, and converts back to bilevel.
    mupdf has its own CCITT decoder independent of libtiff.

Both oracles' decoded bilevel is byte-compared to the source.
A disagreement aborts the script — fixtures only ship when
both decoders, on entirely different code paths, agree on
the result.

Run from this directory:
    python3 generate_fixtures.py

Requirements: Python 3.9+, Pillow (with libtiff support),
``mutool`` on PATH (mupdf-tools).
"""

from __future__ import annotations

import hashlib
import io
import struct
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from PIL import Image


@dataclass
class Params:
    K: int = 0
    EndOfLine: bool = False
    EncodedByteAlign: bool = False
    EndOfBlock: bool = True
    BlackIs1: bool = False
    Columns: int = 1728
    Rows: int = 0
    DamagedRowsBeforeError: int = 0


CFAX_MAGIC = b"CFAX"
CFAX_VERSION = 1


def pack_ccfix(params: Params, ccitt_body: bytes) -> bytes:
    """Build a self-describing .ccfix file: header + body."""
    flags = 0
    if params.EndOfLine: flags |= 0x01
    if params.EncodedByteAlign: flags |= 0x02
    if params.EndOfBlock: flags |= 0x04
    if params.BlackIs1: flags |= 0x08
    if not (-128 <= params.K <= 127):
        raise ValueError(f"K out of int8 range: {params.K}")
    if not (0 <= params.DamagedRowsBeforeError <= 255):
        raise ValueError(
            f"DamagedRowsBeforeError out of uint8 range: "
            f"{params.DamagedRowsBeforeError}")
    if not (0 <= params.Columns <= 0xFFFF):
        raise ValueError(f"Columns out of uint16 range: {params.Columns}")
    if not (0 <= params.Rows <= 0xFFFFFFFF):
        raise ValueError(f"Rows out of uint32 range: {params.Rows}")
    if len(ccitt_body) > 0xFFFF:
        raise ValueError(
            f"encoded body too large for v1 framing "
            f"(uint16 length): {len(ccitt_body)}")
    header = (
        CFAX_MAGIC
        + bytes([CFAX_VERSION])
        + struct.pack("b", params.K)
        + bytes([flags])
        + bytes([params.DamagedRowsBeforeError])
        + struct.pack(">H", params.Columns)
        + struct.pack(">I", params.Rows)
        + struct.pack(">H", len(ccitt_body))
    )
    assert len(header) == 16, len(header)
    return header + ccitt_body


# ---------------------------------------------------------------------------
# Source-bilevel helpers. Each fixture defines a callable that returns
# (width, height, bits) where bits is a flat list of 0/1 — 1 = white,
# 0 = black (PIL mode '1' convention; matches TIFF photometric=1).
# ---------------------------------------------------------------------------


def _bilevel_to_image(width: int, height: int,
                      bits: list[int]) -> Image.Image:
    img = Image.new("1", (width, height), 1)
    px = img.load()
    for r in range(height):
        for c in range(width):
            px[c, r] = 255 if bits[r * width + c] else 0
    return img


def _image_to_bilevel(img: Image.Image) -> list[int]:
    width, height = img.size
    px = img.load()
    out = []
    for r in range(height):
        for c in range(width):
            out.append(1 if px[c, r] else 0)
    return out


# ---------------------------------------------------------------------------
# libtiff oracle — encode via Pillow, extract raw CCITT body from the
# TIFF strip, round-trip back through Pillow to verify.
# ---------------------------------------------------------------------------


def _extract_strip_from_tiff(tiff_data: bytes) -> bytes:
    """Pull StripOffsets[0]..StripOffsets[0]+StripByteCounts[0] out of
    a single-strip TIFF. Pillow's TIFF encoder uses RowsPerStrip ==
    image height for our small fixtures, so there's exactly one
    strip — assert that and return its bytes verbatim."""
    if tiff_data[:2] != b"II":
        raise RuntimeError("expected little-endian TIFF (II header)")
    if struct.unpack("<H", tiff_data[2:4])[0] != 42:
        raise RuntimeError("bad TIFF magic")
    ifd_off = struct.unpack("<I", tiff_data[4:8])[0]
    ne = struct.unpack("<H", tiff_data[ifd_off : ifd_off + 2])[0]
    tags: dict[int, tuple[int, int, bytes]] = {}
    for i in range(ne):
        p = ifd_off + 2 + i * 12
        tag, typ, cnt = struct.unpack("<HHI", tiff_data[p : p + 8])
        val_bytes = tiff_data[p + 8 : p + 12]
        tags[tag] = (typ, cnt, val_bytes)

    def read_one(tag_id: int) -> int:
        typ, cnt, vb = tags[tag_id]
        if cnt != 1:
            raise RuntimeError(
                f"tag {tag_id} multi-valued (cnt={cnt}), "
                "expected single-strip TIFF")
        if typ == 3:  # SHORT
            return struct.unpack("<H", vb[:2])[0]
        if typ == 4:  # LONG
            return struct.unpack("<I", vb)[0]
        raise RuntimeError(
            f"tag {tag_id} unexpected type {typ}")

    strip_off = read_one(273)
    strip_bc = read_one(279)
    return tiff_data[strip_off : strip_off + strip_bc]


def encode_libtiff_g4(width: int, height: int,
                      bits: list[int]) -> bytes:
    """Encode the bilevel raster as a Group 4 TIFF strip and return
    the raw CCITT body. Pillow's libtiff defaults: T6Options=0
    (no uncompressed segments), no per-row markers, no trailing
    EOFB — the body is exactly the encoded run-length codes."""
    img = _bilevel_to_image(width, height, bits)
    buf = io.BytesIO()
    img.save(buf, format="TIFF", compression="group4")
    return _extract_strip_from_tiff(buf.getvalue())


def encode_libtiff_g3(width: int, height: int,
                      bits: list[int]) -> bytes:
    """Encode as G3 1D TIFF strip. Pillow's libtiff defaults:
    T4Options=0 (pure 1D, no byte-align, no uncompressed). No EOL
    codes between rows, no trailing RTC."""
    img = _bilevel_to_image(width, height, bits)
    buf = io.BytesIO()
    img.save(buf, format="TIFF", compression="group3")
    return _extract_strip_from_tiff(buf.getvalue())


def decode_libtiff(ccitt_body: bytes, width: int, height: int,
                   k: int) -> list[int]:
    """Re-wrap the raw CCITT body as a single-strip TIFF and decode
    through Pillow. Used as the fixture-side round-trip oracle."""
    compression = 4 if k < 0 else 3
    body_off = 8
    body_len = len(ccitt_body)
    ifd_off = body_off + body_len
    # Same tag set Pillow itself uses, in tag-numerical order.
    tag_data = [
        (256, 3, 1, struct.pack("<H", width) + b"\x00\x00"),
        (257, 3, 1, struct.pack("<H", height) + b"\x00\x00"),
        (258, 3, 1, struct.pack("<H", 1) + b"\x00\x00"),
        (259, 3, 1, struct.pack("<H", compression) + b"\x00\x00"),
        (262, 3, 1, struct.pack("<H", 1) + b"\x00\x00"),  # BlackIsZero
        (273, 4, 1, struct.pack("<I", body_off)),
        (277, 3, 1, struct.pack("<H", 1) + b"\x00\x00"),
        (278, 3, 1, struct.pack("<H", height) + b"\x00\x00"),
        (279, 4, 1, struct.pack("<I", body_len)),
        (284, 3, 1, struct.pack("<H", 1) + b"\x00\x00"),
    ]
    tag_data.sort()
    header = b"II" + struct.pack("<H", 42) + struct.pack("<I", ifd_off)
    ifd = struct.pack("<H", len(tag_data))
    for tag, typ, cnt, vb in tag_data:
        ifd += struct.pack("<HHI", tag, typ, cnt) + vb
    ifd += struct.pack("<I", 0)
    tiff = header + ccitt_body + ifd
    img = Image.open(io.BytesIO(tiff))
    img.load()
    return _image_to_bilevel(img)


# ---------------------------------------------------------------------------
# mupdf oracle — wrap the CCITT body in a minimal PDF and run mutool.
# ---------------------------------------------------------------------------


def _build_minimal_ccitt_pdf(ccitt_body: bytes,
                             params: Params) -> bytes:
    """Build a one-page PDF whose only image stream is the supplied
    CCITT body. The page MediaBox matches the image size 1:1 so a
    72-dpi render produces width × height pixels exactly."""
    width, height = params.Columns, params.Rows
    if width == 0 or height == 0:
        raise RuntimeError(
            "mupdf oracle needs concrete Columns/Rows in params")

    parts: list[bytes] = []
    objs: list[int] = []

    def add(s: bytes) -> None:
        objs.append(sum(len(p) for p in parts))
        parts.append(s)

    parts.append(b"%PDF-1.4\n%\xb6\xb5\xb6\xb6\n")
    add(b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n")
    add(b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n")
    add(
        f"3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox "
        f"[0 0 {width} {height}] /Contents 4 0 R /Resources "
        f"<< /XObject << /Im0 5 0 R >> >> >>\nendobj\n".encode()
    )
    contents = (
        f"q {width} 0 0 {height} 0 0 cm /Im0 Do Q\n"
    ).encode()
    add(
        f"4 0 obj\n<< /Length {len(contents)} >>\nstream\n".encode()
        + contents
        + b"endstream\nendobj\n"
    )

    dp = [f"/K {params.K}",
          f"/Columns {width}",
          f"/Rows {height}"]
    if params.EndOfLine:
        dp.append("/EndOfLine true")
    if params.EncodedByteAlign:
        dp.append("/EncodedByteAlign true")
    if not params.EndOfBlock:
        dp.append("/EndOfBlock false")
    if params.BlackIs1:
        dp.append("/BlackIs1 true")
    if params.DamagedRowsBeforeError:
        dp.append(
            f"/DamagedRowsBeforeError {params.DamagedRowsBeforeError}")
    dp_str = "<< " + " ".join(dp) + " >>"

    add(
        f"5 0 obj\n<< /Type /XObject /Subtype /Image /Width {width} "
        f"/Height {height} /BitsPerComponent 1 /ColorSpace /DeviceGray "
        f"/Filter /CCITTFaxDecode /DecodeParms {dp_str} "
        f"/Length {len(ccitt_body)} >>\nstream\n".encode()
        + ccitt_body
        + b"\nendstream\nendobj\n"
    )

    body = b"".join(parts)
    xref_off = len(body)
    n = len(objs) + 1
    xref = f"xref\n0 {n}\n0000000000 65535 f \n".encode()
    for off in objs:
        xref += f"{off:010d} 00000 n \n".encode()
    trailer = (
        f"trailer\n<< /Size {n} /Root 1 0 R >>\nstartxref\n"
        f"{xref_off}\n%%EOF\n"
    ).encode()
    return body + xref + trailer


def decode_mupdf(ccitt_body: bytes, params: Params) -> list[int]:
    """Decode the CCITT body through mupdf and return the bilevel
    raster as a list of 0/1 (1 = white, 0 = black, matching the
    fixture's source-side convention).

    mupdf renders the page to a PBM (P4) at the image's exact
    pixel resolution. PBM convention is 1=black, 0=white — so we
    invert the rendered bits to recover the source convention.

    Note: in PDF, /BlackIs1=true means the encoded bit value 1
    represents BLACK on output. With BlackIs1=true the rendered
    PBM matches the source bilevel directly; with BlackIs1=false
    (the PDF default — counter-intuitive!) the rendered output
    is bit-inverted relative to the source. The fixture caller is
    responsible for setting BlackIs1 appropriately so the rendered
    PBM matches the intended source.
    """
    pdf_data = _build_minimal_ccitt_pdf(ccitt_body, params)
    width, height = params.Columns, params.Rows
    with tempfile.TemporaryDirectory() as tmp:
        pdf_path = Path(tmp) / "fixture.pdf"
        out_path = Path(tmp) / "out.pbm"
        pdf_path.write_bytes(pdf_data)
        result = subprocess.run(
            [
                "mutool", "draw", "-q", "-r", "72",
                "-F", "pbm", "-w", str(width), "-h", str(height),
                "-f", "-o", str(out_path), str(pdf_path), "1",
            ],
            capture_output=True,
            timeout=30,
        )
        if result.returncode != 0:
            raise RuntimeError(
                f"mutool draw failed (exit {result.returncode}): "
                f"{result.stderr.decode(errors='replace')}")
        pbm = out_path.read_bytes()

    # Parse PBM (P4): magic, dims, raster.
    lines = pbm.split(b"\n", 2)
    if lines[0] != b"P4":
        raise RuntimeError(f"unexpected PBM magic: {lines[0]!r}")
    raster = lines[2]
    bytes_per_row = (width + 7) // 8

    # PBM packs MSB-first per row; 1 = black, 0 = white. We invert
    # to the fixture's source convention (1 = white) below.
    bits: list[int] = []
    for r in range(height):
        row = raster[r * bytes_per_row : (r + 1) * bytes_per_row]
        for c in range(width):
            bit = (row[c // 8] >> (7 - c % 8)) & 1
            bits.append(0 if bit else 1)
    return bits


# ---------------------------------------------------------------------------
# Canonical text dump (matches the format the C++ ToCanonical emits).
# ---------------------------------------------------------------------------


def _canonical(width: int, height: int,
               bits: list[int], black_is_1: bool) -> str:
    """Pack the bilevel raster into the gate's canonical text
    format: 'width=<W>\\nheight=<H>\\nrow N <hex>\\n…'

    Polarity follows BlackIs1: when black_is_1 is true, the packed
    bytes have bit 1 = black, bit 0 = white. When false (PDF's
    default), they have bit 1 = white, bit 0 = black.
    """
    out = [f"width={width}", f"height={height}"]
    bytes_per_row = (width + 7) // 8
    for r in range(height):
        row = bytearray(bytes_per_row)
        for c in range(width):
            src = bits[r * width + c]  # 1 = white in source convention
            if black_is_1:
                bit = 0 if src else 1  # black=1
            else:
                bit = 1 if src else 0  # white=1
            row[c // 8] |= bit << (7 - c % 8)
        out.append(f"row {r} {row.hex()}")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# Fixture catalogue. Each entry returns (Params, source bilevel bits,
# encoded ccitt body). The source bilevel is supplied by a small
# generator function; the body comes from libtiff via Pillow.
# ---------------------------------------------------------------------------


def _all_white(width: int, height: int) -> list[int]:
    return [1] * (width * height)


def _diagonal_4(width: int, height: int) -> list[int]:
    """(r + c) % 4 == 0 → black. Short black runs on every row,
    exercises white→black→white run alternation and zero-length
    runs at row starts."""
    bits = []
    for r in range(height):
        for c in range(width):
            bits.append(0 if (r + c) % 4 == 0 else 1)
    return bits


def _hashes(width: int, height: int) -> list[int]:
    """Vertical hashes every 8 columns + horizontal hashes every 8
    rows. Long-ish black runs (8+ pixels) and longer white runs
    between hashes — exercises make-up codes when width > 64."""
    bits = []
    for r in range(height):
        for c in range(width):
            black = (c % 8 == 0) or (r % 8 == 0)
            bits.append(0 if black else 1)
    return bits


def _checker_blocks(width: int, height: int,
                    block: int = 16) -> list[int]:
    """Big checkerboard with `block`-sized squares. Generates very
    long black/white runs (≥ block pixels) — for width=128, block=16,
    runs cross the 64-pixel make-up-code boundary multiple times."""
    bits = []
    for r in range(height):
        for c in range(width):
            cell = ((r // block) + (c // block)) % 2
            bits.append(1 if cell == 0 else 0)
    return bits


def _text_like(width: int, height: int) -> list[int]:
    """Pseudo-text: alternating short black ‘glyphs’ on a white
    background, irregular widths/spacings to stress varying run
    lengths — closer to a real scanned page than a regular pattern.
    Deterministic LCG so the fixture is reproducible."""
    state = 0xC0FFEE
    def rng_next():
        nonlocal state
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        return state
    bits = [1] * (width * height)
    cursor_x = 4
    for line in range(height // 12):
        y = line * 12 + 2
        if y + 8 >= height:
            break
        x = cursor_x
        while x + 16 < width:
            glyph_w = 4 + (rng_next() % 6)   # 4..9
            glyph_h = 6 + (rng_next() % 3)   # 6..8
            for rr in range(glyph_h):
                for cc in range(glyph_w):
                    if y + rr < height and x + cc < width:
                        if rng_next() % 4 != 0:
                            bits[(y + rr) * width + (x + cc)] = 0
            x += glyph_w + 2 + (rng_next() % 4)
        cursor_x = 4 + (rng_next() % 4)
    return bits


# Each entry: name, factory closure -> (Params, source bilevel bits).
@dataclass
class FixtureSpec:
    name: str
    width: int
    height: int
    k: int
    black_is_1: bool
    bits_factory: Callable[[int, int], list[int]]
    description: str


_FIXTURES: list[FixtureSpec] = [
    FixtureSpec(
        name="g4_8x4_white",
        width=8, height=4, k=-1, black_is_1=True,
        bits_factory=_all_white,
        description=(
            "Smallest non-trivial G4 case: 8×4 all-white. The "
            "encoder emits a Vertical-V0 (1) for every row "
            "(reference change matches the right-edge sentinel "
            "at column 8). Tests the all-white path and the "
            "imaginary-all-white reference line for row 0."),
    ),
    FixtureSpec(
        name="g4_32x16_diag",
        width=32, height=16, k=-1, black_is_1=True,
        bits_factory=_diagonal_4,
        description=(
            "32×16 diagonal pattern: black pixel every 4 columns, "
            "advancing one per row. Exercises every 2D mode "
            "(Pass / Horizontal / Vertical V_R / V_L) on rows "
            "1..15 and the all-white reference line on row 0."),
    ),
    FixtureSpec(
        name="g4_64x32_hashes",
        width=64, height=32, k=-1, black_is_1=True,
        bits_factory=_hashes,
        description=(
            "64×32 vertical+horizontal 8-pixel hashes. Width hits "
            "the 64-pixel boundary so encoder must emit white "
            "make-up codes for the longest white runs (56 pixels) "
            "AND terminating codes for residuals. Horizontal "
            "rows (every 8th) are entirely black — long black-run "
            "make-up + terminator path."),
    ),
    FixtureSpec(
        name="g4_128x64_checker",
        width=128, height=64, k=-1, black_is_1=True,
        bits_factory=_checker_blocks,
        description=(
            "128×64 16×16 checkerboard. Runs of exactly 16 black "
            "and 16 white per row (no make-up needed) but the "
            "row-to-row reference structure changes every 16 "
            "rows — exercises 2D mode with non-trivial reference "
            "transitions. Largest fixture in v1."),
    ),
    FixtureSpec(
        name="g4_120x80_text",
        width=120, height=80, k=-1, black_is_1=True,
        bits_factory=_text_like,
        description=(
            "120×80 synthetic text-like content: small irregular "
            "‘glyphs’ scattered on a white background. Run "
            "lengths vary widely; reference-line transitions are "
            "irregular — closer to a real-world fax page than the "
            "geometric fixtures."),
    ),
    FixtureSpec(
        name="g4_32x16_inverted",
        width=32, height=16, k=-1, black_is_1=False,
        bits_factory=_diagonal_4,
        description=(
            "Same diagonal as g4_32x16_diag but with the PDF "
            "default polarity (BlackIs1=false). Asserts the "
            "decoder's output is bit-inverted relative to the "
            "BlackIs1=true variant — the canonical bytes here "
            "are 0xFF XOR of the diag canonical."),
    ),
    FixtureSpec(
        name="g3_32x16_diag",
        width=32, height=16, k=0, black_is_1=True,
        bits_factory=_diagonal_4,
        description=(
            "G3 1D version of g4_32x16_diag. Same source raster, "
            "encoded purely from the 1D white/black run-length "
            "code tables — no 2D modes, no per-row EOL (Pillow's "
            "default for compression='group3'), no RTC."),
    ),
    FixtureSpec(
        name="g3_64x32_hashes",
        width=64, height=32, k=0, black_is_1=True,
        bits_factory=_hashes,
        description=(
            "G3 1D version of g4_64x32_hashes. Same raster, "
            "encoded as pure 1D — exercises white and black "
            "make-up code paths in the 1D table."),
    ),
]


# ---------------------------------------------------------------------------
# Driver: build each fixture, run both oracles, write .ccfix +
# .canonical.
# ---------------------------------------------------------------------------


def _build_one(spec: FixtureSpec) -> tuple[bytes, str]:
    bits = spec.bits_factory(spec.width, spec.height)
    if spec.k < 0:
        ccitt_body = encode_libtiff_g4(spec.width, spec.height, bits)
    elif spec.k == 0:
        ccitt_body = encode_libtiff_g3(spec.width, spec.height, bits)
    else:
        raise NotImplementedError(
            "K > 0 (Group 3 mixed 1D/2D) not yet generated — Pillow "
            "doesn't expose the T4Options 2D-coding flag and the "
            "v1 corpus skips this branch.")

    # Oracle 1: libtiff round-trip via Pillow.
    rt_libtiff = decode_libtiff(
        ccitt_body, spec.width, spec.height, spec.k)
    if rt_libtiff != bits:
        raise RuntimeError(
            f"{spec.name}: libtiff round-trip mismatch — encoded body "
            "does not decode back to source bilevel")

    # Oracle 2: mupdf via mutool draw. We always set BlackIs1=true on
    # the PDF wrapper so the rendered PBM directly matches the source
    # convention regardless of the fixture's BlackIs1 setting (the
    # fixture's BlackIs1 affects only the OUTPUT polarity our C++
    # decoder emits — not the underlying CCITT codes, which are the
    # same in both polarities).
    mupdf_params = Params(
        K=spec.k,
        Columns=spec.width,
        Rows=spec.height,
        EndOfBlock=False,
        BlackIs1=True,
    )
    rt_mupdf = decode_mupdf(ccitt_body, mupdf_params)
    if rt_mupdf != bits:
        raise RuntimeError(
            f"{spec.name}: mupdf disagrees with source bilevel "
            "(libtiff agreed)")

    # Pillow's libtiff Group 3 1D encoder always writes an EOL code
    # at the head of every row (per T.4 spec). Group 4 emits no
    # per-row EOL. Reflect that in the fixture's CFAX framing so the
    # decoder skips the leading EOL fill when consuming a G3 row.
    end_of_line = spec.k == 0
    params = Params(
        K=spec.k,
        Columns=spec.width,
        Rows=spec.height,
        EndOfLine=end_of_line,
        EndOfBlock=False,
        BlackIs1=spec.black_is_1,
    )
    ccfix_blob = pack_ccfix(params, ccitt_body)
    canonical = _canonical(spec.width, spec.height, bits,
                           spec.black_is_1)
    return ccfix_blob, canonical


def main() -> int:
    here = Path(__file__).parent
    for spec in _FIXTURES:
        ccfix, canonical = _build_one(spec)
        ccfix_path = here / f"{spec.name}.ccfix"
        canon_path = here / f"{spec.name}.canonical"
        ccfix_path.write_bytes(ccfix)
        canon_path.write_text(canonical)
        digest = hashlib.sha256(ccfix).hexdigest()[:16]
        print(
            f"wrote {spec.name:24s} {len(ccfix):5d}B  "
            f"sha256:{digest}…  {spec.description.splitlines()[0][:50]}"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
