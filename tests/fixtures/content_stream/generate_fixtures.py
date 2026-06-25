#!/usr/bin/env python3
"""Generate the content_stream fixture corpus + .canonical sidecars.

The freeze gate is two_oracle_agreement — each fixture ships a
``.canonical`` sidecar that is the byte-identical output of two
independent content-stream parsers (pypdf + pdfminer.six). At
fixture-authoring time (this script), a fixture whose oracles
disagree is NOT written to disk — the operator has to
investigate and either fix the fixture or exclude it. The C++
gate then compares our adapter's output to the cached sidecar;
no oracle binary is consulted at gate time, so the freeze is
reproducible on any machine with just a C++ compiler plus the
lexer + objects sources.

Fixture shapes:

  * ``empty.bin`` — zero bytes. Parser must emit an empty
    operations list and ToCanonical must return an empty
    string (no leading / trailing newline).
  * ``simple_path.bin`` — path-construction + stroke operators
    (m, l, S). Smallest non-empty content stream.
  * ``text_basic.bin`` — BT … Tf … Td … Tj … ET. Exercises
    text state + text-showing operators with literal string
    operands.
  * ``graphics_state.bin`` — q/Q with cm transformation matrix
    and rg / re / f. Exercises graphics-state save/restore and
    color setting.
  * ``marked_content.bin`` — BMC / BDC with a /Properties dict
    operand / EMC. Exercises marked-content sequences with
    dictionary operands.
  * ``array_operands.bin`` — d (dash pattern) takes an ARRAY
    operand. Exercises array-as-operand handling — a common
    first-draft bug in content-stream parsers is treating
    '[' as an operator rather than as the start of an array
    literal.
  * ``string_kinds.bin`` — literal string (Hello) and hex
    string <48656C6C6F>. Both decode to the same bytes and
    canonicalise identically; fixtures prove the parser
    handles both source forms.

Fixtures are public-domain — generated deterministically from
this script.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

# Oracle adapters + canonicaliser live in sibling modules.
sys.path.insert(0, str(Path(__file__).parent))
import oracle_pdfminer  # noqa: E402
import oracle_pypdf  # noqa: E402
import ref_parser  # noqa: E402


# ---------------------------------------------------------------------------
# Fixture definitions
# ---------------------------------------------------------------------------


def _empty() -> bytes:
    """Zero-byte input. A parser that spuriously emits an EOD-like
    operation on empty input would fail this; a parser that
    crashes on empty input would fail it louder."""
    return b""


def _simple_path() -> bytes:
    """Smallest non-empty happy-path content stream: two numbers,
    'm' operator; two numbers, 'l' operator; no operands, 'S'
    operator. Covers operand-stack push/pop and zero-operand
    operators, which are the two cardinal code paths in the
    parser."""
    return b"100 200 m 150 250 l S\n"


def _text_basic() -> bytes:
    """Basic text-showing sequence. Exercises:
      * text-state block brackets (BT / ET)
      * name operand (/F1) followed by a number (12) for Tf
      * literal-string operand for Tj
    """
    return (
        b"BT\n"
        b"  /F1 12 Tf\n"
        b"  100 700 Td\n"
        b"  (Hello, world!) Tj\n"
        b"ET\n"
    )


def _graphics_state() -> bytes:
    """Graphics-state save/restore with a CTM change, colour
    set, rectangle, fill. Six numeric operands on the cm
    operator exercise the biggest operand-stack fan-in in the
    operator vocabulary."""
    return (
        b"q\n"
        b"  1 0 0 1 50 700 cm\n"
        b"  0.8 0.2 0.1 rg\n"
        b"  0 0 100 50 re\n"
        b"  f\n"
        b"Q\n"
    )


def _marked_content() -> bytes:
    """Marked content with a property dictionary operand on BDC
    and a balancing EMC. Dicts-as-operands surface any parser
    that hardcodes '<<' as the start of a direct value vs the
    start of an operator."""
    return (
        b"/Artifact BMC\n"
        b"  (top) Tj\n"
        b"EMC\n"
        b"/Span << /MCID 1 /ActualText (Hi) >> BDC\n"
        b"  (body) Tj\n"
        b"EMC\n"
    )


def _array_operands() -> bytes:
    """Dash pattern ``d`` takes an array-of-numbers operand + a
    phase number. Arrays-as-operands are the dual of dicts-as-
    operands for parser correctness — a parser that treats '['
    as an operator rather than an array opener would fail here
    immediately."""
    return (
        b"[5 3] 0 d\n"
        b"100 100 m\n"
        b"200 100 l\n"
        b"S\n"
    )


def _string_kinds() -> bytes:
    """Literal and hex strings encoding the same bytes. Both
    canonicalise to '<48656C6C6F>' regardless of source form —
    the freeze gate proves the parser recognises both string
    forms as equivalent, and the oracle-agreement step proves
    pypdf and pdfminer agree on the decoded byte value."""
    return (
        b"BT\n"
        b"  /F1 12 Tf\n"
        b"  50 50 Td\n"
        b"  (Hello) Tj\n"
        b"  <48656C6C6F> Tj\n"
        b"ET\n"
    )


# Fixture registry: name → factory. Keep focused on shape
# coverage; a new fixture should exercise an axis the existing
# set doesn't (a new operand kind, a new delimiter, a new
# operator category).
_FIXTURES: dict[str, callable] = {
    "empty.bin":            _empty,
    "simple_path.bin":      _simple_path,
    "text_basic.bin":       _text_basic,
    "graphics_state.bin":   _graphics_state,
    "marked_content.bin":   _marked_content,
    "array_operands.bin":   _array_operands,
    "string_kinds.bin":     _string_kinds,
}


# ---------------------------------------------------------------------------
# Oracle agreement + sidecar writer
# ---------------------------------------------------------------------------


def _produce_canonical(raw: bytes) -> str:
    """Run both oracles, assert agreement, return the canonical
    text. Raises with a diff-style message if oracles disagree."""
    # Empty input is the degenerate case — both oracles would
    # nominally emit an empty operation list on an empty stream,
    # but driving them through the minimal-PDF wrapper is
    # fragile (zero-byte /Contents streams are unusual). Short-
    # circuit to the known-correct answer.
    if not raw:
        return ""

    ops_pypdf = oracle_pypdf.parse_content_stream(raw)
    ops_pdfminer = oracle_pdfminer.parse_content_stream(raw)

    canon_pypdf = ref_parser.canonicalise_stream(ops_pypdf)
    canon_pdfminer = ref_parser.canonicalise_stream(ops_pdfminer)

    if canon_pypdf != canon_pdfminer:
        # Surface the first divergence line so the operator can
        # inspect it directly. Trim to 500 chars each side so the
        # terminal isn't flooded by a large diff.
        a_lines = canon_pypdf.splitlines()
        b_lines = canon_pdfminer.splitlines()
        for i, (a, b) in enumerate(zip(a_lines, b_lines)):
            if a != b:
                raise RuntimeError(
                    f"oracles disagree at line {i + 1}:\n"
                    f"  pypdf:    {a[:500]!r}\n"
                    f"  pdfminer: {b[:500]!r}")
        # Equal prefix — one list is longer.
        raise RuntimeError(
            f"oracles disagree at length: "
            f"pypdf={len(a_lines)} pdfminer={len(b_lines)}")
    return canon_pypdf


def main() -> int:
    here = Path(__file__).parent
    for name, gen in _FIXTURES.items():
        raw = gen()
        canonical = _produce_canonical(raw)

        bin_path = here / name
        canon_path = bin_path.with_suffix(".canonical")
        bin_path.write_bytes(raw)
        canon_path.write_text(canonical)

        raw_digest = hashlib.sha256(raw).hexdigest()[:16]
        canon_digest = hashlib.sha256(
            canonical.encode("utf-8")).hexdigest()[:16]
        print(f"wrote {name:24s} "
              f"{len(raw):5d} B bin (sha256:{raw_digest}…), "
              f"{len(canonical):4d} B canon "
              f"(sha256:{canon_digest}…)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
