#!/usr/bin/env python3
"""Generate fixture corpus for the foundation/pages_tree
primitive.

Each fixture exercises a different /Pages-tree shape:

* ``single.pdf``       — 1 page, MediaBox 100x100, no rotation.
  Floor case.
* ``three_sizes.pdf``  — 3 pages with distinct MediaBoxes
  (100x100, 200x300, 400x400); page 2 also has /Rotate 180.
  Exercises per-page MediaBox + mixed rotations.
* ``nested_tree.pdf``  — 4 pages organised as a nested tree:
  the /Pages root has 2 intermediate /Pages nodes as /Kids,
  each with 2 leaf /Page children. Exercises the preorder
  traversal of non-flat trees.

For v1 scope, every page carries its /MediaBox directly on
the leaf — no inheritance from a /Pages parent. Walking
/Parent chains for inheritance is supported by the primitive
itself but not exercised by the oracle corpus (neither qpdf's
nor mutool's raw-object views resolve it cheaply). Inheritance
is tested at a separate C++ unit level later.

Two-oracle agreement (qpdf + mutool) gates the corpus: any
fixture on which the two tools disagree is removed before
freezing.

Run: ``python3 generate_fixtures.py`` from this directory.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


HERE = Path(__file__).parent


def _build(objects: list[bytes], trailer: bytes) -> bytes:
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


def _write(path: Path, contents: bytes) -> None:
    path.write_bytes(contents)
    print(f"  wrote {path.name}  ({len(contents)} bytes)")


def build_single() -> None:
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write(HERE / "single.pdf", _build(objects, trailer))


def build_three_sizes() -> None:
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree root
        b"<< /Type /Pages /Kids [3 0 R 4 0 R 5 0 R] "
        b"/Count 3 >>",
        # 3: page 1 — 100x100
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        # 4: page 2 — 200x300, rotate 180
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 300] /Rotate 180 "
        b"/Resources << >> >>",
        # 5: page 3 — 400x400
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 400 400] /Resources << >> >>",
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write(HERE / "three_sizes.pdf",
           _build(objects, trailer))


def build_nested_tree() -> None:
    # Root /Pages -> 2 intermediate /Pages -> 2 leaves each.
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Root /Pages (Kids = [3, 4])
        b"<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 4 >>",
        # 3: First intermediate /Pages (Kids = [5, 6])
        b"<< /Type /Pages /Parent 2 0 R "
        b"/Kids [5 0 R 6 0 R] /Count 2 >>",
        # 4: Second intermediate /Pages (Kids = [7, 8])
        b"<< /Type /Pages /Parent 2 0 R "
        b"/Kids [7 0 R 8 0 R] /Count 2 >>",
        # 5: leaf page 1 — 100x100
        b"<< /Type /Page /Parent 3 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        # 6: leaf page 2 — 200x200
        b"<< /Type /Page /Parent 3 0 R "
        b"/MediaBox [0 0 200 200] /Resources << >> >>",
        # 7: leaf page 3 — 300x300
        b"<< /Type /Page /Parent 4 0 R "
        b"/MediaBox [0 0 300 300] /Resources << >> >>",
        # 8: leaf page 4 — 400x500, rotate 90
        b"<< /Type /Page /Parent 4 0 R "
        b"/MediaBox [0 0 400 500] /Rotate 90 "
        b"/Resources << >> >>",
    ]
    trailer = b" /Size 9 /Root 1 0 R "
    _write(HERE / "nested_tree.pdf",
           _build(objects, trailer))


def run_oracle(script: str, pdf: Path) -> str:
    p = subprocess.run(
        [sys.executable, str(HERE / script), str(pdf)],
        capture_output=True, text=True, timeout=30)
    if p.returncode != 0:
        raise RuntimeError(
            f"{script} on {pdf.name}: {p.stderr.strip()}")
    return p.stdout


def agree_or_die(pdf: Path) -> str:
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
    canonical = agree_or_die(pdf)
    sidecar = pdf.with_suffix(".canonical")
    sidecar.write_text(canonical)
    print(f"  cached {sidecar.name}  "
          f"({len(canonical)} bytes)")


def main() -> None:
    build_single()
    build_three_sizes()
    build_nested_tree()
    print("")
    for pdf in sorted(HERE.glob("*.pdf")):
        cache_canonical(pdf)


if __name__ == "__main__":
    main()
