#!/usr/bin/env python3
"""pdfminer.six to_unicode adapter — first oracle.

Leans on pdfminer's CMapParser for the mapping tables (bfchar +
bfrange) — that code path has been exercised against tens of
thousands of production PDFs for 15+ years, so its expansion of
hex dst strings into UTF-16 code units is battle-tested.

The codespacerange entries aren't exposed by pdfminer's
FileUnicodeMap (it tracks the char → Unicode mapping only, not
the codespace that bounds character codes), so we parse them
with a small regex on the raw bytes. Keeping codespace parsing
separate from mapping parsing is defensible — the codespace is
purely syntactic (start / end pair of hex codes), there is no
interpretation layer pdfminer could contribute.

Canonical form (byte-identical to oracle_minparser and to
foundation::to_unicode::ToCanonical):

    codespace <byteCount> <startHex> <endHex>
    bf <byteCount> <charHex> <uniHex> [<uniHex>...]

where:
  * byteCount is 1..4 (single digit)
  * startHex / endHex / charHex are zero-padded uppercase hex
    of width 2 * byteCount
  * uniHex is zero-padded uppercase hex of width 4 (UTF-16BE
    BMP code unit; surrogate pairs are v2 scope and rejected
    here by raising on any hex string containing a value in
    [0xD800, 0xDFFF])

Lines are sorted: codespace entries first (by startHex), then
bf entries (by charHex). Trailing newline after last line.
"""
from __future__ import annotations

import io
import re
import sys
from pathlib import Path

from pdfminer.cmapdb import CMapParser, FileUnicodeMap

_COMMENT = re.compile(rb"%[^\n]*")
# PDF hex strings allow whitespace per §7.3.4.3; the regex
# captures it and the caller strips before int(,,16).
_HEX = re.compile(rb"<([0-9A-Fa-f\s]+)>")
_CODESPACE_BLOCK = re.compile(
    rb"\bbegincodespacerange\b(.*?)\bendcodespacerange\b",
    re.DOTALL,
)
# For byte_count inference on bfchar / bfrange src codes we need
# the ORIGINAL hex width (pdfminer loses leading zeros when it
# stores codes as ints). Grab every <hex> token inside bfchar /
# bfrange blocks and build a {int_value -> byte_count} map.
_BFCHAR_BLOCK = re.compile(
    rb"\bbeginbfchar\b(.*?)\bendbfchar\b", re.DOTALL)
_BFRANGE_BLOCK = re.compile(
    rb"\bbeginbfrange\b(.*?)\bendbfrange\b", re.DOTALL)


_TOKEN = re.compile(rb"<([0-9A-Fa-f\s]+)>|\[([^\]]*)\]")


def _build_byte_count_map(raw: bytes) -> dict[int, int]:
    """Return {char_code -> byte_count} built from the HEX WIDTH
    of every src hex token inside beginbfchar / beginbfrange
    blocks. pdfminer loses the leading-zero width when it
    decodes to int, so we preserve it here. Comment-stripped
    bytes expected.
    """
    mapping: dict[int, int] = {}

    # bfchar blocks: pairs of <srcHex> <dstHex>; only src tokens
    # contribute byte_count data. Strip whitespace inside hex
    # before int()-ing — PDF §7.3.4.3 allows it.
    for block in _BFCHAR_BLOCK.findall(raw):
        hex_tokens = _HEX.findall(block)
        for i in range(0, len(hex_tokens) - 1, 2):
            src_hex = b"".join(
                hex_tokens[i].split()).decode("ascii")
            if len(src_hex) % 2 == 0:
                mapping[int(src_hex, 16)] = len(src_hex) // 2

    # bfrange blocks: triples of <startHex> <endHex> <dst>, where
    # <dst> is either <hex> or [<hex> ...]. We walk the block's
    # token structure so arrays are counted as one entry.
    for block in _BFRANGE_BLOCK.findall(raw):
        toks: list[tuple[str, str]] = []
        for m in _TOKEN.finditer(block):
            if m.group(1) is not None:
                hx = b"".join(m.group(1).split()).decode("ascii")
                toks.append(("hex", hx))
            else:
                toks.append(("array", ""))
        i = 0
        while i + 2 < len(toks):
            if toks[i][0] != "hex" or toks[i + 1][0] != "hex":
                i += 1
                continue
            start_hex = toks[i][1]
            end_hex = toks[i + 1][1]
            if (len(start_hex) % 2 == 0
                    and len(start_hex) == len(end_hex)):
                bc = len(start_hex) // 2
                for c in range(int(start_hex, 16),
                               int(end_hex, 16) + 1):
                    mapping[c] = bc
            i += 3
    return mapping


def _codespace_lines(raw: bytes) -> list[str]:
    """Find every begincodespacerange block and emit one line per
    (start, end) pair. Pair hex widths must match; that's our
    byte-count signal."""
    lines: list[str] = []
    for block in _CODESPACE_BLOCK.findall(raw):
        hex_tokens = _HEX.findall(block)
        if len(hex_tokens) % 2 != 0:
            raise ValueError(
                "codespacerange block has an odd hex-token count")
        for i in range(0, len(hex_tokens), 2):
            start_hex = b"".join(
                hex_tokens[i].split()).decode("ascii")
            end_hex = b"".join(
                hex_tokens[i + 1].split()).decode("ascii")
            if len(start_hex) != len(end_hex):
                raise ValueError(
                    "codespacerange pair with mismatched "
                    f"widths: {start_hex!r} {end_hex!r}")
            if len(start_hex) % 2 != 0:
                raise ValueError(
                    f"odd-length hex in codespacerange: {start_hex!r}")
            byte_count = len(start_hex) // 2
            start_val = int(start_hex, 16)
            end_val = int(end_hex, 16)
            width = 2 * byte_count
            lines.append(
                f"codespace {byte_count} "
                f"{start_val:0{width}X} {end_val:0{width}X}")
    # Stable ordering keyed on the padded start.
    lines.sort(key=lambda s: s.split()[2])
    return lines


def _bf_lines(raw: bytes, raw_nc: bytes,
              widest_byte_count: int) -> list[str]:
    """Use pdfminer to expand all bfchar + bfrange mappings.
    Byte-count per mapping comes from the comment-stripped raw
    bytes — we preserve the source-declared hex width rather
    than infer it from the decoded int value. Widest byte count
    drives the char-hex padding width so the canonical output
    can be directly diffed across oracles. Multi-codepoint dst
    is preserved (UTF-16BE code units).
    """
    cmap = FileUnicodeMap()
    CMapParser(cmap, io.BytesIO(raw)).run()
    byte_count_map = _build_byte_count_map(raw_nc)
    lines: list[str] = []
    for char_code in sorted(cmap.cid2unichr):
        dst = cmap.cid2unichr[char_code]
        # ord(c) is each UTF-16 code unit pdfminer stored for
        # this code. Reject surrogates; v1 is BMP-only.
        uni_hex: list[str] = []
        for ch in dst:
            cp = ord(ch)
            if 0xD800 <= cp <= 0xDFFF:
                raise ValueError(
                    f"surrogate 0x{cp:04X} in mapping; "
                    "v1 is BMP-only")
            if cp > 0xFFFF:
                raise ValueError(
                    f"codepoint 0x{cp:04X} exceeds BMP; "
                    "v1 is BMP-only")
            uni_hex.append(f"{cp:04X}")
        byte_count = byte_count_map.get(char_code)
        if byte_count is None:
            raise ValueError(
                f"char_code 0x{char_code:X} has no source-declared "
                "byte_count (not found in bfchar/bfrange scan)")
        width = 2 * widest_byte_count if widest_byte_count else 2 * byte_count
        uni_str = " ".join(uni_hex)
        lines.append(
            f"bf {byte_count} "
            f"{char_code:0{width}X} {uni_str}")
    lines.sort(key=lambda s: s.split()[2])
    return lines


def adapt(cmap_path: Path) -> str:
    raw = cmap_path.read_bytes()
    # Strip PostScript `%` end-of-line comments; otherwise a
    # comment mentioning "begincodespacerange" opens a bogus block.
    raw_nc = _COMMENT.sub(b"", raw)

    cs_lines = _codespace_lines(raw_nc)
    # Widest declared codespace dictates char-hex width.
    widest = max(
        (int(ln.split()[1]) for ln in cs_lines), default=1)
    # pdfminer is comment-aware so we feed it the original bytes;
    # the byte-count scan needs the comment-stripped view.
    bf_lines = _bf_lines(raw, raw_nc, widest)
    return "\n".join(cs_lines + bf_lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_pdfminer.py <fixture.cmap>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
