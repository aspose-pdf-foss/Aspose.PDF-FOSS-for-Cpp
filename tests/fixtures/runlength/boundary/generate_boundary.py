#!/usr/bin/env python3
"""Generate the boundary sub-corpus for the runlength primitive.

Boundary fixtures probe spec-permissive Decode-only inputs that the
encoder does NOT emit, so the main binding_roundtrip corpus never
exercises them. Each fixture is a pair:

    <name>.bin       — the input bytes the decoder receives
    <name>.expected  — the bytes Decode is required to produce

The driver runs Decode(input) and asserts byte-equality against
.expected.

These fixtures pin spec-permissive behavior the spec yaml allows
("Read segments until EOD or end of input") so cross-target
implementations cannot diverge on the boundary case while still
passing self-roundtrip. The cpp/csharp Runlength.Decode contracts
diverged on exactly this case until 2026-05-02 — csharp threw on
streams ending without 0x80 EOD; cpp matched the spec's permissive
clause. The boundary corpus exists so future divergence is caught
at gate time rather than via downstream-caller surprise.

Run from this directory:
    python3 generate_boundary.py
"""

from __future__ import annotations

from pathlib import Path


# Each tuple: (name, input_bytes, expected_bytes, description).
_FIXTURES: list[tuple[str, bytes, bytes, str]] = [
    # Stream that ends mid-decode without an EOD marker. A single
    # repeat segment (length=0x82 → 257-130=127 copies of 0x41) and
    # the stream stops there. Spec: "Read segments until EOD (0x80)
    # or end of input." Decoder must emit 127 'A' bytes and stop
    # cleanly — NOT throw "missing EOD marker."
    (
        "no_eod_repeat",
        bytes([0x82, 0x41]),
        b"A" * (257 - 130),
        "repeat segment terminated by end-of-input, no 0x80 EOD",
    ),

    # Same boundary, literal flavour. A literal segment declaring
    # length=4 (length byte 0x03 → copy 4 bytes), payload "TIFF",
    # stream ends. No EOD byte. Spec accepts; decoder emits "TIFF".
    (
        "no_eod_literal",
        bytes([0x03]) + b"TIFF",
        b"TIFF",
        "literal segment terminated by end-of-input, no 0x80 EOD",
    ),

    # A mixed segment sequence (literal then repeat) ending with
    # the repeat's payload byte and no EOD. Probes that the
    # decoder doesn't accidentally require EOD as a between-
    # segment terminator.
    (
        "mixed_no_eod",
        bytes([0x01]) + b"AB" + bytes([0xFE, 0x43]),
        b"AB" + b"C" * 3,
        "literal+repeat ending at end-of-input, no 0x80 EOD",
    ),
]


def main() -> int:
    here = Path(__file__).parent
    for name, src, expected, desc in _FIXTURES:
        (here / f"{name}.bin").write_bytes(src)
        (here / f"{name}.expected").write_bytes(expected)
        print(f"wrote {name}.bin ({len(src)} B) + "
              f"{name}.expected ({len(expected)} B) — {desc}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
