#!/usr/bin/env python3
"""Generate the PDF fixture corpus for the foundation/objects primitive.

The objects primitive's freeze gate is two_oracle_agreement — for
each fixture PDF, the C++ implementation must produce a text dump
(one line per in-use object, sorted by (id, gen)) that matches the
cached ``<fixture>.canonical`` sidecar. Each sidecar is produced by
running qpdf and mutool (via oracle_qpdf.py + oracle_mutool.py, both
feeding ref_parser for normalisation) and asserting byte-identical
output — fixtures on which the two oracles disagree are excluded
before freeze.

Canonical form (per objects.shared.yaml):

    <id> <gen> <value_canonical>

See ref_parser.canonical() for the per-kind grammar. Strings are
rendered as ``<uppercase-hex>`` regardless of source form; dict
keys are sorted alphabetically; reals use shortest round-trip
decimal with a ``.``.

Fixtures:

  * ``primitives.pdf`` — one indirect object per primitive kind:
    null, bool, integer (positive/negative), real (various
    precisions), name, literal string, hex string, empty/non-empty
    array, empty/non-empty dict, indirect reference. Establishes
    the floor for per-kind parsing.
  * ``nested.pdf`` — arrays containing dicts containing arrays,
    several levels deep. Exposes recursion-depth and value-copying
    bugs.
  * ``stream.pdf`` — a single stream object with /Length + /Filter
    in the header. Body bytes are deterministic (FlateDecode of a
    short ASCII payload) so the /Length entry is stable. Exposes
    stream boundary-handling: the body lives between ``stream\\n``
    and ``endstream`` and is skipped over by ``/Length`` bytes.
  * ``edge_escapes.pdf`` — strings exercising every escape kind
    (``\\n``, ``\\(``, octal, line continuation); names using
    ``#XX`` hex escapes for delimiters.
  * ``number_edges.pdf`` — numeric edge cases: leading ``+``,
    leading ``.``, trailing ``.``, zero and negative zero, very
    small / large integers within 32-bit range.

Run: ``python3 generate_fixtures.py``.
The script aborts on first oracle disagreement, naming the fixture
and showing a diff so the author can tighten the fixture or the
adapter.
"""

from __future__ import annotations

import struct
import subprocess
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent


# ---------------------------------------------------------------- builders


def _header(version: bytes = b"1.4") -> bytearray:
    """Common PDF header + binary-safe marker comment."""
    out = bytearray()
    out += b"%PDF-" + version + b"\n"
    out += b"%\xe2\xe3\xcf\xd3\n"
    return out


def _render_classical_xref(
        subsections: list[tuple[int, list[tuple[int, int, bytes]]]]) -> bytes:
    """Render a classical xref table in the 20-byte fixed-width
    format per §7.5.4."""
    out = bytearray()
    out += b"xref\n"
    for first_id, entries in subsections:
        out += f"{first_id} {len(entries)}\n".encode("latin-1")
        for offset, gen, status in entries:
            out += f"{offset:010d} {gen:05d} ".encode("latin-1")
            out += status + b" \n"
    return bytes(out)


def _assemble(object_bodies: list[bytes]) -> bytes:
    """Build a PDF containing the given object bodies. object_bodies[i]
    is the raw bytes of object ``i+1`` (the content between
    ``N 0 obj\\n`` and ``\\nendobj\\n`` inclusive of surrounding
    newlines). Object 1 is conventionally the Catalog."""
    body = _header()
    offsets: list[int] = []
    for i, content in enumerate(object_bodies, start=1):
        offsets.append(len(body))
        body += f"{i} 0 obj\n".encode("latin-1")
        body += content
        if not content.endswith(b"\n"):
            body += b"\n"
        body += b"endobj\n"
    xref_off = len(body)
    entries = [(0, 65535, b"f")] + [(o, 0, b"n") for o in offsets]
    body += _render_classical_xref([(0, entries)])
    body += (
        f"trailer\n<< /Size {len(object_bodies) + 1} /Root 1 0 R >>\n"
    ).encode("latin-1")
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_primitives() -> bytes:
    """One indirect object per primitive kind.

    Objects 1-3 are the standard catalog/pages/page skeleton so the
    file validates as a PDF even though objects 4+ are semantically
    unreferenced. Unreferenced objects are still emitted by both
    qpdf --show-object and mutool show — neither oracle garbage-
    collects when called on a single object.
    """
    objs: list[bytes] = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        # 4: null
        b"null",
        # 5: true
        b"true",
        # 6: false
        b"false",
        # 7: positive integer
        b"42",
        # 8: negative integer
        b"-17",
        # 9: real with integer part + fraction
        b"3.14",
        # 10: real with leading sign, no integer-part digits-only
        b"-0.5",
        # 11: name
        b"/Hello",
        # 12: literal string
        b"(Hello)",
        # 13: hex string (note: uppercase/lowercase hex normalises
        #     to the same bytes; canonical emits uppercase)
        b"<48656c6c6f>",
        # 14: empty array
        b"[ ]",
        # 15: integer array
        b"[ 1 2 3 ]",
        # 16: empty dict
        b"<< >>",
        # 17: small dict
        b"<< /A 1 /B 2 >>",
        # NOTE: a top-level indirect object whose body is itself a
        # bare indirect reference (``18 0 obj\\n2 0 R\\nendobj\\n``)
        # is syntactically valid per §7.3.10 but qpdf rejects it with
        # "expected endobj". Refs are covered inside arrays/dicts by
        # primitives objs 1-2 and by nested.pdf — no need to force a
        # bare-ref case here.
    ]
    return _assemble(objs)


def build_nested() -> bytes:
    """Nested composite values — arrays inside dicts inside arrays.

    The nesting depth (4-5 levels) is enough to expose recursion
    bugs in value copying or canonicalisation. Keeping the leaves
    simple (ints, names) isolates the structural parts.
    """
    objs: list[bytes] = [
        # 1: Catalog — valid PDF skeleton.
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        # 4: dict-in-array-in-dict-in-array. Four levels deep.
        b"[ << /K [ << /X 1 /Y 2 >> 3 ] >> [ 4 5 ] ]",
        # 5: dict with many keys, a mix of value kinds. Source order
        #    is deliberately shuffled to make canonical's alphabetical
        #    sort visible (Z first in source, but canonical starts
        #    with /A).
        b"<< /Z 1 /A /name /M (str) /B [ 1 2 ] /K << /X 1 >> >>",
        # 6: array of mixed kinds including a ref — covers the
        #    ref-vs-integer disambiguation at every array position.
        b"[ 1 2 0 R 3 /name (str) << >> ]",
    ]
    return _assemble(objs)


def build_stream() -> bytes:
    """One stream object with /Length + /Filter.

    The body is FlateDecode of a short ASCII payload; /Length
    carries the compressed byte count. The canonical form for this
    object is::

        N 0 stream << /Filter /FlateDecode /Length <L> >>

    where ``<L>`` is the deterministic compressed length — we
    recompute it at build time below so the fixture stays sealed.
    """
    payload = b"Stream content for the objects fixture.\n"
    compressed = zlib.compress(payload, 6)  # level-6, deterministic

    objs: list[bytes] = [
        # 1: Catalog
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # 2: Pages
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        # 3: Page
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
    ]

    # Object 4 is the stream — assemble its bytes manually because
    # _assemble wraps only non-stream bodies. We append it to the
    # PDF after the skeleton.
    body = _header()
    offsets: list[int] = []
    for i, content in enumerate(objs, start=1):
        offsets.append(len(body))
        body += f"{i} 0 obj\n".encode("latin-1") + content + b"\nendobj\n"

    # Stream object.
    offsets.append(len(body))
    dict_bytes = (
        f"<< /Filter /FlateDecode /Length {len(compressed)} >>"
    ).encode("latin-1")
    body += b"4 0 obj\n" + dict_bytes + b"\nstream\n"
    body += compressed + b"\nendstream\nendobj\n"

    # xref
    xref_off = len(body)
    entries = [(0, 65535, b"f")] + [(o, 0, b"n") for o in offsets]
    body += _render_classical_xref([(0, entries)])
    body += b"trailer\n<< /Size 5 /Root 1 0 R >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_indirect_length() -> bytes:
    """A stream whose /Length is an INDIRECT reference (`5 0 R`).

    Real PDFs routinely store a stream's /Length as a separate
    indirect object resolved through the xref table. A positional,
    single-object parse cannot resolve the reference, so it must
    locate the body end by scanning forward to ``endstream``. This
    fixture exercises that fallback: object 4 is a FlateDecode stream
    whose /Length points at object 5, which holds the byte count.
    """
    payload = b"Indirect-length stream body.\n"
    compressed = zlib.compress(payload, 6)  # deterministic at level 6

    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
    ]
    body = _header()
    offsets: list[int] = []
    for i, content in enumerate(objs, start=1):
        offsets.append(len(body))
        body += f"{i} 0 obj\n".encode("latin-1") + content + b"\nendobj\n"

    # Object 4: stream with an indirect /Length (-> object 5).
    offsets.append(len(body))
    dict_bytes = b"<< /Filter /FlateDecode /Length 5 0 R >>"
    body += b"4 0 obj\n" + dict_bytes + b"\nstream\n"
    body += compressed + b"\nendstream\nendobj\n"

    # Object 5: the length integer the stream's /Length resolves to.
    offsets.append(len(body))
    body += f"5 0 obj\n{len(compressed)}\nendobj\n".encode("latin-1")

    xref_off = len(body)
    entries = [(0, 65535, b"f")] + [(o, 0, b"n") for o in offsets]
    body += _render_classical_xref([(0, entries)])
    body += b"trailer\n<< /Size 6 /Root 1 0 R >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_stream_nul_body() -> bytes:
    """A stream whose raw body BEGINS with NUL bytes.

    PDF classifies NUL (0x00) as whitespace (§7.2.3). A parser that
    skips a greedy whitespace run after the ``stream`` keyword — the
    EOL plus the leading NUL body bytes — mis-locates the body start
    and then fails to find ``endstream``. The body here opens with
    ``00 00 00 0C`` (the first bytes of a JPEG-2000 box, the
    real-world trigger). Direct /Length, no filter, so the canonical
    form for the object is just ``stream << /Length N >>``.
    """
    raw_body = bytes([0x00, 0x00, 0x00, 0x0C]) + b"jP\x20\x20body"

    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
    ]
    body = _header()
    offsets: list[int] = []
    for i, content in enumerate(objs, start=1):
        offsets.append(len(body))
        body += f"{i} 0 obj\n".encode("latin-1") + content + b"\nendobj\n"

    # Object 4: unfiltered stream with a NUL-leading body.
    offsets.append(len(body))
    dict_bytes = f"<< /Length {len(raw_body)} >>".encode("latin-1")
    body += b"4 0 obj\n" + dict_bytes + b"\nstream\n"
    body += raw_body + b"\nendstream\nendobj\n"

    xref_off = len(body)
    entries = [(0, 65535, b"f")] + [(o, 0, b"n") for o in offsets]
    body += _render_classical_xref([(0, entries)])
    body += b"trailer\n<< /Size 5 /Root 1 0 R >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_objstm() -> bytes:
    """Objects packed inside an /Type /ObjStm container (§7.5.7).

    Objects 1 (Catalog) and 2 (Pages) live compressed inside the
    ObjStm (object 4); object 3 (Page) is uncompressed. A PDF 1.5
    /XRef stream (object 5) records 1,2 as type-2 (compressed,
    stream 4, indices 0,1) and 3,4,5 as type-1. The objects
    primitive must decode the container and extract the packed
    objects so they appear in the dump alongside the type-1 objects.
    """
    obj1 = b"<< /Type /Catalog /Pages 2 0 R >>"
    obj2 = b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>"
    payload = obj1 + b" " + obj2
    off2 = payload.index(obj2)
    header = f"1 0 2 {off2} ".encode("latin-1")  # (objnum offset) pairs
    objstm_raw = header + payload
    objstm_comp = zlib.compress(objstm_raw)

    obj3 = (b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
            b" /Resources << >> >>")

    body = _header(b"1.5")

    # Object 4: the ObjStm container.
    obj4_off = len(body)
    body += (f"4 0 obj\n<< /Type /ObjStm /N 2 /First {len(header)} "
             f"/Length {len(objstm_comp)} /Filter /FlateDecode >>\n"
             ).encode("latin-1")
    body += b"stream\n" + objstm_comp + b"\nendstream\nendobj\n"

    # Object 3: uncompressed Page.
    obj3_off = len(body)
    body += b"3 0 obj\n" + obj3 + b"\nendobj\n"

    # Object 5: the /XRef stream (W = [1 3 2]).
    obj5_off = len(body)
    entries = [
        (0, 0, 0xFFFF),     # obj 0: free
        (2, 4, 0),          # obj 1: compressed in stream 4, index 0
        (2, 4, 1),          # obj 2: compressed in stream 4, index 1
        (1, obj3_off, 0),   # obj 3: uncompressed
        (1, obj4_off, 0),   # obj 4: the ObjStm (uncompressed)
        (1, obj5_off, 0),   # obj 5: the xref stream (uncompressed, self)
    ]
    raw = bytearray()
    for etype, f2, f3 in entries:
        raw += bytes([etype]) + struct.pack(">I", f2)[1:] \
            + struct.pack(">H", f3)
    comp = zlib.compress(bytes(raw))
    stream_dict = (
        f"<< /Type /XRef /Size 6 /Root 1 0 R /W [1 3 2] "
        f"/Length {len(comp)} /Filter /FlateDecode >>"
    ).encode("latin-1")
    body += b"5 0 obj\n" + stream_dict + b"\nstream\n"
    body += comp + b"\nendstream\nendobj\n"
    body += f"startxref\n{obj5_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_edge_escapes() -> bytes:
    """Strings + names with the full range of escape mechanisms.

    Every byte value that ``(...)`` supports escaping for is
    exercised at least once — `\\n`, `\\(`, `\\\\`, octal. Names
    include `#XX` hex escapes for delimiters. Canonical form
    renders ALL strings as uppercase hex, so the source form is
    irrelevant at gate time; this fixture's job is to verify the
    decoding matches between oracle adapters + our parser.
    """
    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        # 4: string with backslash-n / backslash-paren / backslash-backslash
        b"(A\\nB\\(C\\\\D)",
        # 5: octal escape — \101 is 'A', \040 is space
        b"(\\101B\\040C)",
        # 6: name with hex escapes for space and slash
        b"/A#20B#2FC",
        # 7: hex string with mixed case + embedded whitespace (should
        #    normalise to uppercase, whitespace dropped)
        b"<48 65 6c 6C 6F>",
    ]
    return _assemble(objs)


def build_number_edges() -> bytes:
    """Numeric corner cases qpdf + mutool both accept.

    Reals: leading `.5`, trailing `5.`, negative with fraction.
    Integers: signed, zero, negative zero (canonicalises to 0).
    We avoid scientific notation (not valid PDF) and ridiculously
    large values (PDF real precision is implementation-defined
    and oracles may disagree on rendering).
    """
    objs: list[bytes] = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100]"
        b" /Resources << >> >>",
        # NOTE: leading-dot reals (``.5``) are valid per §7.3.3 but
        # the foundation::Lexer v1 does not dispatch them as
        # Number — it only enters ScanNumber when the first byte is
        # `+`, `-`, or a digit. A bare `.` falls through to the
        # Keyword branch, so `.5` arrives as a Keyword token rather
        # than Number. Excluded from this corpus until the lexer is
        # bumped to v2 with leading-dot dispatch. See memory note.
        # NOTE: trailing-dot reals (``5.``) are valid per §7.3.3 but
        # qpdf preserves the real type (emits ``5.0``) while mutool
        # coerces to integer (emits ``5``). The two oracles disagree
        # on type-vs-value at re-emit so this case is excluded from
        # the two_oracle_agreement corpus.
        # 4: negative with fraction
        b"-0.001",
        # 7: explicit-plus integer (canonical strips the +)
        b"+7",
        # 8: zero
        b"0",
        # 9: negative zero (canonicalises to 0)
        b"-0",
    ]
    return _assemble(objs)


# ---------------------------------------------------------------- oracle glue


def _oracle(script: str, pdf_path: Path) -> str:
    """Run a sibling oracle script on ``pdf_path`` and return its
    stdout. Output is textual canonical form with trailing newline."""
    r = subprocess.run(
        [sys.executable, str(HERE / script), str(pdf_path)],
        capture_output=True, text=True, timeout=30, check=False)
    if r.returncode != 0:
        raise RuntimeError(
            f"{script} failed on {pdf_path.name}: {r.stderr.strip()}")
    return r.stdout


def _check_agreement(pdf_path: Path) -> str:
    """Run both oracles and assert byte-identical canonical output.

    On disagreement raises with a first-diverging-line diff so the
    author can decide whether to adjust the fixture, tighten an
    oracle adapter, or widen the canonical grammar.
    """
    q = _oracle("oracle_qpdf.py", pdf_path)
    m = _oracle("oracle_mutool.py", pdf_path)
    if q == m:
        return q
    ql = q.splitlines()
    ml = m.splitlines()
    diff_at = 0
    while diff_at < min(len(ql), len(ml)) and ql[diff_at] == ml[diff_at]:
        diff_at += 1
    q_line = ql[diff_at] if diff_at < len(ql) else "<end>"
    m_line = ml[diff_at] if diff_at < len(ml) else "<end>"
    raise RuntimeError(
        f"oracle disagreement on {pdf_path.name} at line {diff_at + 1}:\n"
        f"  qpdf:   {q_line!r}\n"
        f"  mutool: {m_line!r}\n"
        f"Full qpdf output:\n{q}\n"
        f"Full mutool output:\n{m}")


# ---------------------------------------------------------------- main


FIXTURES = {
    "primitives.pdf": build_primitives,
    "nested.pdf": build_nested,
    "stream.pdf": build_stream,
    "indirect_length.pdf": build_indirect_length,
    "stream_nul_body.pdf": build_stream_nul_body,
    "objstm.pdf": build_objstm,
    "edge_escapes.pdf": build_edge_escapes,
    "number_edges.pdf": build_number_edges,
}


def main() -> int:
    """Build every fixture, verify oracle agreement, write the pair."""
    for name, builder in FIXTURES.items():
        pdf_path = HERE / name
        pdf_path.write_bytes(builder())
        canonical = _check_agreement(pdf_path)
        canonical_path = pdf_path.with_suffix(".canonical")
        canonical_path.write_text(canonical)
        print(f"  {name:24s}  {canonical_path.name} "
              f"({len(canonical.splitlines())} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
