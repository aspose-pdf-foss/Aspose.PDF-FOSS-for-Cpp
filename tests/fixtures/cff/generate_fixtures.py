#!/usr/bin/env python3
"""Regenerate canonical sidecars for the cff fixture corpus.

For each ``*.cff`` in this directory:
  1. run oracle_fonttools.adapt(path)
  2. run oracle_freetype.adapt(path)
  3. if the two outputs agree, write the result as
     ``<name>.canonical``; otherwise raise — a disagreement
     means the fixture is outside v1 scope or uncovers an
     oracle-vs-oracle bug that must be diagnosed before
     freezing.

Run: ``python3 generate_fixtures.py`` from this directory.

The .cff fixtures themselves are committed artefacts (small
extracted CFF tables from SIL OFL OTF source fonts; see
NOTICE.txt) — this script does not regenerate them. Use
make_corpus.py for that one-shot bootstrap step.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

sys.path.insert(0, str(HERE))
import oracle_fonttools      # noqa: E402
import oracle_freetype       # noqa: E402


def main() -> int:
    cffs = sorted(HERE.glob("*.cff"))
    if not cffs:
        print("no .cff fixtures found", file=sys.stderr)
        return 1
    rc = 0
    for cff in cffs:
        a = oracle_fonttools.adapt(cff)
        b = oracle_freetype.adapt(cff)
        if a != b:
            print(f"DISAGREE: {cff.name}", file=sys.stderr)
            a_lines = a.splitlines()
            b_lines = b.splitlines()
            for i, (la, lb) in enumerate(zip(a_lines, b_lines)):
                if la != lb:
                    print(
                        f"  line {i}: fonttools={la!r} "
                        f"freetype={lb!r}", file=sys.stderr)
                    break
            if len(a_lines) != len(b_lines):
                print(
                    f"  length: fonttools={len(a_lines)} "
                    f"freetype={len(b_lines)}",
                    file=sys.stderr)
            rc = 1
            continue
        out = cff.with_suffix(".canonical")
        out.write_text(a)
        print(f"wrote {out.name} ({a.count(chr(10))} lines)")
    return rc


if __name__ == "__main__":
    sys.exit(main())
