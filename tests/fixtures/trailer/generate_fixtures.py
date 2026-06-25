#!/usr/bin/env python3
"""Generate the PDF fixture corpus for the foundation/trailer
primitive.

Each fixture exercises a different combination of trailer-dict
entries:

* ``only_size_root.pdf`` — minimal trailer: just /Size + /Root.
  Establishes the floor. Info and encrypt absent.
* ``with_info.pdf``      — /Size + /Root + /Info. The typical
  case for a metadata-carrying PDF.

Fixtures with /Encrypt present in the trailer are deliberately
NOT in v1's oracle corpus. qpdf / mutool both attempt actual
decryption on such files (empty-password or otherwise); a
syntactically-stub /Encrypt dict that fails real decryption
gets rejected by both tools before they can emit trailer data.
Authoring a valid empty-password /Encrypt (proper /O, /U
hashes per §7.6.3.4) is out of scope for this primitive's
fixture corpus — the /Encrypt-present code path is exercised
instead via a unit-level test on the C++ body.

For every fixture, both qpdf and mutool are invoked via the
adapter scripts and their outputs are compared. If they agree,
the canonical trailer string is written as
``<fixture>.canonical``; if they disagree, the fixture is
removed and the script aborts.

Run: ``python3 generate_fixtures.py`` from this directory.
"""

from __future__ import annotations

import struct
import subprocess
import sys
import zlib
from pathlib import Path


HERE = Path(__file__).parent


def _build(objects: list[bytes], trailer: bytes) -> bytes:
    """Assemble a minimal PDF 1.4. Same helper shape used in
    the test fixtures —
    kept local so each fixture directory stays self-contained."""
    header = b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n"
    out = bytearray(header)
    offsets: list[int] = []
    for i, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode()
        out += body
        out += b"\nendobj\n"
    xref_pos = len(out)
    n = len(objects)
    out += f"xref\n0 {n + 1}\n".encode()
    out += b"0000000000 65535 f \n"
    for off in offsets:
        out += f"{off:010d} 00000 n \n".encode()
    out += b"trailer\n<<"
    out += trailer
    out += f">>\nstartxref\n{xref_pos}\n%%EOF".encode()
    return bytes(out)


def _pdf_string(s: str) -> bytes:
    return b"(" + s.encode("ascii") + b")"


def _write(path: Path, contents: bytes) -> None:
    path.write_bytes(contents)
    print(f"  wrote {path.name}  ({len(contents)} bytes)")


def build_only_size_root() -> None:
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write(HERE / "only_size_root.pdf", _build(objects, trailer))


def build_with_info() -> None:
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        b"<< /Producer (Trailer Fixture Generator) >>",
    ]
    trailer = b" /Size 5 /Root 1 0 R /Info 4 0 R "
    _write(HERE / "with_info.pdf", _build(objects, trailer))


def build_xref_stream_trailer() -> None:
    """PDF 1.5 whose trailer IS a cross-reference stream (§7.5.8) —
    there is no ``trailer`` keyword. /Size, /Root and /Info live in
    the xref stream object's own dictionary; the trailer primitive
    must read them from there (startxref points at that object)."""
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        b"<< /Producer (Trailer Fixture Generator) >>",
    ]
    out = bytearray(b"%PDF-1.5\n%\xe2\xe3\xcf\xd3\n")
    offsets: list[int] = []
    for i, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode() + body + b"\nendobj\n"

    # Object 5 is the xref stream itself; its entry points at its own
    # offset. W = [1 3 2]; entries are (type, field2, field3).
    xref_pos = len(out)
    entries = [(0, 0, 0xFFFF)] + [(1, off, 0) for off in offsets] \
        + [(1, xref_pos, 0)]
    raw = bytearray()
    for etype, f2, f3 in entries:
        raw += bytes([etype]) + struct.pack(">I", f2)[1:] \
            + struct.pack(">H", f3)
    comp = zlib.compress(bytes(raw))
    stream_dict = (
        f"<< /Type /XRef /Size 6 /Root 1 0 R /Info 4 0 R "
        f"/W [1 3 2] /Length {len(comp)} /Filter /FlateDecode >>"
    ).encode()
    out += b"5 0 obj\n" + stream_dict + b"\nstream\n"
    out += comp + b"\nendstream\nendobj\n"
    out += f"startxref\n{xref_pos}\n%%EOF".encode()
    _write(HERE / "xref_stream_trailer.pdf", bytes(out))


def run_oracle(script: str, pdf: Path) -> str:
    """Invoke a local oracle adapter and return its stdout."""
    proc = subprocess.run(
        [sys.executable, str(HERE / script), str(pdf)],
        capture_output=True, text=True, timeout=15)
    if proc.returncode != 0:
        raise RuntimeError(
            f"{script} on {pdf.name}: "
            f"{proc.stderr.strip()}")
    return proc.stdout


def agree_or_die(pdf: Path) -> str:
    """Run both oracles; return their agreed canonical string,
    or abort with a clear diff if they disagree."""
    qpdf_out = run_oracle("oracle_qpdf.py", pdf)
    mutool_out = run_oracle("oracle_mutool.py", pdf)
    if qpdf_out != mutool_out:
        print(f"\n  ORACLES DISAGREE on {pdf.name}:",
              file=sys.stderr)
        print("  --- qpdf ---", file=sys.stderr)
        print(qpdf_out, file=sys.stderr)
        print("  --- mutool ---", file=sys.stderr)
        print(mutool_out, file=sys.stderr)
        raise SystemExit(1)
    return qpdf_out


def cache_canonical(pdf: Path) -> None:
    """Write <fixture>.canonical sidecar, gated by oracle
    agreement."""
    canonical = agree_or_die(pdf)
    sidecar = pdf.with_suffix(".canonical")
    sidecar.write_text(canonical)
    print(f"  cached {sidecar.name}  "
          f"({len(canonical)} bytes)")


def main() -> None:
    build_only_size_root()
    build_with_info()
    build_xref_stream_trailer()
    print("")
    for pdf in sorted(HERE.glob("*.pdf")):
        cache_canonical(pdf)


if __name__ == "__main__":
    main()
