#!/usr/bin/env python3
"""Generate fixture PDFs for the Page::ExtractText capability.

Each fixture is a hand-authored 1-page PDF that exercises one
specific extraction path:

* ``hello_world.pdf``    — single Tj of literal text in a font
                            without /ToUnicode. The body must
                            fall back to Latin-1 byte
                            interpretation. Expected:
                            "Hello World".
* ``two_lines.pdf``      — two BT/ET blocks, one literal-text
                            Tj each. Expected: "Line one\\n
                            Line two".
* ``tj_array.pdf``       — single TJ with mixed string + numeric
                            kerning entries. Numeric entries
                            are ignored in v1; expected:
                            "ABCDEF".
* ``tounicode_acute.pdf`` — single Tj of two-byte char codes
                            decoded through a /ToUnicode CMap
                            mapping <0001>..<0003> to "É",
                            "ñ", "中" (U+4E2D, BMP). Expected:
                            "Éñ中".

Each fixture is paired with a ``.expected`` sidecar — the exact
UTF-8 string that Page::ExtractText is expected to return after
``Trim`` (no leading/trailing whitespace; internal newlines
between BT/ET blocks).

Run: ``python3 generate_fixtures.py`` from this directory.
"""

from __future__ import annotations

import sys
from pathlib import Path


HERE = Path(__file__).parent


def _build(objects: list[bytes], trailer: bytes) -> bytes:
    """Build a PDF 1.4 file from a list of object bodies.

    Indices are 1-based in the PDF and 0-based in the list.
    Returns the assembled bytes including header, body, xref
    table, trailer, startxref + EOF marker.
    """
    header = b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n"
    out = bytearray(header)
    offsets: list[int] = []
    for i, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode()
        out += body
        out += b"\nendobj\n"
    xref_pos = len(out)
    n = len(objects)
    out += f"xref\n0 {n + 1}\n".encode()
    out += b"0000000000 65535 f \n"
    for off in offsets:
        out += f"{off:010d} 00000 n \n".encode()
    out += b"trailer\n<<"
    out += trailer
    out += f">>\nstartxref\n{xref_pos}\n%%EOF".encode()
    return bytes(out)


def _stream_object(body: bytes) -> bytes:
    """Build an inline (uncompressed) /Contents stream with the
    /Length set to the literal byte count of `body`."""
    return (
        f"<< /Length {len(body)} >>\n".encode()
        + b"stream\n" + body + b"\nendstream"
    )


def _write(name: str, contents: bytes, expected: str) -> None:
    pdf = HERE / f"{name}.pdf"
    sidecar = HERE / f"{name}.expected"
    pdf.write_bytes(contents)
    sidecar.write_text(expected, encoding="utf-8")
    print(f"  wrote {pdf.name}  ({len(contents)} bytes)  -> "
          f"expected {expected!r}")


def build_hello_world() -> None:
    """1-page PDF, one Tj of literal text, font without
    /ToUnicode → Latin-1 fallback path."""
    cs = b"BT /F1 12 Tf 100 700 Td (Hello World) Tj ET"
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page (refers to /Resources/Font and /Contents)
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        # 4: Font (Type 1 Helvetica, no /ToUnicode)
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica >>",
        # 5: Contents stream
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("hello_world", _build(objects, trailer),
           "Hello World")


def build_two_lines() -> None:
    """1-page PDF, two BT/ET blocks. Body inserts a `\\n`
    between blocks → expected output is two lines."""
    cs = (
        b"BT /F1 12 Tf 100 720 Td (Line one) Tj ET\n"
        b"BT /F1 12 Tf 100 700 Td (Line two) Tj ET"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica >>",
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("two_lines", _build(objects, trailer),
           "Line one\nLine two")


def build_tj_array() -> None:
    """1-page PDF using TJ with kerning-free entries — exercises
    the array-iteration path (multiple string fragments inside a
    single TJ) without triggering kerning-driven space insertion
    (which both pdftotext and pdfminer.six perform on negative
    kerning above ~100; that's a separate v1.1 concern). With
    only string entries, the expected output is simply the
    concatenation of all fragments."""
    # TJ array with three string fragments + tiny zero-impact
    # numerics so the array shape is exercised but no oracle adds
    # whitespace.
    cs = b"BT /F1 12 Tf 100 700 Td [(AB) 0 (CD) 0 (EF)] TJ ET"
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica >>",
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("tj_array", _build(objects, trailer), "ABCDEF")


def build_tounicode_acute() -> None:
    """1-page PDF using two-byte char codes decoded via a
    /ToUnicode CMap. Three codes: 0x0001 -> "É" (U+00C9),
    0x0002 -> "ñ" (U+00F1), 0x0003 -> "中" (U+4E2D).

    The font is a Type0 / CIDFontType0 with /Encoding
    /Identity-H so char codes pass through unchanged into the
    /ToUnicode lookup. The /ToUnicode stream is the actual
    decoder; the rest is dressing.
    """
    # Three two-byte codes: 0x0001 0x0002 0x0003.
    text_bytes = bytes([0x00, 0x01, 0x00, 0x02, 0x00, 0x03])
    text_literal = b"<" + text_bytes.hex().encode().upper() + b">"
    cs = (
        b"BT /F1 12 Tf 100 700 Td "
        + text_literal + b" Tj ET"
    )

    # Minimal /ToUnicode CMap. PDF 32000 §9.10.3 + Adobe TN
    # #5014. Standard prologue + codespacerange + bfchar.
    cmap = b"""\
/CIDInit /ProcSet findresource begin
12 dict begin
begincmap
/CIDSystemInfo
<< /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def
/CMapName /Adobe-Identity-UCS def
/CMapType 2 def
1 begincodespacerange
<0001> <FFFF>
endcodespacerange
3 beginbfchar
<0001> <00C9>
<0002> <00F1>
<0003> <4E2D>
endbfchar
endcmap
CMapName currentdict /CMap defineresource pop
end
end
"""

    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 8 0 R >>",
        # 4: Font (Type0 wrapper)
        b"<< /Type /Font /Subtype /Type0 "
        b"/BaseFont /Identity "
        b"/Encoding /Identity-H "
        b"/DescendantFonts [5 0 R] "
        b"/ToUnicode 7 0 R >>",
        # 5: CIDFontType0 (descendant) — minimal, no real
        # font program. ExtractText only reads /ToUnicode.
        b"<< /Type /Font /Subtype /CIDFontType0 "
        b"/BaseFont /Identity "
        b"/CIDSystemInfo "
        b"<< /Registry (Adobe) /Ordering (Identity) "
        b"/Supplement 0 >> "
        b"/FontDescriptor 6 0 R >>",
        # 6: FontDescriptor (filler — not used by ExtractText)
        b"<< /Type /FontDescriptor /FontName /Identity "
        b"/Flags 4 /FontBBox [0 0 0 0] /ItalicAngle 0 "
        b"/Ascent 0 /Descent 0 /CapHeight 0 /StemV 0 >>",
        # 7: ToUnicode CMap stream
        _stream_object(cmap),
        # 8: Contents stream
        _stream_object(cs),
    ]
    trailer = b" /Size 9 /Root 1 0 R "
    _write("tounicode_acute", _build(objects, trailer),
           "\u00c9\u00f1\u4e2d")


def build_encoding_winansi() -> None:
    """1-page PDF using a Type1 Helvetica with /Encoding
    /WinAnsiEncoding (simple Name form) and NO /ToUnicode.
    Exercises text_extractor v1.1's encoding_tables + agl
    fallback path on bytes that decode DIFFERENTLY under
    WinAnsi vs Latin-1:

      0x80 → /Euro      → U+20AC €
      0x91 → /quoteleft → U+2018 '
      0x99 → /trademark → U+2122 ™
      0xE9 → /eacute    → U+00E9 é (same as Latin-1, anchor case)

    Latin-1 fallback would have produced "\\x80\\x91\\x99\\xE9"
    (raw bytes; 0x80/0x91/0x99 are control characters that both
    pdftotext and pdfminer.six drop or render as junk). With
    /Encoding /WinAnsiEncoding both oracles produce the agreed
    canonical "€'™é".
    """
    # Hex string carrying the four single-byte codes — avoids
    # any literal-string escaping ambiguity for high bytes.
    text_literal = b"<8091 99E9>"
    cs = b"BT /F1 12 Tf 100 700 Td " + text_literal + b" Tj ET"
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        # 4: Type1 Helvetica, /Encoding /WinAnsiEncoding,
        #    NO /ToUnicode — forces oracles AND our v1.1
        #    extractor onto the encoding_tables path.
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica "
        b"/Encoding /WinAnsiEncoding >>",
        # 5: Contents stream
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("encoding_winansi", _build(objects, trailer),
           "\u20ac\u2018\u2122\u00e9")


def build_encoding_dict_basenc() -> None:
    """1-page PDF using a Type1 Helvetica with the DICT form of
    /Encoding:
      /Encoding << /Type /Encoding /BaseEncoding /WinAnsiEncoding >>
    No /Differences and no /ToUnicode. Exercises the dict-shape
    branch of v1.1's GetFontEncoding helper — pulls
    /BaseEncoding out of the dict and walks its table the same
    way the simple-Name form does.

    Same byte-for-byte expected output as encoding_winansi —
    the dict shape is purely a syntactic difference for fonts
    whose authors planned for /Differences but then didn't
    populate it.
    """
    text_literal = b"<8091 99E9>"
    cs = b"BT /F1 12 Tf 100 700 Td " + text_literal + b" Tj ET"
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        # Dict form of /Encoding. PDF 32000 §9.6.6.1 spells
        # "/Type /Encoding" as required for the dict variant;
        # /BaseEncoding selects the underlying table.
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica "
        b"/Encoding << /Type /Encoding "
        b"/BaseEncoding /WinAnsiEncoding >> >>",
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("encoding_dict_basenc", _build(objects, trailer),
           "\u20ac\u2018\u2122\u00e9")


def build_same_line_blocks() -> None:
    """1-page PDF whose single visual line is split across FOUR
    separate BT/ET blocks, each with its own Tm at the SAME
    baseline y (708.7) but increasing x — the shape office
    suites emit for kerning. A per-BT/ET newline heuristic would
    put each fragment on its own line; baseline-aware extraction
    joins them into one line, matching what pdftotext / pdfminer
    report (both reflow same-baseline fragments onto one line).
    Guards the v4 baseline-aware line-break behaviour."""
    cs = (
        b"BT /F1 12 Tf 1 0 0 1 100 708.7 Tm (This ) Tj ET\n"
        b"BT /F1 12 Tf 1 0 0 1 130 708.7 Tm (is ) Tj ET\n"
        b"BT /F1 12 Tf 1 0 0 1 150 708.7 Tm (one ) Tj ET\n"
        b"BT /F1 12 Tf 1 0 0 1 175 708.7 Tm (line) Tj ET"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica >>",
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("same_line_blocks", _build(objects, trailer),
           "This is one line")


def build_two_pages() -> None:
    """2-page PDF — exercises the /Pages-tree preorder walk and
    ToCanonical's form-feed-between-pages branch, both of which
    are unreachable from any single-page fixture in this corpus.
    Each leaf carries its own /Contents and /Resources; the /Pages
    root has /Kids [3 0 R, 4 0 R] /Count 2.

    Both pdftotext and pdfminer.six emit form-feed-delimited
    pages, so post-normalisation the agreed canonical is
    "Page one\\fPage two\\n"."""
    cs1 = b"BT /F1 12 Tf 100 700 Td (Page one) Tj ET"
    cs2 = b"BT /F1 12 Tf 100 700 Td (Page two) Tj ET"
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree root (two leaves)
        b"<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>",
        # 3: Page 1
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 5 0 R >> >> "
        b"/Contents 6 0 R >>",
        # 4: Page 2
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 5 0 R >> >> "
        b"/Contents 7 0 R >>",
        # 5: Font shared between both pages
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica >>",
        # 6: Page 1 contents
        _stream_object(cs1),
        # 7: Page 2 contents
        _stream_object(cs2),
    ]
    trailer = b" /Size 8 /Root 1 0 R "
    _write("two_pages", _build(objects, trailer),
           "Page one\fPage two")


def build_encoding_differences() -> None:
    """1-page PDF using /Encoding << /BaseEncoding /WinAnsiEncoding
    /Differences [128 /Aacute] >>. Exercises text_extractor v2.1's
    /Differences override application: byte 0x80 in plain WinAnsi
    is /Euro → €, but the override remaps it to /Aacute → Á.
    Byte 0x91 retains the WinAnsi default (/quoteleft → ').

    Both pdftotext and pdfminer.six respect /Differences and
    produce the agreed canonical "Á'\\n".

    Without v2.1's /Differences support, v2 would have read
    BaseEncoding for both bytes and produced "€'" — wrong on
    the override slot.
    """
    text_literal = b"<8091>"
    cs = b"BT /F1 12 Tf 100 700 Td " + text_literal + b" Tj ET"
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> "
        b"/Contents 5 0 R >>",
        # Type1 Helvetica with /Encoding dict carrying both
        # /BaseEncoding /WinAnsiEncoding AND a /Differences
        # array that overrides slot 128 to /Aacute.
        b"<< /Type /Font /Subtype /Type1 "
        b"/BaseFont /Helvetica "
        b"/Encoding << /Type /Encoding "
        b"/BaseEncoding /WinAnsiEncoding "
        b"/Differences [128 /Aacute] >> >>",
        _stream_object(cs),
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write("encoding_differences", _build(objects, trailer),
           "\u00c1\u2018")  # Á'


def _check_oracle_agreement(pdf_path: Path) -> str:
    """Run both oracle adapters on `pdf_path` and assert they
    produce byte-identical canonical output. Returns the agreed
    canonical string. Raises RuntimeError on disagreement, naming
    the first divergent character.

    Used by both consumers: the public-API smoke fixture corpus
    and the foundation::text_extractor freeze gate. Same fixtures,
    two different consumers — keeping them in one corpus avoids
    drift between layers."""
    import oracle_pdfminer
    import oracle_pdftotext
    pt = oracle_pdftotext.canonical(pdf_path)
    pm = oracle_pdfminer.canonical(pdf_path)
    if pt == pm:
        return pt
    diff_at = 0
    while diff_at < min(len(pt), len(pm)) and pt[diff_at] == pm[diff_at]:
        diff_at += 1
    raise RuntimeError(
        f"oracle disagreement on {pdf_path.name} at char {diff_at}:\n"
        f"  pdftotext: {pt!r}\n"
        f"  pdfminer:  {pm!r}")


def write_canonicals() -> None:
    """For every generated `.pdf` in this directory, run the
    pdftotext + pdfminer.six oracles, assert agreement, and write
    the agreed canonical to `<name>.canonical`. The freeze-gate
    driver compares the C++ ``foundation::text_extractor::ToCanonical``
    output against these committed sidecars."""
    print("Generating .canonical sidecars (oracle agreement)...")
    for pdf_path in sorted(HERE.glob("*.pdf")):
        canonical = _check_oracle_agreement(pdf_path)
        cp = pdf_path.with_suffix(".canonical")
        cp.write_text(canonical, encoding="utf-8")
        print(f"  {pdf_path.name:24s}  -> {cp.name} "
              f"({len(canonical.encode())} bytes)")


def main() -> None:
    print("Generating Page::ExtractText fixtures...")
    build_hello_world()
    build_two_lines()
    build_tj_array()
    build_tounicode_acute()
    build_encoding_winansi()
    build_encoding_dict_basenc()
    build_encoding_differences()
    build_same_line_blocks()
    build_two_pages()
    write_canonicals()
    print("Done.")


if __name__ == "__main__":
    sys.exit(main())
