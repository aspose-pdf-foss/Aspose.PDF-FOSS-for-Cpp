#!/usr/bin/env python3
"""qpdf outlines adapter — run ``qpdf --json --json-key=outlines``
and normalise its outline tree into the canonical form used by
the foundation/outlines freeze gate.

qpdf's outline JSON shape (v12.x):

    {
      "outlines": [
        { "title": "Chapter 1",
          "kids": [
            { "title": "Section 1.1", "kids": [], ... },
            ...
          ],
          ...
        },
        ...
      ]
    }

Canonical form: one line per item in preorder traversal, each
line is ``<depth>\\t<title>\\n``. depth is 0 for top-level
items, 1 for first-level children, and so on. A PDF without
/Outlines emits the empty string.
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def run_qpdf(pdf_path: Path) -> str:
    """Invoke qpdf and return stdout. Stderr is discarded;
    warnings about missing /Resources etc. inject themselves
    into stdout in some qpdf versions, so we suppress them at
    the source via --no-warn."""
    proc = subprocess.run(
        ["qpdf", "--no-warn", "--json",
         "--json-key=outlines", str(pdf_path)],
        capture_output=True, text=True, timeout=10)
    if proc.returncode != 0 and proc.returncode != 3:
        # qpdf exit 3 = warnings (we asked for --no-warn but
        # it can still set this on certain malformations).
        # Anything else is a real failure.
        raise RuntimeError(
            f"qpdf failed (exit {proc.returncode}): "
            f"{proc.stderr.strip()}")
    return proc.stdout


def walk(items: list, depth: int, out: list[str]) -> None:
    """Preorder walk over qpdf's nested ``kids`` arrays."""
    for item in items:
        title = item.get("title", "")
        out.append(f"{depth}\t{title}")
        kids = item.get("kids", [])
        if kids:
            walk(kids, depth + 1, out)


def adapt(pdf_path: Path) -> str:
    """Run qpdf and return the canonical outline string."""
    raw = run_qpdf(pdf_path)
    doc = json.loads(raw)
    items = doc.get("outlines", [])
    lines: list[str] = []
    walk(items, 0, lines)
    if not lines:
        return ""
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print(
            "usage: oracle_qpdf.py <path/to/fixture.pdf>",
            file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
