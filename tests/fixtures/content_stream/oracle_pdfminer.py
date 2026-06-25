#!/usr/bin/env python3
"""pdfminer.six oracle adapter for content_stream fixtures.

Takes raw content-stream bytes, wraps them in a minimal PDF
page (indirect /Contents stream — pdfminer chokes on direct
embedded streams), drives pdfminer's PDFContentParser, and
returns a list of (op_bytes, [Value tuple]) pairs ready for
ref_parser.canonicalise_stream().

pdfminer.six is oracle 2 of the two_oracle_agreement gate. Its
content-stream parser is pure Python, implemented independently
of pypdf's. Both oracles consume the same wrapped PDF but run
entirely different parsers underneath.
"""

from __future__ import annotations

import io
from typing import Any

from pdfminer.pdfdocument import PDFDocument
from pdfminer.pdfinterp import PDFContentParser
from pdfminer.pdfpage import PDFPage
from pdfminer.pdfparser import PDFParser
from pdfminer.pdftypes import PDFObjRef
from pdfminer.psparser import PSKeyword, PSLiteral, PSEOF


def parse_content_stream(raw: bytes) -> list[tuple[bytes, list[tuple]]]:
    """Parse raw content-stream bytes with pdfminer.six and return
    a normalised operation list."""
    pdf_bytes = _wrap_in_minimal_pdf(raw)
    parser = PDFParser(io.BytesIO(pdf_bytes))
    doc = PDFDocument(parser)
    pages = list(PDFPage.create_pages(doc))
    if not pages:
        raise RuntimeError(
            "pdfminer oracle: minimal PDF wrapper produced 0 pages")
    cp = PDFContentParser(pages[0].contents)
    ops: list[tuple[bytes, list[tuple]]] = []
    stack: list[tuple] = []
    while True:
        try:
            _, obj = cp.nextobject()
        except PSEOF:
            break
        if isinstance(obj, PSKeyword):
            # pdfminer encodes the operator name as bytes in .name
            ops.append((obj.name, stack))
            stack = []
        else:
            stack.append(_normalise(obj))
    if stack:
        # Any operand without a following operator is malformed;
        # let the sidecar-writer see this and decide whether to
        # drop the fixture or fail loud.
        raise RuntimeError(
            f"pdfminer oracle: {len(stack)} trailing operands "
            "without a terminating operator — fixture malformed")
    return ops


def _wrap_in_minimal_pdf(raw: bytes) -> bytes:
    """Build a minimal PDF with /Contents as an INDIRECT stream.
    pdfminer.six's page-walk rejects direct-embedded streams
    (pypdf's default), so we route through pypdf's writer and
    explicitly add the stream as an indirect object.

    Shared code path with oracle_pypdf — the same byte sequence
    is what both oracles parse, which keeps the independence
    guarantee honest (both libraries see identical bytes)."""
    from pypdf import PdfWriter
    from pypdf.generic import NameObject, DecodedStreamObject
    writer = PdfWriter()
    page = writer.add_blank_page(612, 792)
    stream = DecodedStreamObject()
    stream._data = raw
    stream_ref = writer._add_object(stream)
    page[NameObject("/Contents")] = stream_ref
    buf = io.BytesIO()
    writer.write(buf)
    return buf.getvalue()


def _normalise(value: Any) -> tuple:
    """Convert a pdfminer.six value to the shared tuple form.
    pdfminer returns native Python types for most things —
    bool for booleans, int/float for numbers, bytes for
    strings, PSLiteral for names, list for arrays, dict for
    dicts, PDFObjRef for indirect references."""
    if value is None:
        return ("null",)
    if isinstance(value, bool):
        # Check before int since bool is a subclass of int.
        return ("bool", value)
    if isinstance(value, int):
        return ("int", value)
    if isinstance(value, float):
        # pdfminer doesn't preserve the source digit form.
        return ("real", value, repr(value))
    if isinstance(value, bytes):
        return ("string", value)
    if isinstance(value, PSLiteral):
        # PSLiteral.name is the decoded name bytes (or str in
        # older pdfminer versions). Normalise to bytes.
        name = value.name
        if isinstance(name, str):
            name = name.encode("latin-1", errors="replace")
        return ("name", bytes(name))
    if isinstance(value, PSKeyword):
        # Shouldn't appear in operand position — keywords ARE
        # operators — but pdfminer occasionally leaks one in
        # malformed streams. Surface it as a string for visibility
        # so the sidecar compare catches the oddity.
        return ("string", bytes(value.name))
    if isinstance(value, list):
        return ("array", [_normalise(v) for v in value])
    if isinstance(value, dict):
        entries = []
        for k, v in value.items():
            if isinstance(k, str):
                key_bytes = k.encode("latin-1", errors="replace")
            elif isinstance(k, bytes):
                key_bytes = k
            else:
                key_bytes = str(k).encode("latin-1", errors="replace")
            entries.append((key_bytes, _normalise(v)))
        return ("dict", entries)
    if isinstance(value, PDFObjRef):
        return ("ref", int(value.objid), 0)  # generation unknown
    raise TypeError(
        f"pdfminer oracle: unrecognised value type "
        f"{type(value).__name__} ({value!r})")
