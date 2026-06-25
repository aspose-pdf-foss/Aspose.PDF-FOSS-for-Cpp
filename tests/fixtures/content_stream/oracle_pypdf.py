#!/usr/bin/env python3
"""pypdf oracle adapter for content_stream fixtures.

Takes raw content-stream bytes, wraps them in a minimal PDF
page, reads the page via pypdf, iterates the content stream's
operations, and returns a list of (op_bytes, [Value tuple])
pairs ready for ref_parser.canonicalise_stream().

pypdf is oracle 1 of the two_oracle_agreement gate for
content_stream. Its content-stream parser is pure Python,
implemented independently of pdfminer.six. Any bug common to
both would need to affect TWO independent parsers — unlikely
for non-adversarial fixtures.
"""

from __future__ import annotations

import io
from typing import Any

from pypdf import PdfReader, PdfWriter
from pypdf.generic import (
    ArrayObject,
    BooleanObject,
    ByteStringObject,
    ContentStream,
    DecodedStreamObject,
    DictionaryObject,
    FloatObject,
    IndirectObject,
    NameObject,
    NullObject,
    NumberObject,
    TextStringObject,
)


def parse_content_stream(raw: bytes) -> list[tuple[bytes, list[tuple]]]:
    """Parse the raw content-stream bytes with pypdf and return a
    normalised operation list. Raises on any pypdf error — caller
    should catch to handle malformed fixtures."""
    pdf_bytes = _wrap_in_minimal_pdf(raw)
    reader = PdfReader(io.BytesIO(pdf_bytes))
    page = reader.pages[0]
    # ContentStream(stream, pdf) handles the decoded body + operand
    # iteration. The operations attribute is a list of
    # (operand_list, op_bytes) tuples.
    cs = ContentStream(page["/Contents"], reader)
    ops: list[tuple[bytes, list[tuple]]] = []
    for operands, op in cs.operations:
        ops.append((op, [_normalise(v) for v in operands]))
    return ops


def _wrap_in_minimal_pdf(raw: bytes) -> bytes:
    """Wrap raw content-stream bytes in the smallest valid PDF
    that pdfminer.six and pypdf both accept: one page, /Contents
    is an INDIRECT stream (pypdf's default direct-embedded form
    trips up pdfminer — see sibling oracle adapter), no resources,
    no fonts (fonts aren't needed to tokenise content streams)."""
    writer = PdfWriter()
    page = writer.add_blank_page(612, 792)
    stream = DecodedStreamObject()
    # pypdf's DecodedStreamObject stores body bytes on ._data;
    # calling set_data() would re-encode via flate which we don't
    # want (the oracles need to see our raw bytes).
    stream._data = raw
    stream_ref = writer._add_object(stream)
    page[NameObject("/Contents")] = stream_ref
    buf = io.BytesIO()
    writer.write(buf)
    return buf.getvalue()


def _normalise(value: Any) -> tuple:
    """Convert a pypdf typed value into the shared tuple form
    documented in ref_parser.py. Dispatches on Python type +
    isinstance checks against pypdf's typed wrapper classes.

    Order of isinstance checks matters: NumberObject subclasses
    int, FloatObject subclasses float, so the specific type
    check comes before its base-class check.
    """
    if isinstance(value, NullObject) or value is None:
        return ("null",)
    if isinstance(value, BooleanObject):
        # BooleanObject wraps a bool — compare against .value.
        return ("bool", bool(value))
    if isinstance(value, FloatObject):
        # FloatObject keeps the source-form string in .original_str
        # on newer pypdf versions; fall back to str(value) which
        # round-trips for the ranges PDF numerics use.
        source = str(value)
        return ("real", float(value), source)
    if isinstance(value, NumberObject):
        # NumberObject is the integer specialisation.
        return ("int", int(value))
    if isinstance(value, NameObject):
        # NameObject is str; strip the leading '/' and decode any
        # '#XX' escapes to raw bytes.
        name_str = str(value)
        if name_str.startswith("/"):
            name_str = name_str[1:]
        return ("name", _decode_name_escapes(name_str))
    if isinstance(value, ByteStringObject):
        return ("string", bytes(value))
    if isinstance(value, TextStringObject):
        # TextStringObject represents a PDFDocEncoding / UTF-16BE
        # decoded string. For canonical comparison we want the
        # RAW bytes — pypdf stores them on .original_bytes when
        # the decode succeeded. If not available, re-encode the
        # Unicode content as PDFDocEncoding.
        raw = getattr(value, "original_bytes", None)
        if raw is None:
            raw = str(value).encode("utf-8", errors="replace")
        return ("string", bytes(raw))
    if isinstance(value, ArrayObject):
        return ("array", [_normalise(v) for v in value])
    if isinstance(value, DictionaryObject):
        entries = []
        for key, val in value.items():
            # Dictionary keys in PDF are names; strip the '/'.
            k_str = str(key)
            if k_str.startswith("/"):
                k_str = k_str[1:]
            entries.append(
                (_decode_name_escapes(k_str), _normalise(val)))
        return ("dict", entries)
    if isinstance(value, IndirectObject):
        return ("ref", int(value.idnum), int(value.generation))
    # Plain Python primitives fall through when pypdf passes them
    # unwrapped (strings inside operands are typically
    # ByteStringObject but a bare bytes slips through in some
    # code paths).
    if isinstance(value, bool):
        return ("bool", value)
    if isinstance(value, int):
        return ("int", value)
    if isinstance(value, float):
        return ("real", float(value), repr(value))
    if isinstance(value, bytes):
        return ("string", value)
    if isinstance(value, str):
        return ("string", value.encode("latin-1", errors="replace"))
    raise TypeError(
        f"pypdf oracle: unrecognised value type {type(value).__name__} "
        f"({value!r})")


def _decode_name_escapes(s: str) -> bytes:
    """Decode '#XX' escape sequences inside a PDF name back to
    raw bytes. Matches §7.3.5 — '#XX' is always two uppercase
    hex digits (we accept lowercase too for tolerance).
    Characters outside the escape set pass through as their
    latin-1 byte values (PDF names are byte sequences, not
    Unicode text, so latin-1 is a byte-preserving encoding)."""
    out = bytearray()
    i = 0
    while i < len(s):
        if s[i] == "#" and i + 2 < len(s):
            try:
                out.append(int(s[i + 1:i + 3], 16))
                i += 3
                continue
            except ValueError:
                pass  # fall through as literal '#'
        out.append(ord(s[i]) & 0xFF)
        i += 1
    return bytes(out)
