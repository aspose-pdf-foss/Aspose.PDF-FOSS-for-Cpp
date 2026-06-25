#!/usr/bin/env python3
"""mutool pages_tree adapter — run ``mutool pages <fixture>``
and parse its per-page XML-ish report into the canonical form.

mutool's output shape (observed on v1.26):

    <fixture>:
    <page pagenum="1">
    <MediaBox l="0" b="0" r="100" t="100" />
    </page>
    <page pagenum="2">
    <MediaBox l="0" b="0" r="200" t="300" />
    <Rotate v="90" />
    </page>

Canonical form (same as oracle_qpdf.py):

    page <pagenum> <width> <height> <rotation>

rotation defaults to 0 when the <Rotate> element is absent.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


_PAGE_OPEN = re.compile(r'<page\s+pagenum\s*=\s*"(\d+)"\s*>')
_MEDIABOX = re.compile(
    r'<MediaBox\s+l="([-\d.]+)"\s+b="([-\d.]+)"\s+'
    r'r="([-\d.]+)"\s+t="([-\d.]+)"\s*/>')
_ROTATE = re.compile(r'<Rotate\s+v="([-\d]+)"\s*/>')
_PAGE_CLOSE = re.compile(r'</page>')


def _fmt_num(x: float) -> str:
    return f"{int(x)}" if x == int(x) else f"{x:g}"


def _run(pdf_path: Path) -> str:
    p = subprocess.run(
        ["mutool", "pages", str(pdf_path)],
        capture_output=True, text=True, timeout=15)
    if p.returncode != 0:
        raise RuntimeError(
            f"mutool pages {pdf_path.name}: "
            f"{p.stderr.strip()}")
    return p.stdout


def adapt(pdf_path: Path) -> str:
    raw = _run(pdf_path)
    # Split into per-page blocks between <page pagenum="N"> and
    # </page>. mutool's output is line-oriented within each
    # block; we scan each block independently.
    blocks: list[tuple[int, str]] = []
    cur_num: int | None = None
    cur_body: list[str] = []
    for line in raw.splitlines():
        m_open = _PAGE_OPEN.search(line)
        m_close = _PAGE_CLOSE.search(line)
        if m_open:
            cur_num = int(m_open.group(1))
            cur_body = []
        elif m_close and cur_num is not None:
            blocks.append((cur_num, "\n".join(cur_body)))
            cur_num = None
            cur_body = []
        elif cur_num is not None:
            cur_body.append(line)

    lines: list[str] = []
    for pagenum, block in blocks:
        m_mb = _MEDIABOX.search(block)
        if not m_mb:
            raise RuntimeError(
                f"mutool page {pagenum}: no MediaBox")
        llx, lly, urx, ury = (float(g) for g in m_mb.groups())
        width = urx - llx
        height = ury - lly
        m_rot = _ROTATE.search(block)
        rotation = int(m_rot.group(1)) if m_rot else 0
        lines.append(
            f"page {pagenum} "
            f"{_fmt_num(width)} {_fmt_num(height)} {rotation}")
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_mutool.py <path/to/fixture.pdf>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
