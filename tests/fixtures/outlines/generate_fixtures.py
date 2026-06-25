#!/usr/bin/env python3
"""Generate the PDF fixture corpus for the foundation/outlines
primitive.

Each fixture exercises a distinct outline tree shape:

* ``empty.pdf``   — no /Outlines on the Catalog. The
                    canonical form is the empty string.
                    Validates the absent-/Outlines branch.
* ``flat.pdf``    — three top-level items, no children.
                    Validates the /Next sibling walk and the
                    PDFDocEncoding /Title decode (titles are
                    pure ASCII; same byte-for-byte output as
                    UTF-8).
* ``nested.pdf``  — two top-level chapters, the first with
                    two children. Tree shape:
                      Chapter 1
                        Section 1.1
                        Section 1.2
                      Chapter 2
                    Validates the recursive /First descent
                    interleaved with /Next sibling walk.

Each fixture is paired with a ``.canonical`` sidecar produced
by the oracle scripts in this directory after both qpdf and
mutool agree on the output.

Run: ``python3 generate_fixtures.py`` from this directory.
"""

from __future__ import annotations

from pathlib import Path

HERE = Path(__file__).resolve().parent


# ---------------------------------------------------------------------------
# PDF assembly helpers — same shape as the text_extractor + trailer
# fixture generators. Indirect-object indices are 1-based in the
# PDF and 0-based in the python list.
# ---------------------------------------------------------------------------


def _build(objects: list[bytes], trailer: bytes) -> bytes:
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


def _write(name: str, contents: bytes) -> None:
    pdf = HERE / f"{name}.pdf"
    pdf.write_bytes(contents)
    print(f"  wrote {pdf.name}  ({len(contents)} bytes)")


# ---------------------------------------------------------------------------
# Fixture builders
# ---------------------------------------------------------------------------


def build_empty() -> None:
    """1-page PDF with NO /Outlines on the Catalog."""
    objects = [
        # 1: Catalog (no /Outlines)
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write("empty", _build(objects, trailer))


def build_flat() -> None:
    """1-page PDF with a flat outline: 3 top-level items."""
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R /Outlines 3 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [4 0 R] /Count 1 >>",
        # 3: Outline root
        b"<< /Type /Outlines /First 5 0 R /Last 7 0 R "
        b"/Count 3 >>",
        # 4: Page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << >> >>",
        # 5: Item 1 (Introduction)
        b"<< /Title (Introduction) /Parent 3 0 R "
        b"/Next 6 0 R >>",
        # 6: Item 2 (Methods)
        b"<< /Title (Methods) /Parent 3 0 R "
        b"/Prev 5 0 R /Next 7 0 R >>",
        # 7: Item 3 (Conclusion)
        b"<< /Title (Conclusion) /Parent 3 0 R "
        b"/Prev 6 0 R >>",
    ]
    trailer = b" /Size 8 /Root 1 0 R "
    _write("flat", _build(objects, trailer))


def build_nested() -> None:
    """1-page PDF with a 2-level nested outline:
        Chapter 1
          Section 1.1
          Section 1.2
        Chapter 2
    """
    objects = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R /Outlines 3 0 R >>",
        # 2: Pages tree
        b"<< /Type /Pages /Kids [4 0 R] /Count 1 >>",
        # 3: Outline root: First 5 (Chapter 1), Last 8 (Chapter 2)
        b"<< /Type /Outlines /First 5 0 R /Last 8 0 R "
        b"/Count 4 >>",
        # 4: Page
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 612 792] "
        b"/Resources << >> >>",
        # 5: Chapter 1: First 6 (Section 1.1), Last 7 (Section 1.2),
        #    Next 8 (Chapter 2)
        b"<< /Title (Chapter 1) /Parent 3 0 R "
        b"/First 6 0 R /Last 7 0 R /Next 8 0 R "
        b"/Count 2 >>",
        # 6: Section 1.1
        b"<< /Title (Section 1.1) /Parent 5 0 R "
        b"/Next 7 0 R >>",
        # 7: Section 1.2
        b"<< /Title (Section 1.2) /Parent 5 0 R "
        b"/Prev 6 0 R >>",
        # 8: Chapter 2 (no children)
        b"<< /Title (Chapter 2) /Parent 3 0 R "
        b"/Prev 5 0 R >>",
    ]
    trailer = b" /Size 9 /Root 1 0 R "
    _write("nested", _build(objects, trailer))


# ---------------------------------------------------------------------------
# Oracle agreement
# ---------------------------------------------------------------------------


def _check_oracle_agreement(pdf_path: Path) -> str:
    """Run both oracle adapters and assert byte-identical output."""
    import oracle_mutool
    import oracle_qpdf
    pt = oracle_qpdf.adapt(pdf_path)
    pm = oracle_mutool.adapt(pdf_path)
    if pt == pm:
        return pt
    diff_at = 0
    while diff_at < min(len(pt), len(pm)) and pt[diff_at] == pm[diff_at]:
        diff_at += 1
    raise RuntimeError(
        f"oracle disagreement on {pdf_path.name} at char {diff_at}:\n"
        f"  qpdf:   {pt!r}\n"
        f"  mutool: {pm!r}")


def write_canonicals() -> None:
    print("Generating .canonical sidecars (oracle agreement)...")
    for pdf_path in sorted(HERE.glob("*.pdf")):
        canonical = _check_oracle_agreement(pdf_path)
        cp = pdf_path.with_suffix(".canonical")
        cp.write_text(canonical, encoding="utf-8")
        print(f"  {pdf_path.name:24s}  -> {cp.name} "
              f"({len(canonical.encode())} bytes)")


def main() -> int:
    print("Generating foundation/outlines fixtures...")
    build_empty()
    build_flat()
    build_nested()
    write_canonicals()
    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
