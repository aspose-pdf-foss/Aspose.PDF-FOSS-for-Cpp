#!/usr/bin/env python3
"""mutool outlines adapter — run ``mutool show <pdf> outline``
and normalise its outline tree into the canonical form used by
the foundation/outlines freeze gate.

mutool's outline output (observed on v1.26):

    -\t"Chapter 1"\t(null)
    |\t\t"Section 1.1"\t(null)
    |\t\t"Section 1.2"\t(null)
    |\t"Chapter 2"\t(null)

Each line starts with a tree-rendering prefix (``-`` for the
first item, ``|`` for siblings), followed by tab-separated
fields. The number of leading tabs after the prefix indicates
depth: 1 tab → top-level (depth 0), 2 tabs → first-level
nested (depth 1), and so on.

Canonical form: ``<depth>\\t<title>\\n`` per item, in preorder.
A PDF without /Outlines emits the empty string.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


# A line looks like:
#   <prefix>\t+<TAB>+"<title>"<TAB>(null)
# where prefix is one of '-' or '|'. The number of TABs between
# the prefix and the opening quote determines depth (depth =
# tab_count - 1).
_LINE = re.compile(r'^([-|])(\t+)"((?:[^"\\]|\\.)*)"\t')


def run_mutool(pdf_path: Path) -> str:
    """Invoke ``mutool show <pdf> outline`` and return stdout."""
    proc = subprocess.run(
        ["mutool", "show", str(pdf_path), "outline"],
        capture_output=True, text=True, timeout=10)
    if proc.returncode != 0:
        # mutool exits non-zero on outline-less PDFs in some
        # versions; treat empty output as legitimate.
        if not proc.stdout.strip():
            return ""
        raise RuntimeError(
            f"mutool failed: {proc.stderr.strip()}")
    return proc.stdout


def adapt(pdf_path: Path) -> str:
    """Run mutool and return the canonical outline string."""
    raw = run_mutool(pdf_path)
    lines: list[str] = []
    for line in raw.splitlines():
        m = _LINE.match(line)
        if not m:
            continue
        tabs = m.group(2)
        title = m.group(3)
        # Unescape \" and \\ that mutool emits inside titles.
        title = title.replace('\\"', '"').replace('\\\\', '\\')
        depth = len(tabs) - 1
        lines.append(f"{depth}\t{title}")
    if not lines:
        return ""
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print(
            "usage: oracle_mutool.py <path/to/fixture.pdf>",
            file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
