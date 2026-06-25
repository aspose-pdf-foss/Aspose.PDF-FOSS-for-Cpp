#!/usr/bin/env python3
"""Generate `.canonical` sidecars for the foundation/xmp
fixture corpus.

Fixtures themselves (`*.xmp`) are hand-authored XMP packets
committed to the repo (see README.md for the catalogue).
This script:

1. For every fixture, runs both oracle adapters
   (oracle_libxmp + oracle_exempi).
2. Asserts byte-identical canonical output.
3. Writes `<fixture>.canonical` on agreement.
4. Aborts with a diff on disagreement.

Run: ``python3 generate_fixtures.py`` from this directory,
once both oracles are installed:
  brew install exempi
  pip install python-xmp-toolkit
"""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _check_oracle_agreement(xmp_path: Path) -> str:
    import oracle_libxmp
    import oracle_lxml
    a = oracle_libxmp.adapt(xmp_path)
    b = oracle_lxml.adapt(xmp_path)
    if a == b:
        return a
    diff_at = 0
    while diff_at < min(len(a), len(b)) and a[diff_at] == b[diff_at]:
        diff_at += 1
    raise RuntimeError(
        f"oracle disagreement on {xmp_path.name} at char {diff_at}:\n"
        f"  libxmp:  {a!r}\n"
        f"  lxml:    {b!r}")


def write_canonicals() -> None:
    print("Generating .canonical sidecars (oracle agreement)...")
    for xmp_path in sorted(HERE.glob("*.xmp")):
        canonical = _check_oracle_agreement(xmp_path)
        cp = xmp_path.with_suffix(".canonical")
        cp.write_text(canonical, encoding="utf-8")
        print(f"  {xmp_path.name:30s}  -> {cp.name} "
              f"({len(canonical.encode())} bytes)")


def main() -> int:
    print("Generating foundation/xmp .canonical sidecars...")
    write_canonicals()
    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
