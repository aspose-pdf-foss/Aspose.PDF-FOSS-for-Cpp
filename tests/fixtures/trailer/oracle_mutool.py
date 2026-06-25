#!/usr/bin/env python3
"""mutool trailer adapter — run ``mutool show <fixture> trailer``
and normalise its output into the canonical form.

mutool's trailer output (observed on v1.26):

    trailer
    <<
      /Size 5
      /Root 1 0 R
      /Info 4 0 R
      /Encrypt 6 0 R
      /ID [ <...> <...> ]
    >>

Canonical form (same as oracle_qpdf.py):

    size <N>
    root <id>
    info <id>         # only when /Info present
    encrypt <id>      # only when /Encrypt present

mutool's output order reflects the PDF's byte order; we sort
fields into the canonical order (size, root, info, encrypt) so
both oracles produce byte-identical canonical strings regardless
of who writes the PDF.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


# Per-line grammar. mutool emits one key per line inside the dict
# body, indented with whitespace. We pick off the four we care
# about; /ID and other keys are ignored.
_SIZE = re.compile(r"^\s*/Size\s+(\d+)\s*$")
_ROOT = re.compile(r"^\s*/Root\s+(\d+)\s+(\d+)\s+R\s*$")
_INFO = re.compile(r"^\s*/Info\s+(\d+)\s+(\d+)\s+R\s*$")
_ENCRYPT = re.compile(r"^\s*/Encrypt\s+(\d+)\s+(\d+)\s+R\s*$")


def run_mutool(pdf_path: Path) -> str:
    """Invoke ``mutool show <pdf> trailer`` and return stdout."""
    proc = subprocess.run(
        ["mutool", "show", str(pdf_path), "trailer"],
        capture_output=True, text=True, timeout=10)
    if proc.returncode != 0:
        raise RuntimeError(
            f"mutool failed: {proc.stderr.strip()}")
    return proc.stdout


def adapt(pdf_path: Path) -> str:
    """Run mutool and return the canonical trailer string."""
    raw = run_mutool(pdf_path)
    size = 0
    root_id = 0
    info_id: int | None = None
    encrypt_id: int | None = None
    for line in raw.splitlines():
        if m := _SIZE.match(line):
            size = int(m.group(1))
        elif m := _ROOT.match(line):
            root_id = int(m.group(1))
        elif m := _INFO.match(line):
            info_id = int(m.group(1))
        elif m := _ENCRYPT.match(line):
            encrypt_id = int(m.group(1))
    lines = [f"size {size}", f"root {root_id}"]
    if info_id is not None:
        lines.append(f"info {info_id}")
    if encrypt_id is not None:
        lines.append(f"encrypt {encrypt_id}")
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
