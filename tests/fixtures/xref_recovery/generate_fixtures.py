#!/usr/bin/env python3
"""Generate the xref-recovery gtest fixtures.

These exercise xref v3 behaviours that the two_oracle_agreement freeze
corpus deliberately cannot cover: qpdf and mutool diverge on broken-xref
recovery (qpdf's --json adapter errors; mutool reconstructs with a
different /Size), so the agreement gate excludes them. They are gated
instead by the library's own unit test (tests/xref_recovery_smoke_test.cpp).

  * recon_stale.pdf   — well-formed objects + a classical xref, but the
    startxref offset is wrong (points mid-header). The primary parse
    fails; reconstruction recovers root + all object offsets.
  * recon_badtype.pdf — classical xref whose first in-use entry carries a
    non-conforming type byte (0x02, neither 'n' nor 'f'). The classical
    parse throws; reconstruction recovers.
  * length_objstm.pdf — a /XRef-stream PDF with two objects compressed in
    an /ObjStm, whose FlateDecode body ends in a 0x0A byte. Honouring
    /Length keeps that byte; the old scan-to-endstream + whitespace-trim
    truncated it, so the stream failed to decode. The compressed entries
    are recoverable ONLY via the stream (reconstruction scans direct
    `N G obj` headers and cannot see ObjStm-packed objects), so they
    prove the /Length path ran.

Run from this directory: python3 generate_fixtures.py
"""
from __future__ import annotations

import struct
import zlib
from pathlib import Path

HERE = Path(__file__).parent

_OBJS = [
    b"<</Type/Catalog/Pages 2 0 R>>",
    b"<</Type/Pages/Kids[3 0 R]/Count 1>>",
    b"<</Type/Page/Parent 2 0 R/MediaBox[0 0 100 100]>>",
]


def _classical(startxref_override=None, corrupt_type=False) -> bytes:
    out = bytearray(b"%PDF-1.4\n")
    offs = []
    for i, b in enumerate(_OBJS, 1):
        offs.append(len(out))
        out += f"{i} 0 obj\n".encode() + b + b"\nendobj\n"
    xref_pos = len(out)
    n = len(_OBJS)
    out += f"xref\n0 {n + 1}\n".encode()
    out += b"0000000000 65535 f \n"
    for k, o in enumerate(offs):
        typ = b"\x02" if (corrupt_type and k == 0) else b"n"
        out += f"{o:010d} 00000 ".encode() + typ + b" \n"
    out += b"trailer\n<</Size " + str(n + 1).encode() + b"/Root 1 0 R>>\n"
    sx = startxref_override if startxref_override is not None else xref_pos
    out += b"startxref\n" + str(sx).encode() + b"\n%%EOF\n"
    return bytes(out)


_WS = {0x00, 0x09, 0x0A, 0x0C, 0x0D, 0x20}


def _length_objstm(pad: int):
    out = bytearray(b"%PDF-1.5\n%\xe2\xe3\xcf\xd3\n%" + b"x" * pad + b"\n")
    o1 = b"<</Type/Catalog/Pages 2 0 R>>"
    o2 = b"<</Type/Pages/Kids[3 0 R]/Count 1>>"
    hdr = f"1 0 2 {len(o1) + 1} ".encode()
    objstm_z = zlib.compress(hdr + o1 + b" " + o2)
    off4 = len(out)
    out += (b"4 0 obj <</Type/ObjStm/N 2/First " + str(len(hdr)).encode()
            + b"/Filter/FlateDecode/Length " + str(len(objstm_z)).encode()
            + b">>\nstream\n" + objstm_z + b"\nendstream endobj\n")
    off3 = len(out)
    out += b"3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 100 100]>> endobj\n"
    off5 = len(out)
    rows = [
        bytes([0]) + struct.pack(">H", 0) + bytes([255]),     # 0 free
        bytes([2]) + struct.pack(">H", 4) + bytes([0]),       # 1 compressed
        bytes([2]) + struct.pack(">H", 4) + bytes([1]),       # 2 compressed
        bytes([1]) + struct.pack(">H", off3) + bytes([0]),    # 3 inuse
        bytes([1]) + struct.pack(">H", off4) + bytes([0]),    # 4 inuse objstm
        bytes([1]) + struct.pack(">H", off5) + bytes([0]),    # 5 inuse xref
    ]
    cz = zlib.compress(b"".join(rows))
    if cz[-1] not in _WS:
        return None
    out += (b"5 0 obj <</Type/XRef/Size 6/Root 1 0 R/W[1 2 1]"
            b"/Filter/FlateDecode/Length " + str(len(cz)).encode()
            + b">>\nstream\n" + cz + b"\nendstream endobj\n")
    out += b"startxref\n" + str(off5).encode() + b"\n%%EOF\n"
    return bytes(out)


def main() -> None:
    (HERE / "recon_stale.pdf").write_bytes(_classical(startxref_override=5))
    (HERE / "recon_badtype.pdf").write_bytes(_classical(corrupt_type=True))
    for pad in range(0, 4000):
        b = _length_objstm(pad)
        if b is not None:
            (HERE / "length_objstm.pdf").write_bytes(b)
            break
    else:
        raise RuntimeError("no padding produced a whitespace-terminated body")
    print(f"wrote 3 fixtures into {HERE}/")


if __name__ == "__main__":
    main()
