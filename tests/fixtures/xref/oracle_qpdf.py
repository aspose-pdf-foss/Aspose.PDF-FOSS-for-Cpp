#!/usr/bin/env python3
"""qpdf xref adapter — run ``qpdf --show-xref <fixture>`` and
normalise its output into the canonical form used by the freeze gate.

qpdf's native format (observed on v12.3):

    1/0: uncompressed; offset = 15
    2/0: uncompressed; offset = 133
    3/0: compressed; stream = 4, index = 1

Canonical form (one line per in-use entry, sorted by object id):

    1 0 inuse 15
    2 0 inuse 133
    3 0 compressed 4 1

Free entries are dropped — qpdf itself drops them, so this is the
same information the tool exposes. Keeping the canonical form narrow
makes it easier for the mutool adapter to emit byte-identical output.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


# Grammar as regex — qpdf's line layout is stable and short, so the
# regex is more readable than a hand-rolled split. Two alternatives
# in one pattern: "uncompressed" or "compressed" — anchor on the id
# prefix so a stray banner line can't masquerade as an entry.
_LINE = re.compile(
    r"^(?P<id>\d+)/(?P<gen>\d+):\s+"
    r"(?:"
    r"uncompressed;\s+offset\s*=\s*(?P<off>\d+)"
    r"|"
    r"compressed;\s+stream\s*=\s*(?P<stream>\d+),\s*index\s*=\s*(?P<idx>\d+)"
    r")\s*$"
)


def run_qpdf(pdf_path: Path) -> str:
    """Invoke ``qpdf --show-xref`` and return its stdout.

    qpdf is installed via homebrew at ``/opt/homebrew/bin/qpdf`` on
    the dev machine; we let PATH resolve it so CI can substitute its
    own build. The call is capped at 10 s — qpdf on a ~1 KB fixture
    returns in milliseconds; anything longer is a hang.
    """
    r = subprocess.run(
        ["qpdf", "--show-xref", str(pdf_path)],
        capture_output=True, text=True, timeout=10, check=False,
    )
    if r.returncode != 0:
        raise RuntimeError(
            f"qpdf --show-xref failed on {pdf_path}: {r.stderr.strip()}")
    return r.stdout


def parse(qpdf_output: str) -> list[str]:
    """Return canonical entry lines sorted by (id, gen) ascending.

    Malformed lines from qpdf (banner, warnings, blank) are skipped
    — we surface agreement failures by three-way comparison at the
    gate, not by over-validating individual oracle outputs here.
    """
    rows: list[tuple[int, int, str]] = []
    for raw in qpdf_output.splitlines():
        m = _LINE.match(raw.strip())
        if not m:
            continue
        oid = int(m.group("id"))
        gen = int(m.group("gen"))
        if m.group("off") is not None:
            line = f"{oid} {gen} inuse {int(m.group('off'))}"
        else:
            line = (f"{oid} {gen} compressed "
                    f"{int(m.group('stream'))} {int(m.group('idx'))}")
        rows.append((oid, gen, line))
    rows.sort(key=lambda r: (r[0], r[1]))
    return [r[2] for r in rows]


def canonical(pdf_path: Path) -> str:
    """Run qpdf + normalise. Returns the canonical text form with a
    trailing newline after the last entry — matches the format the
    C++ primitive's ``ToCanonical`` is expected to emit."""
    return "\n".join(parse(run_qpdf(pdf_path))) + "\n"


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write(
            "usage: oracle_qpdf.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical(Path(sys.argv[1])))
