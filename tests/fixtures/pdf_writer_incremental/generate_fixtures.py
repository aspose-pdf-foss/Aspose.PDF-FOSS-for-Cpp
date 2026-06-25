#!/usr/bin/env python3
"""Generate the PDF fixture corpus for the foundation/pdf_writer_incremental
primitive.

The primitive's freeze gate is parse_serialize_roundtrip — for each
fixture base PDF the hand-authored test (per-target) mutates one or
more named indirect objects, calls AppendIncremental, re-parses the
result, and asserts (a) every dirty object's post-Parse Value equals
the expected mutation, (b) every non-dirty object is byte-identical
to its original Dump entry.

Each base PDF is also validated with qpdf at generation time as a
sanity check that we are emitting well-formed input.

Fixtures (small, single-purpose):

  * info_update.pdf — 4 objects (Catalog, Pages, Page, /Info dict).
    Trailer /Info points at obj 4. Use case: in-place /Info dict
    mutation (the primary v1 driver).
  * add_object.pdf — 3 objects (Catalog, Pages, Page). Test adds
    object id=4. Exercises the "dirty list adds a NEW id" path
    and the trailer /Size bump.
  * gap_dirty.pdf — 5 objects. Test mutates objs 2 and 5 (gap at
    3,4). Exercises classical xref subsection split — one
    subsection per consecutive id run.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


# ---------------------------------------------------------------- builders


def _header(version: bytes = b"1.4") -> bytearray:
    out = bytearray()
    out += b"%PDF-" + version + b"\n"
    out += b"%\xe2\xe3\xcf\xd3\n"
    return out


def _render_classical_xref(
        subsections: list[tuple[int, list[tuple[int, int, bytes]]]]) -> bytes:
    out = bytearray()
    out += b"xref\n"
    for first_id, entries in subsections:
        out += f"{first_id} {len(entries)}\n".encode("latin-1")
        for offset, gen, status in entries:
            out += f"{offset:010d} {gen:05d} ".encode("latin-1")
            out += status + b" \n"
    return bytes(out)


def _assemble(object_bodies: list[bytes], trailer_dict: bytes) -> bytes:
    """Build a PDF containing the given object bodies. trailer_dict is
    the bytes between `<<` and `>>` (exclusive)."""
    body = _header()
    offsets: list[int] = []
    for i, content in enumerate(object_bodies, start=1):
        offsets.append(len(body))
        body += f"{i} 0 obj\n".encode("latin-1")
        body += content
        if not content.endswith(b"\n"):
            body += b"\n"
        body += b"endobj\n"
    xref_off = len(body)
    entries = [(0, 65535, b"f")] + [(o, 0, b"n") for o in offsets]
    body += _render_classical_xref([(0, entries)])
    body += b"trailer\n<< " + trailer_dict + b" >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_info_update() -> bytes:
    objs: list[bytes] = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        # 4: /Info dict — the v1 mutation target.
        b"<< /Title (Original) /Author (Tester) >>",
    ]
    return _assemble(
        objs, b"/Size 5 /Root 1 0 R /Info 4 0 R")


def build_add_object() -> bytes:
    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
    ]
    return _assemble(objs, b"/Size 4 /Root 1 0 R")


def build_gap_dirty() -> bytes:
    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        b"<< /Title (Original) >>",
        # 5: a stand-alone integer object — easy mutation target.
        b"42",
    ]
    return _assemble(objs, b"/Size 6 /Root 1 0 R /Info 4 0 R")


# ---------------------------------------------------------------- qpdf check


def _qpdf_check(pdf_path: Path) -> None:
    """Validate the fixture parses cleanly via qpdf. Raises on failure."""
    r = subprocess.run(
        ["qpdf", "--check", str(pdf_path)],
        capture_output=True, text=True, timeout=15, check=False)
    if r.returncode != 0:
        raise RuntimeError(
            f"qpdf rejected {pdf_path.name}:\n{r.stdout}\n{r.stderr}")


# ---------------------------------------------------------------- main


FIXTURES = {
    "info_update.pdf": build_info_update,
    "add_object.pdf": build_add_object,
    "gap_dirty.pdf": build_gap_dirty,
}


def main() -> int:
    for name, builder in FIXTURES.items():
        pdf_path = HERE / name
        pdf_path.write_bytes(builder())
        _qpdf_check(pdf_path)
        print(f"  {name:20s} ({pdf_path.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
