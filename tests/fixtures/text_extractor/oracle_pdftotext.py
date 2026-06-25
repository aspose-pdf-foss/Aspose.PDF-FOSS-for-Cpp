#!/usr/bin/env python3
"""pdftotext text-extraction adapter — run ``pdftotext <fixture>``
and normalise its output into the canonical form used by the
freeze gate.

pdftotext (Poppler) emits text with these conventions:

* Pages separated by U+000C (form feed, ``\\f``).
* Lines separated by U+000A (LF).
* Trailing newline + form feed after the final page.
* Reflowed reading order (multi-column → single column).

Canonical form for the foundation::text_extractor freeze gate:

* One page per logical block, separated by ``\\f`` (form feed).
* Within a page, runs of two or more consecutive newlines are
  collapsed to a single newline (so pdfminer.six's "blank line
  between BT/ET blocks" and pdftotext's tighter spacing both
  reduce to the same canonical text).
* Leading and trailing whitespace stripped on each page.
* A single trailing ``\\n`` after the last page's text.

Both oracle adapters (this one and oracle_pdfminer.py) emit the
same canonical form. The freeze-gate driver compares C++
``foundation::text_extractor::ToCanonical(...)`` output against
the cached ``<fixture>.canonical`` sidecar produced at fixture-
authoring time.

Two-oracle agreement (pdftotext + pdfminer.six on the SAME
fixture, both normalised through THIS canonical form) is what
the corpus-author check enforces — a fixture on which the two
oracles disagree post-normalisation is removed before freeze.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def run_pdftotext(pdf_path: Path) -> str:
    """Invoke ``pdftotext <pdf> -`` (output to stdout). Errors
    surface as a RuntimeError carrying both the return code and
    stderr — the gate logs that, so we keep the message rich."""
    r = subprocess.run(
        ["pdftotext", str(pdf_path), "-"],
        capture_output=True, text=True, timeout=30)
    if r.returncode != 0:
        raise RuntimeError(
            f"pdftotext failed for {pdf_path.name} "
            f"(rc={r.returncode}): {r.stderr.strip()}")
    return r.stdout


def normalise(raw: str) -> str:
    """Apply the canonical-form rules described in the module
    docstring: split on form feeds (page boundaries), collapse
    intra-page blank lines, strip leading/trailing whitespace per
    page, rejoin with form feeds, append a single trailing
    newline."""
    pages = raw.split("\f")
    out_pages: list[str] = []
    for p in pages:
        # Collapse 2+ newlines into 1.
        p = re.sub(r"\n{2,}", "\n", p)
        p = p.strip()
        # Drop entirely-empty trailing page-slot pdftotext
        # sometimes emits after the final form feed.
        if p == "" and out_pages:
            continue
        out_pages.append(p)
    if not out_pages:
        return ""
    return "\f".join(out_pages) + "\n"


def canonical(pdf_path: Path) -> str:
    """Convenience entry — used by generate_fixtures.py at fixture
    authoring time to compute the agreement-checked canonical."""
    return normalise(run_pdftotext(pdf_path))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write(
            "usage: oracle_pdftotext.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical(Path(sys.argv[1])))
