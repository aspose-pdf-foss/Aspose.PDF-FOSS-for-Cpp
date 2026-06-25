#!/usr/bin/env python3
"""qpdf pages_tree adapter — run qpdf in two passes (pages list
+ per-page dict), extract pagenum / MediaBox / Rotate, emit the
canonical form used by the pages_tree freeze gate.

Pass 1: ``qpdf --json --json-key=pages <fixture>`` gives the
page list in order (``pageposfrom1`` + ``object`` ref).

Pass 2: ``qpdf --show-object=<id> <fixture>`` for each page id
gives the raw dict; we regex out /MediaBox + /Rotate.

Canonical form (one line per page, trailing newline after last):

    page <pagenum> <width> <height> <rotation>

width, height = urx-llx, ury-lly from the 4-element /MediaBox
array (emitted as integers when the input values are integers;
floats printed with ``%g``).

rotation defaults to 0 when /Rotate is absent.

Scope: fixture corpus is authored so every page's /MediaBox is
directly on the leaf; walking /Parent chains for inheritance is
out of scope. /Rotate on a /Pages parent would also need the
walk — also out of scope.
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path


_REF = re.compile(r"^\s*(\d+)\s+\d+\s+R\s*$")
_MEDIABOX = re.compile(
    r"/MediaBox\s*\[\s*(-?[\d.]+)\s+(-?[\d.]+)\s+"
    r"(-?[\d.]+)\s+(-?[\d.]+)\s*\]")
_ROTATE = re.compile(r"/Rotate\s+(-?\d+)")


def _run(args: list[str]) -> str:
    p = subprocess.run(args, capture_output=True, text=True,
                       timeout=15)
    if p.returncode != 0:
        raise RuntimeError(
            f"{' '.join(args)}: {p.stderr.strip()}")
    return p.stdout


def _fmt_num(x: float) -> str:
    """Integer when exact-integer-valued; otherwise ``%g``.
    Both oracles should produce the same canonical numeric form
    for every fixture's MediaBox values (which are integer in
    the current corpus)."""
    return f"{int(x)}" if x == int(x) else f"{x:g}"


def _page_ids(pdf_path: Path) -> list[int]:
    """Page indirect-reference object ids in document order."""
    raw = _run(["qpdf", "--json", "--json-key=pages",
                str(pdf_path)])
    doc = json.loads(raw)
    ids: list[int] = []
    for p in doc.get("pages", []):
        m = _REF.match(p.get("object", ""))
        if m:
            ids.append(int(m.group(1)))
    return ids


def _page_geometry(pdf_path: Path,
                   page_id: int) -> tuple[float, float, int]:
    """Return (width, height, rotation) for one page."""
    dict_str = _run(
        ["qpdf", f"--show-object={page_id}", str(pdf_path)])
    m = _MEDIABOX.search(dict_str)
    if not m:
        raise RuntimeError(
            f"page {page_id}: /MediaBox not present on the leaf "
            "(inheritance is out of scope for this adapter)")
    llx, lly, urx, ury = (float(g) for g in m.groups())
    width = urx - llx
    height = ury - lly
    rot_m = _ROTATE.search(dict_str)
    rotation = int(rot_m.group(1)) if rot_m else 0
    return width, height, rotation


def adapt(pdf_path: Path) -> str:
    lines: list[str] = []
    for pagenum, page_id in enumerate(
            _page_ids(pdf_path), start=1):
        w, h, r = _page_geometry(pdf_path, page_id)
        lines.append(
            f"page {pagenum} {_fmt_num(w)} {_fmt_num(h)} {r}")
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_qpdf.py <path/to/fixture.pdf>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
