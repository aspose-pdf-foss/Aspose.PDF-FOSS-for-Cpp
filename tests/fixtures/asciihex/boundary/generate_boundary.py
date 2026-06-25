#!/usr/bin/env python3
"""Generate the boundary sub-corpus for the asciihex primitive.

Boundary fixtures probe spec-permissive Decode-only inputs that the
encoder does NOT emit, so the main binding_roundtrip corpus never
exercises them. Each fixture is a pair:

    <name>.bin       — the input bytes the decoder receives
    <name>.expected  — the bytes Decode is required to produce

The driver runs Decode(input) and asserts byte-equality against
.expected.

ASCIIHexDecode (PDF 32000-1:2008 §7.4.2) is decoder-permissive on
four axes that Encode never exercises:

  1. Hex digit case — Decode accepts 'a'..'f' as well as 'A'..'F',
     but Encode always emits uppercase.
  2. Embedded whitespace — Decode skips space, HT, LF, CR, FF, NUL
     anywhere in the input (PDF wraps long ASCIIHex streams across
     lines), but Encode emits a contiguous unwrapped stream.
  3. End-of-data marker — Decode stops on '>' and ignores any bytes
     past it, but Encode never emits '>' (the EOD is PDF stream
     framing, added by the wrapping layer).
  4. Odd trailing digit — Decode treats a final unpaired hex digit
     as if followed by '0' (so "A" alone decodes to 0xA0), but
     Encode always emits an even number of digits.

Self-roundtrip Decode(Encode(X)) == X cannot reach any of these
branches. A target whose Decode is stricter than the spec — e.g.
rejecting lowercase, throwing on embedded whitespace, ignoring the
'>' EOD, or refusing odd-length input — passes self-roundtrip
silently and only diverges from a sibling target when a downstream
caller hands it a real-world PDF stream that exercises one of the
permissive paths. The boundary corpus closes that gap.

Run from this directory:
    python3 generate_boundary.py
"""

from __future__ import annotations

from pathlib import Path


# Each tuple: (name, input_bytes, expected_bytes, description).
_FIXTURES: list[tuple[str, bytes, bytes, str]] = [
    # Lowercase hex digits. §7.4.2: "Both uppercase and lowercase
    # letters are accepted on decode." Encode always emits
    # uppercase, so self-roundtrip never feeds lowercase to Decode.
    # A target that uppercases-and-compares (instead of accepting
    # both) silently fails on real PDFs whose ASCIIHex was written
    # by a tool that emits lowercase.
    (
        "lowercase_hex",
        b"deadbeef",
        bytes([0xDE, 0xAD, 0xBE, 0xEF]),
        "lowercase 'a'..'f' accepted on decode",
    ),

    # Embedded whitespace between hex pairs. §7.4.2: white-space
    # characters (space, HT, LF, CR, FF, NUL) are skipped anywhere
    # in the input. Encode emits a contiguous stream, so
    # self-roundtrip never probes the whitespace-skip path. This
    # fixture interleaves all six whitespace bytes between the five
    # hex pairs of "Hello"; a decoder that mishandles any of them
    # (most likely NUL, which doubles as a C-string terminator)
    # produces wrong output here.
    (
        "embedded_whitespace",
        (b"48" + b"\x00"        # NUL between pairs 1 and 2
         + b"65" + b"\x09"      # HT between 2 and 3
         + b"6C" + b" "         # SPACE between 3 and 4
         + b"6C" + b"\x0A"      # LF between 4 and 5
         + b"6F" + b"\x0C"      # FF after pair 5
         + b"\x0D"),            # trailing CR — must not break
        b"Hello",
        "all six whitespace bytes (NUL/HT/LF/CR/FF/SPACE) skipped",
    ),

    # '>' end-of-data marker mid-stream with garbage afterward.
    # §7.4.2: Decode stops immediately on '>' and does not consume
    # further bytes. Encode never emits '>'. Self-roundtrip never
    # tests early-termination, so a target that runs to end-of-input
    # regardless of '>' (or, conversely, throws on '>' as
    # malformed) passes the main corpus and only diverges on real
    # PDF streams that wrap with EOD.
    (
        "eod_marker",
        b"48656C6C6F>NOT HEX -- DECODER MUST IGNORE THIS TAIL",
        b"Hello",
        "'>' EOD stops decode; trailing non-hex bytes ignored",
    ),

    # Odd number of hex digits before end-of-input. §7.4.2: "If the
    # filter encounters the EOD marker after reading an odd number
    # of hexadecimal digits, it shall behave as if a 0 (zero)
    # followed the last digit." Encode always emits an even number
    # of digits, so self-roundtrip cannot reach this branch. A
    # target that throws on odd length, or pads with the wrong
    # nibble (low instead of high), produces wrong output here.
    (
        "odd_trailing_digit",
        b"48656C6C6F4",
        bytes([0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x40]),
        "trailing single digit '4' → 0x40 (pads with low-nibble 0)",
    ),

    # Combined-edges fixture: lowercase + embedded whitespace +
    # odd trailing digit before '>' + garbage after EOD. Probes
    # that the four boundaries compose correctly — a target that
    # handles each one in isolation but breaks when they coincide
    # (e.g. odd-digit detection runs after EOD scan but before
    # whitespace skip, miscounting digits) fails here. The seven
    # accumulated hex digits 'd','e','a','d','b','e','a' decode as
    # the four pairs DE, AD, BE, A0 (last 'a' padded to "a0").
    (
        "combined_edges",
        b"de\nad\tbe\r a> trailing GARBAGE!!",
        bytes([0xDE, 0xAD, 0xBE, 0xA0]),
        "lowercase + whitespace + odd-before-EOD + garbage after",
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
