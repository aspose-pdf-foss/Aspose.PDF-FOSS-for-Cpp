#!/usr/bin/env python3
"""mutool xref adapter — run ``mutool show <fixture> xref`` and
normalise its output into the canonical form used by the freeze gate.

mutool's native format (observed on v1.27):

    xref
    0 5
    00000: 0000000000 65535 f
    00001: 0000000015 00000 n
    00002: 0000000004 00000 o
    00003: 0000000187 00000 n

The first two lines are a section banner ("xref") and a subsection
header ("<first_id> <count>"); we ignore both. Entry status is a
single trailing letter:

    f  free (skipped — qpdf omits these, so does our canonical form)
    n  in-use uncompressed: (offset, generation)
    o  in-use compressed:   (stream_id, index_in_stream)

Canonical form (one line per in-use entry, sorted by object id):

    1 0 inuse 15
    2 0 compressed 4 0
    3 0 inuse 187
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


# Each fixed-width mutool entry line. The padded zeros make parsing
# trivial (fixed-width fields) but we still use a regex so a stray
# banner line slipping past the header skip can't produce a false
# "entry".
_LINE = re.compile(
    r"^(?P<id>\d{5}):\s+(?P<field1>\d{10})\s+(?P<field2>\d{5})\s+"
    r"(?P<status>[fno])\s*$"
)


def run_mutool(pdf_path: Path) -> str:
    """Invoke ``mutool show <pdf> xref`` and return its stdout.

    mutool is installed via homebrew at ``/opt/homebrew/bin/mutool``.
    The 10 s timeout is defensive — mutool on a KB-sized fixture is
    fast; a timeout hit means the fixture is pathological and the
    fixture author should redesign it.
    """
    r = subprocess.run(
        ["mutool", "show", str(pdf_path), "xref"],
        capture_output=True, text=True, timeout=10, check=False,
    )
    if r.returncode != 0:
        raise RuntimeError(
            f"mutool show xref failed on {pdf_path}: {r.stderr.strip()}")
    return r.stdout


def parse(mutool_output: str) -> list[str]:
    """Return canonical entry lines sorted by (id, gen) ascending.

    Free entries (status=f) are dropped to match the qpdf adapter's
    output. In-use uncompressed (n) uses (offset, generation);
    in-use compressed (o) uses (stream_id, index_in_stream) — the
    two numeric fields mutool prints have different meanings per
    status, so we branch on status before emitting.
    """
    rows: list[tuple[int, int, str]] = []
    for raw in mutool_output.splitlines():
        m = _LINE.match(raw.strip())
        if not m:
            continue
        oid = int(m.group("id"))
        status = m.group("status")
        f1 = int(m.group("field1"))
        f2 = int(m.group("field2"))
        if status == "f":
            continue
        if status == "n":
            line = f"{oid} {f2} inuse {f1}"
        else:  # "o" — compressed into an object stream
            # mutool's PDF 1.5 convention: field1 = container stream
            # object id, field2 = index within that stream. The
            # generation of a compressed object is always 0 per
            # PDF 32000 §7.5.8.2 — we hard-code it to keep the
            # canonical form aligned with qpdf's "<id>/0:" prefix.
            line = f"{oid} 0 compressed {f1} {f2}"
        rows.append((oid, 0 if status == "o" else f2, line))
    rows.sort(key=lambda r: (r[0], r[1]))
    return [r[2] for r in rows]


def canonical(pdf_path: Path) -> str:
    """Run mutool + normalise. Returns the canonical text form with
    a trailing newline so text-compare against the C++ primitive's
    ``ToCanonical`` output is byte-identical."""
    return "\n".join(parse(run_mutool(pdf_path))) + "\n"


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write(
            "usage: oracle_mutool.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical(Path(sys.argv[1])))
