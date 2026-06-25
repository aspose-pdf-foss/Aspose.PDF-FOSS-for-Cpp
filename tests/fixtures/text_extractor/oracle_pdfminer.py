#!/usr/bin/env python3
"""pdfminer.six text-extraction adapter — run
``pdfminer.high_level.extract_text(<fixture>)`` and normalise its
output into the same canonical form as the pdftotext adapter.

pdfminer.six emits text with these conventions:

* Pages separated by U+000C (form feed, ``\\f``).
* Lines separated by U+000A (LF).
* Tends to insert blank lines between BT/ET blocks even when
  pdftotext doesn't (the two engines have different decisions
  on inter-block whitespace).

The shared canonical form (see oracle_pdftotext.py for the
full rules):

* Pages joined with ``\\f``.
* Within a page, runs of 2+ newlines collapse to 1.
* Each page's leading + trailing whitespace stripped.
* Trailing ``\\n`` after the last page.

After normalisation pdftotext and pdfminer.six should agree
byte-for-byte on every fixture in the corpus.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def run_pdfminer(pdf_path: Path) -> str:
    """Call pdfminer.high_level.extract_text — pure-Python, no
    subprocess. Imported lazily so the script is still useful as
    a CLI even if pdfminer.six is missing (we re-raise a
    RuntimeError that names the missing dep)."""
    try:
        from pdfminer.high_level import extract_text
    except ImportError as e:
        raise RuntimeError(
            "pdfminer.six is required: pip install pdfminer.six "
            f"(import error: {e})")
    return extract_text(str(pdf_path))


def normalise(raw: str) -> str:
    """Identical normalisation to oracle_pdftotext.normalise."""
    pages = raw.split("\f")
    out_pages: list[str] = []
    for p in pages:
        p = re.sub(r"\n{2,}", "\n", p)
        p = p.strip()
        if p == "" and out_pages:
            continue
        out_pages.append(p)
    if not out_pages:
        return ""
    return "\f".join(out_pages) + "\n"


def canonical(pdf_path: Path) -> str:
    return normalise(run_pdfminer(pdf_path))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write(
            "usage: oracle_pdfminer.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical(Path(sys.argv[1])))
