#!/usr/bin/env python3
"""Generate minimal PDF fixtures carrying document metadata —
both the legacy /Info dictionary (consumed by the DocumentInfo
capability) and the modern /Catalog/Metadata XMP packet
(consumed by the Metadata capability).

/Info fixtures:

* ``with_info.pdf`` — /Info present with every standard field
  populated. Establishes the floor for DocumentInfo accessors.
* ``without_info.pdf`` — no /Info entry in the trailer.
  Establishes empty-fallback behaviour.

XMP fixtures (PDF 32000 §14.3.2):

* ``with_xmp.pdf`` — /Catalog has /Metadata <ref> pointing to
  a /Type /Metadata /Subtype /XML stream that holds an XMP
  packet covering dc:title (Alt), dc:creator (Seq),
  pdf:Producer, xmp:CreateDate, xmp:CreatorTool. Establishes
  the floor for "Metadata accessors decode XMP correctly."
* ``without_xmp.pdf`` — no /Metadata entry on the Catalog.
  Establishes that Metadata reads cleanly and surfaces an
  empty property bag without throwing.

Canonical expected accessor values are documented inside each
fixture's header comment and re-asserted in the smoke tests
at the API spec{document_info,metadata}_test.cpp.

Run: ``python3 generate_fixtures.py`` from this directory.
"""

from __future__ import annotations

from pathlib import Path


HERE = Path(__file__).parent


def _build(objects: list[bytes], trailer: bytes) -> bytes:
    """Assemble a minimal PDF 1.4 from a list of object bodies and
    a trailer dictionary body. Returns the full file bytes,
    including a classical single-subsection xref table and the
    startxref trailer.

    Each element in ``objects`` is the BODY between ``N 0 obj\\n``
    and ``\\nendobj``, not the whole indirect-object block. The
    trailer argument is the BODY between ``trailer\\n<<`` and
    ``>>\\n``.
    """
    header = b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n"
    out = bytearray(header)
    offsets: list[int] = []
    for i, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode()
        out += body
        out += b"\nendobj\n"
    # Classical xref with a single subsection covering object 0..N.
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


def _pdf_string(s: str) -> bytes:
    """Encode a PDF literal string. PDF 32000 §7.3.4.2 allows
    parenthesised strings with balanced nested parens; our
    values don't need escaping so the cheap form is enough."""
    return b"(" + s.encode("pdfdocencoding", errors="replace") \
        + b")" if False else b"(" + s.encode("ascii") + b")"


def _write(path: Path, contents: bytes) -> None:
    path.write_bytes(contents)
    print(f"  wrote {path.relative_to(HERE.parents[2])}"
          f"  ({len(contents)} bytes)")


def build_with_info() -> None:
    """A minimal PDF with every /Info field populated."""
    info_fields = {
        "Title":        "LibForge Test Document",
        "Author":       "LibForge",
        "Subject":      "DocumentInfo smoke fixture",
        "Keywords":     "libforge test smoke metadata",
        "Creator":      "Hand-authored by generate_fixtures.py",
        "Producer":     "LibForge Fixtures",
        "CreationDate": "D:20260424000000+00'00'",
        "ModDate":      "D:20260424000000+00'00'",
    }
    info_body = b"<< "
    for k, v in info_fields.items():
        info_body += f"/{k} ".encode() + _pdf_string(v) + b" "
    info_body += b">>"

    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Single page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        # 4: /Info dictionary
        info_body,
    ]
    trailer = b" /Size 5 /Root 1 0 R /Info 4 0 R "
    _write(HERE / "with_info.pdf", _build(objects, trailer))


def build_without_info() -> None:
    """Identical structure but no /Info entry in the trailer."""
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write(HERE / "without_info.pdf", _build(objects, trailer))


def _xmp_packet() -> bytes:
    """The XMP packet embedded in ``with_xmp.pdf``. Hand-authored
    so the canonical expected values are obvious by reading the
    XML. The leading byte-order-mark inside the xpacket header is
    the canonical UTF-8 BOM (\xEF\xBB\xBF), present in every
    well-formed XMP packet per ISO 16684-1.

    Reference XmpMetadata.Get expected values (multi-value
    Seq/Bag joined with "; ", Alt flattened to x-default):

        dc:title              LibForge Test Document
        dc:creator            LibForge; LibForge Contributors
        dc:description        Metadata smoke fixture
        pdf:Producer          LibForge Fixtures
        xmp:CreateDate        2026-04-26T00:00:00Z
        xmp:ModifyDate        2026-04-26T00:00:00Z
        xmp:CreatorTool       Hand-authored by generate_fixtures.py
        pdfaid:part           1
        pdfaid:conformance    B
    """
    return (
        b'<?xpacket begin="\xef\xbb\xbf" '
        b'id="W5M0MpCehiHzreSzNTczkc9d"?>\n'
        b'<x:xmpmeta xmlns:x="adobe:ns:meta/">\n'
        b'  <rdf:RDF '
        b'xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">\n'
        b'    <rdf:Description rdf:about=""\n'
        b'        xmlns:dc="http://purl.org/dc/elements/1.1/"\n'
        b'        xmlns:xmp="http://ns.adobe.com/xap/1.0/"\n'
        b'        xmlns:pdf="http://ns.adobe.com/pdf/1.3/"\n'
        b'        xmlns:pdfaid="http://www.aiim.org/pdfa/ns/id/">\n'
        b'      <dc:title>\n'
        b'        <rdf:Alt>\n'
        b'          <rdf:li xml:lang="x-default">'
        b'LibForge Test Document</rdf:li>\n'
        b'        </rdf:Alt>\n'
        b'      </dc:title>\n'
        b'      <dc:creator>\n'
        b'        <rdf:Seq>\n'
        b'          <rdf:li>LibForge</rdf:li>\n'
        b'          <rdf:li>LibForge Contributors</rdf:li>\n'
        b'        </rdf:Seq>\n'
        b'      </dc:creator>\n'
        b'      <dc:description>\n'
        b'        <rdf:Alt>\n'
        b'          <rdf:li xml:lang="x-default">'
        b'Metadata smoke fixture</rdf:li>\n'
        b'        </rdf:Alt>\n'
        b'      </dc:description>\n'
        b'      <pdf:Producer>LibForge Fixtures</pdf:Producer>\n'
        b'      <xmp:CreateDate>2026-04-26T00:00:00Z'
        b'</xmp:CreateDate>\n'
        b'      <xmp:ModifyDate>2026-04-26T00:00:00Z'
        b'</xmp:ModifyDate>\n'
        b'      <xmp:CreatorTool>'
        b'Hand-authored by generate_fixtures.py'
        b'</xmp:CreatorTool>\n'
        b'      <pdfaid:part>1</pdfaid:part>\n'
        b'      <pdfaid:conformance>B</pdfaid:conformance>\n'
        b'    </rdf:Description>\n'
        b'  </rdf:RDF>\n'
        b'</x:xmpmeta>\n'
        b'<?xpacket end="r"?>'
    )


def build_with_xmp() -> None:
    """A minimal PDF carrying /Catalog/Metadata pointing at an
    XMP packet stream. /Info is omitted so the smoke test can
    assert that Metadata works independently of DocumentInfo —
    XMP is the modern replacement, not a supplement.

    Stream object 4 carries the XMP packet verbatim with
    /Type /Metadata /Subtype /XML — no /Filter (PDF 32000
    §14.3.2 forbids compression on metadata streams that
    must be readable by tools without a PDF parser).
    """
    xmp = _xmp_packet()
    # Stream object: dictionary then "stream\n<bytes>\nendstream".
    # /Length is the byte count of the body, which excludes the
    # newline immediately after `stream` and the newline before
    # `endstream` per PDF 32000 §7.3.8.1.
    stream_body = (
        f"<< /Type /Metadata /Subtype /XML "
        f"/Length {len(xmp)} >>\n"
        f"stream\n"
    ).encode() + xmp + b"\nendstream"

    objects = [
        # 1: Catalog with /Metadata reference
        b"<< /Type /Catalog /Pages 2 0 R /Metadata 4 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Single page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
        # 4: XMP metadata stream
        stream_body,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "with_xmp.pdf", _build(objects, trailer))


def build_without_xmp() -> None:
    """Identical structure but no /Metadata entry on the Catalog —
    the legacy case for PDFs that predate XMP. Metadata::ReadFromBytes
    must return an empty Properties bag, NOT throw.
    """
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write(HERE / "without_xmp.pdf", _build(objects, trailer))


def main() -> None:
    build_with_info()
    build_without_info()
    build_with_xmp()
    build_without_xmp()


if __name__ == "__main__":
    main()
