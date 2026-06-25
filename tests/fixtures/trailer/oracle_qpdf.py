#!/usr/bin/env python3
"""qpdf trailer adapter — run ``qpdf --json`` and normalise its
trailer section into the canonical form used by the freeze gate.

qpdf's JSON output structure (observed on v12.x):

    {
      "qpdf": [
        {"qpdf-v": "2", "pdf-version": "1.4"},
        {
          "trailer": {
            "value": {
              "/Info": "4 0 R",
              "/Root": "1 0 R",
              "/Size": 5
            }
          },
          ...
        }
      ]
    }

Canonical form (newline-terminated lines, in this order):

    size <N>
    root <id>
    info <id>         # only when /Info present
    encrypt <id>      # only when /Encrypt present

qpdf discards any other trailer keys as far as the anchor's scope
goes — /ID and custom entries are off-topic for v1.
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path


# "<id> <gen> R" — the textual form qpdf uses inside the JSON
# value fields for indirect references.
_REF = re.compile(r"^\s*(\d+)\s+(\d+)\s+R\s*$")


def run_qpdf(pdf_path: Path) -> str:
    """Invoke ``qpdf --json`` and return stdout. qpdf is
    installed via homebrew; we let PATH resolve it."""
    proc = subprocess.run(
        ["qpdf", "--json", str(pdf_path)],
        capture_output=True, text=True, timeout=10)
    if proc.returncode != 0:
        raise RuntimeError(
            f"qpdf failed: {proc.stderr.strip()}")
    return proc.stdout


def parse_ref(value: str) -> int:
    """Extract the object id from an "<id> <gen> R" string.
    Returns 0 if the string doesn't match (indicating a
    non-ref value which is a schema violation upstream)."""
    m = _REF.match(value)
    return int(m.group(1)) if m else 0


def trailer_to_canonical(trailer_dict: dict) -> str:
    """Shape qpdf's trailer dict into the canonical lines."""
    size = int(trailer_dict.get("/Size", 0))
    root = parse_ref(trailer_dict.get("/Root", ""))
    info_raw = trailer_dict.get("/Info")
    encrypt_raw = trailer_dict.get("/Encrypt")

    lines = [f"size {size}", f"root {root}"]
    if info_raw:
        lines.append(f"info {parse_ref(info_raw)}")
    if encrypt_raw:
        lines.append(f"encrypt {parse_ref(encrypt_raw)}")
    return "\n".join(lines) + "\n"


def adapt(pdf_path: Path) -> str:
    """Run qpdf and return the canonical trailer string."""
    raw = run_qpdf(pdf_path)
    doc = json.loads(raw)
    # qpdf's JSON top-level has two sections; the trailer
    # lives in the second.
    sections = doc.get("qpdf", [])
    trailer = {}
    for sec in sections:
        if "trailer" in sec:
            trailer = sec["trailer"].get("value", {})
            break
    return trailer_to_canonical(trailer)


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
