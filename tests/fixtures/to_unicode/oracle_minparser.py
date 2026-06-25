#!/usr/bin/env python3
"""Minimal hand-rolled ToUnicode CMap parser — second oracle.

Reads the CMap as raw bytes, finds begincodespacerange /
beginbfchar / beginbfrange blocks via regex, and expands each
block by tokenising hex strings inside `< ... >` and arrays
inside `[ ... ]`. Completely independent code path from
pdfminer's PLY-based parser — that independence is the whole
value of running it as a second oracle.

Canonical form: identical to oracle_pdfminer. See that file
for the exact layout and sorting rules.

Scope v1 — must match the anchor:
  * Single or double-byte codespaces and char codes.
  * beginbfchar entries: both 1-byte and 2-byte src codes; dst
    is one hex-string that decodes to one or more UTF-16BE code
    units (no surrogate pairs).
  * beginbfrange entries: both dst forms — a single hex-string
    that's per-char-incremented across the range, and an array
    of per-char hex-strings.
  * Surrogate pairs (codepoints in [0xD800, 0xDFFF]) are
    rejected — v1 is BMP-only.

Out of scope: beginnotdefchar / beginnotdefrange; PostScript
flow control; usecmap; mixed-width codespaces where the ranges
disagree on byte count (not forbidden by spec but absent from
the v1 fixture corpus).
"""
from __future__ import annotations

import re
import sys
from pathlib import Path


_COMMENT = re.compile(rb"%[^\n]*")
_CODESPACE_BLOCK = re.compile(
    rb"\bbegincodespacerange\b(.*?)\bendcodespacerange\b",
    re.DOTALL)
_BFCHAR_BLOCK = re.compile(
    rb"\bbeginbfchar\b(.*?)\bendbfchar\b", re.DOTALL)
_BFRANGE_BLOCK = re.compile(
    rb"\bbeginbfrange\b(.*?)\bendbfrange\b", re.DOTALL)

# Inside a block we alternate between <hex> and [<hex> <hex> ...]
# tokens. _TOKEN returns either a "hex" match group 1 or an
# "array" match group 2 (which itself has to be re-scanned for
# inner hex strings). PDF §7.3.4.3 allows whitespace inside hex
# strings — we strip it when decoding the match group.
_TOKEN = re.compile(rb"<([0-9A-Fa-f\s]+)>|\[([^\]]*)\]")
_INNER_HEX = re.compile(rb"<([0-9A-Fa-f\s]+)>")


def _normalise_hex(b: bytes) -> str:
    return b"".join(b.split()).decode("ascii")


def _parse_hex_to_codepoints(hex_str: str) -> list[int]:
    """A dst hex string is a concatenation of UTF-16BE code
    units, 4 hex digits each. Reject surrogates (v1 is BMP-only)
    and reject non-multiple-of-4 lengths."""
    if len(hex_str) == 0 or len(hex_str) % 4 != 0:
        raise ValueError(
            f"dst hex string length must be multiple of 4: {hex_str!r}")
    cps: list[int] = []
    for i in range(0, len(hex_str), 4):
        cp = int(hex_str[i:i + 4], 16)
        if 0xD800 <= cp <= 0xDFFF:
            raise ValueError(
                f"surrogate 0x{cp:04X} in dst; v1 is BMP-only")
        cps.append(cp)
    return cps


def _tokenise(block: bytes) -> list[tuple[str, object]]:
    """Return an ordered list of ("hex", str) or
    ("array", list[str]) tokens from the block body."""
    out: list[tuple[str, object]] = []
    for m in _TOKEN.finditer(block):
        if m.group(1) is not None:
            out.append(("hex", _normalise_hex(m.group(1))))
        else:
            inner = _INNER_HEX.findall(m.group(2))
            out.append(
                ("array", [_normalise_hex(h) for h in inner]))
    return out


def _collect_codespace(raw: bytes) -> list[tuple[int, int, int]]:
    """Returns [(byte_count, start, end), ...]."""
    out: list[tuple[int, int, int]] = []
    for block in _CODESPACE_BLOCK.findall(raw):
        toks = _tokenise(block)
        # Codespaces are consecutive hex pairs; no arrays allowed.
        if any(t[0] != "hex" for t in toks):
            raise ValueError(
                "codespacerange block must contain only hex tokens")
        hexes = [t[1] for t in toks]
        if len(hexes) % 2 != 0:
            raise ValueError(
                "codespacerange has odd hex-token count")
        for i in range(0, len(hexes), 2):
            s_hex = hexes[i]
            e_hex = hexes[i + 1]
            if len(s_hex) != len(e_hex) or len(s_hex) % 2 != 0:
                raise ValueError(
                    f"codespacerange pair width mismatch: "
                    f"{s_hex!r} {e_hex!r}")
            byte_count = len(s_hex) // 2
            out.append((byte_count, int(s_hex, 16),
                        int(e_hex, 16)))
    return out


def _collect_bfchar(raw: bytes) -> list[tuple[int, int, list[int]]]:
    """Returns [(byte_count, char, [cp, cp, ...]), ...]."""
    out: list[tuple[int, int, list[int]]] = []
    for block in _BFCHAR_BLOCK.findall(raw):
        toks = _tokenise(block)
        # Pairs: <srcCode> <dstHex>. Arrays illegal here.
        if any(t[0] != "hex" for t in toks):
            raise ValueError(
                "beginbfchar block must contain only hex tokens")
        hexes = [t[1] for t in toks]
        if len(hexes) % 2 != 0:
            raise ValueError(
                "beginbfchar has odd hex-token count")
        for i in range(0, len(hexes), 2):
            src_hex = hexes[i]
            dst_hex = hexes[i + 1]
            if len(src_hex) % 2 != 0:
                raise ValueError(
                    f"odd-length src hex in bfchar: {src_hex!r}")
            byte_count = len(src_hex) // 2
            src = int(src_hex, 16)
            cps = _parse_hex_to_codepoints(dst_hex)
            out.append((byte_count, src, cps))
    return out


def _collect_bfrange(raw: bytes) -> list[tuple[int, int, list[int]]]:
    """Returns [(byte_count, char, [cp, cp, ...]), ...], one
    entry per char expanded from the range."""
    out: list[tuple[int, int, list[int]]] = []
    for block in _BFRANGE_BLOCK.findall(raw):
        toks = _tokenise(block)
        # Each entry is three tokens: <startHex> <endHex> <dst>
        # where <dst> is either <hex> or [<hex> ...]. Iterate.
        i = 0
        while i < len(toks):
            start_t = toks[i]
            end_t = toks[i + 1]
            dst_t = toks[i + 2]
            if start_t[0] != "hex" or end_t[0] != "hex":
                raise ValueError(
                    "bfrange start/end must be hex tokens")
            start_hex = start_t[1]
            end_hex = end_t[1]
            if len(start_hex) != len(end_hex) or len(start_hex) % 2 != 0:
                raise ValueError(
                    "bfrange start/end width mismatch: "
                    f"{start_hex!r} {end_hex!r}")
            byte_count = len(start_hex) // 2
            start = int(start_hex, 16)
            end = int(end_hex, 16)
            if end < start:
                raise ValueError(
                    f"bfrange end < start: {start_hex} > {end_hex}")

            if dst_t[0] == "hex":
                # Single-hex form: each successive char code
                # maps to the dst with its last UTF-16 code unit
                # incremented.
                base = _parse_hex_to_codepoints(dst_t[1])
                if not base:
                    raise ValueError("bfrange dst empty")
                for offset in range(end - start + 1):
                    cps = list(base)
                    cps[-1] = cps[-1] + offset
                    if cps[-1] > 0xFFFF:
                        raise ValueError(
                            "bfrange overflow past BMP")
                    out.append((byte_count, start + offset, cps))
            else:
                # Array form: dst is per-char.
                dst_list = dst_t[1]
                if len(dst_list) != end - start + 1:
                    raise ValueError(
                        "bfrange array length != range length")
                for offset, dst_hex in enumerate(dst_list):
                    cps = _parse_hex_to_codepoints(dst_hex)
                    out.append((byte_count, start + offset, cps))
            i += 3
    return out


def adapt(cmap_path: Path) -> str:
    raw = cmap_path.read_bytes()
    # Strip PostScript `%` end-of-line comments before block
    # matching — otherwise a word like "beginbfchar" appearing
    # in a comment would open a bogus block.
    raw = _COMMENT.sub(b"", raw)
    codespaces = _collect_codespace(raw)
    chars = _collect_bfchar(raw) + _collect_bfrange(raw)

    widest = max(
        (bc for bc, _, _ in codespaces), default=1)

    cs_lines: list[str] = []
    for bc, start, end in codespaces:
        width = 2 * bc
        cs_lines.append(
            f"codespace {bc} "
            f"{start:0{width}X} {end:0{width}X}")
    cs_lines.sort(key=lambda s: s.split()[2])

    bf_lines: list[str] = []
    width = 2 * widest
    for bc, char, cps in chars:
        uni_str = " ".join(f"{cp:04X}" for cp in cps)
        bf_lines.append(
            f"bf {bc} {char:0{width}X} {uni_str}")
    bf_lines.sort(key=lambda s: s.split()[2])

    return "\n".join(cs_lines + bf_lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_minparser.py <fixture.cmap>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
