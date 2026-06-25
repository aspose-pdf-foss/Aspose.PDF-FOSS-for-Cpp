#!/usr/bin/env python3
"""Generate minimal PDF fixtures exercising PDF 32000 §7.2–7.3 lexer edge cases.

Each fixture is a valid PDF whose Catalog carries an extra ``/Test``
entry that exercises a specific token category. All fixtures share
the same three-object skeleton (Catalog + Pages + Page) so the
interesting bytes live in a predictable place and every fixture
passes ``qpdf --check`` cleanly.

Run: ``python3 generate_fixtures.py``.
Fixtures are written next to this script as ``*.pdf``.

The lexer's freeze gate is byte-identical round-trip: tokenise → concat
raw bytes → assert equals input bytes. Each fixture below is designed
to break that assertion if the lexer drops, merges, normalises, or
reorders any token class.
"""

from __future__ import annotations

from pathlib import Path


def build_pdf(catalog_extra: bytes, *, eol: bytes = b"\n") -> bytes:
    """Assemble a minimal valid PDF around an extra Catalog entry.

    ``catalog_extra`` is appended to the Catalog dictionary before
    its closing ``>>``. The builder emits a three-object skeleton
    (Catalog + Pages + single Page), a cross-reference table with
    correct byte offsets computed from the produced bytes, and the
    trailer. ``eol`` lets fixtures exercise different end-of-line
    conventions (LF / CR / CRLF are all valid per §7.2.3).

    A single Page object is emitted so ``qpdf --check`` is happy —
    its page-tree validator rejects zero-page documents with a
    non-specific ``ERROR: vector``. Every fixture wants a clean
    --check pass because that's our sanity-check that we've built
    a valid PDF at all.
    """
    # PDF header + binary-safe magic comment. The four high-bit
    # bytes tell downstream tools the file is binary-safe — required
    # so text-mode transfers don't mangle the file.
    output = bytearray()
    output += b"%PDF-1.4" + eol
    output += b"%\xe2\xe3\xcf\xd3" + eol

    # Object 1 — Catalog with the caller-supplied extra entry.
    obj1_offset = len(output)
    output += b"1 0 obj" + eol
    output += b"<< /Type /Catalog /Pages 2 0 R " + catalog_extra + b" >>" + eol
    output += b"endobj" + eol

    # Object 2 — Pages with exactly one kid (object 3).
    obj2_offset = len(output)
    output += b"2 0 obj" + eol
    output += b"<< /Type /Pages /Count 1 /Kids [3 0 R] >>" + eol
    output += b"endobj" + eol

    # Object 3 — minimal Page with an A0-ish MediaBox and an empty
    # /Resources dict. /Resources is required by §7.7.3.3; omitting
    # it produces a qpdf-warning-with-repair. Empty content is
    # legal and renders as blank.
    obj3_offset = len(output)
    output += b"3 0 obj" + eol
    output += (b"<< /Type /Page /Parent 2 0 R"
               b" /MediaBox [0 0 100 100] /Resources << >> >>" + eol)
    output += b"endobj" + eol

    # xref table — four slots (obj 0..3). Entry 0 is the head of
    # the free list (always "0000000000 65535 f "); entries 1–3
    # point at the object offsets we tracked above. xref entries
    # are always 20 bytes each and end with "\n" or " \n" per §7.5.4
    # regardless of the file's general EOL convention.
    xref_offset = len(output)
    output += b"xref" + eol
    output += b"0 4" + eol
    output += b"0000000000 65535 f \n"
    output += f"{obj1_offset:010d} 00000 n \n".encode("latin-1")
    output += f"{obj2_offset:010d} 00000 n \n".encode("latin-1")
    output += f"{obj3_offset:010d} 00000 n \n".encode("latin-1")

    # Trailer + startxref + %%EOF. /Size is the number of object
    # slots including the free slot 0.
    output += b"trailer" + eol
    output += b"<< /Size 4 /Root 1 0 R >>" + eol
    output += b"startxref" + eol
    output += f"{xref_offset}".encode("latin-1") + eol
    output += b"%%EOF" + eol

    return bytes(output)


# ---------------------------------------------------------------- fixtures


def fixture_numbers() -> bytes:
    """Number token edge cases per §7.3.3.

    Exercises: positive integer, negative integer, explicit-sign
    positive, zero, negative zero, float with leading dot, float
    with trailing dot, standard float, negative float. A correct
    lexer preserves each sign character and decimal-point location
    verbatim; a lossy lexer that canonicalises numbers (e.g.
    converts -0 to 0, or .5 to 0.5) fails the round-trip gate.
    """
    return build_pdf(b"/Test [ 42 -17 +8 0 -0 .5 5. 3.14 -0.25 ]")


def fixture_names() -> bytes:
    """Name token edge cases per §7.3.5.

    Exercises: simple name, name with #hh hex escape (space via
    #20), a hex-escape of the escape char itself (#23 = '#'),
    name starting with digits, name containing special chars via
    hex. A lexer that prematurely decodes hex escapes fails
    round-trip because the original #XX bytes must be preserved
    at the lexer layer — decoding is parser business.
    """
    return build_pdf(
        b"/Test [ /Simple /With#20Space /Escaped#23Hash"
        b" /123Starts /Mix#7Eed ]")


def fixture_strings_literal() -> bytes:
    """Literal string edge cases per §7.3.4.2.

    Exercises: plain string, string with newline escape, string
    with nested balanced parens (permitted unescaped), string
    ending in escaped backslash, and string with octal escape
    (\\053 → '+'). A lexer that collapses escape sequences at
    tokenisation time loses the original bytes and fails round-trip
    — escapes are resolved at the parser layer.
    """
    return build_pdf(
        b"/Test [ (hello) (with\\nnewline) (outer (inner) text)"
        b" (text\\\\) (\\053plus) ]")


def fixture_strings_hex() -> bytes:
    """Hex string edge cases per §7.3.4.3.

    Exercises: standard hex string, odd-length hex (spec says pad
    trailing zero at decode time — bytes must round-trip as-is),
    hex with embedded whitespace (ignored at decode but bytes must
    round-trip), empty hex string. A lexer that normalises or
    removes whitespace inside hex fails round-trip.
    """
    return build_pdf(b"/Test [ <48656c6c6f> <48F> <48 65 6C> <> ]")


def fixture_comments() -> bytes:
    """Comment edge cases per §7.2.4.

    Exercises: comment at end of line inside object, inline
    comment between array elements. Comments run from '%' to the
    next EOL and are treated as whitespace semantically — but a
    trivia-preserving lexer must keep them as tokens so the
    original file reconstructs byte-for-byte.
    """
    return build_pdf(b"/Test [ 1 % inline comment\n 2 %another\n 3 ]")


def fixture_eol_variants() -> bytes:
    """Bare-CR end-of-line convention per §7.2.3.

    PDF permits three EOL conventions: LF, CR, CRLF. The default
    builder uses LF (exercised by every other fixture). This
    fixture builds with bare CR throughout so the lexer sees the
    third EOL byte sequence. A lexer that only recognises LF or
    that merges CR+text as one token fails round-trip.

    CRLF is a superset case (LF + preceding CR both appear) and
    is less likely to trip a lexer — covered by the bare CR case.
    """
    return build_pdf(b"/Test 42", eol=b"\r")


FIXTURES = {
    "numbers.pdf": fixture_numbers,
    "names.pdf": fixture_names,
    "strings_literal.pdf": fixture_strings_literal,
    "strings_hex.pdf": fixture_strings_hex,
    "comments.pdf": fixture_comments,
    "eol_variants.pdf": fixture_eol_variants,
}


def main() -> None:
    out_dir = Path(__file__).parent
    for name, make in FIXTURES.items():
        data = make()
        (out_dir / name).write_bytes(data)
        print(f"wrote {name} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
