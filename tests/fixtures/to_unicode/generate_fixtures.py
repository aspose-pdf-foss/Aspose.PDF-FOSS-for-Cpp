#!/usr/bin/env python3
"""Regenerate canonical sidecars for the to_unicode fixture corpus.

For each ``*.cmap`` in this directory:
  1. run oracle_pdfminer.adapt(path)
  2. run oracle_minparser.adapt(path)
  3. if the two outputs agree, write the result as
     ``<name>.canonical``; otherwise raise — a disagreement
     means the fixture uncovers a genuine oracle-vs-oracle
     bug that must be resolved before freezing.

Run: ``python3 generate_fixtures.py`` from this directory.

The .cmap fixtures themselves are committed hand-authored
artefacts covering the four shape classes the v1 primitive must
handle:

  simple_bfchar       — 1-byte codes, beginbfchar only
  bfrange_single_dst  — 1-byte codes, bfrange with single-hex dst
  bfrange_array_dst   — 1-byte codes, bfrange with array-of-dst
  two_byte_mixed      — 2-byte codes, bfchar + bfrange, one
                         multi-codepoint mapping (ffi ligature)
  multi_block         — 2-byte codes whose mappings span more
                         than one bfchar block (with a bfrange
                         in between); guards against a parser
                         that only reads the first block of a
                         kind
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import oracle_pdfminer      # noqa: E402
import oracle_minparser     # noqa: E402


def main() -> int:
    cmaps = sorted(HERE.glob("*.cmap"))
    if not cmaps:
        print("no .cmap fixtures found", file=sys.stderr)
        return 1
    for cmap in cmaps:
        a = oracle_pdfminer.adapt(cmap)
        b = oracle_minparser.adapt(cmap)
        if a != b:
            print(f"DISAGREE: {cmap.name}", file=sys.stderr)
            a_lines = a.splitlines()
            b_lines = b.splitlines()
            for i, (la, lb) in enumerate(zip(a_lines, b_lines)):
                if la != lb:
                    print(
                        f"  line {i}: pdfminer={la!r} minparser={lb!r}",
                        file=sys.stderr)
                    break
            return 1
        out = cmap.with_suffix(".canonical")
        out.write_text(a)
        print(f"wrote {out.name} ({a.count(chr(10))} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
