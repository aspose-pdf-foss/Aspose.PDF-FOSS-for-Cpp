#!/usr/bin/env python3
"""Regenerate canonical sidecars for the truetype fixture corpus.

For each ``*.ttf`` in this directory:
  1. run oracle_fonttools.adapt(path)
  2. run oracle_freetype.adapt(path)
  3. if the two outputs agree, write the result as ``<name>.canonical``;
     otherwise raise — a disagreement means the fixture is outside v1
     scope or uncovers an oracle-oracle bug that must be diagnosed
     before freezing.

Run: ``python3 generate_fixtures.py`` from this directory.

The .ttf fixtures themselves are committed artefacts (three small
Noto fonts, SIL OFL, see NOTICE.txt) — this script does not
download or modify them; it only materialises the .canonical
sidecars that the freeze gate compares against.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

sys.path.insert(0, str(HERE))
import oracle_fonttools      # noqa: E402
import oracle_freetype       # noqa: E402


def main() -> int:
    ttfs = sorted(HERE.glob("*.ttf"))
    if not ttfs:
        print("no .ttf fixtures found", file=sys.stderr)
        return 1
    for ttf in ttfs:
        a = oracle_fonttools.adapt(ttf)
        b = oracle_freetype.adapt(ttf)
        if a != b:
            print(f"DISAGREE: {ttf.name}", file=sys.stderr)
            # Show first differing line for triage.
            a_lines = a.splitlines()
            b_lines = b.splitlines()
            for i, (la, lb) in enumerate(zip(a_lines, b_lines)):
                if la != lb:
                    print(
                        f"  line {i}: fonttools={la!r} freetype={lb!r}",
                        file=sys.stderr)
                    break
            return 1
        out = ttf.with_suffix(".canonical")
        out.write_text(a)
        print(f"wrote {out.name} ({a.count(chr(10))} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
