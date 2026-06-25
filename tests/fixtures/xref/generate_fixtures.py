#!/usr/bin/env python3
"""Generate the PDF fixture corpus for the foundation/xref primitive.

The xref primitive's freeze gate is two_oracle_agreement — for each
fixture PDF, the C++ implementation must produce a textual cross-
reference dump that matches the cached ``<fixture>.canonical`` sidecar.
Each sidecar is produced by running qpdf and mutool on the fixture,
normalising their outputs, and asserting they agree — if they
disagree, the fixture is inherently ambiguous and is removed from the
corpus before freeze.

Canonical line format (sorted by object id ascending):

    <id> <gen> inuse <offset>              # uncompressed in-use
    <id> <gen> compressed <stream_id> <index>  # object-stream member

Free entries are intentionally omitted — qpdf omits them, so does
our canonical form. Object 0 is the free-list head and is
uninteresting for any real reader.

Fixtures:

  * ``minimal.pdf`` — trivial 3-object PDF with a single-subsection
    xref table. Establishes the floor.
  * ``two_subsections.pdf`` — xref table with two subsections
    separated by a gap in object ids (``0 1`` + ``3 3``). Exposes
    lexers that assume a single contiguous subsection.
  * ``xref_stream.pdf`` — PDF 1.5 /XRef stream replacing the
    classical table. Entries uncompressed only (type 1), W=[1 3 1].
  * ``objstm.pdf`` — PDF 1.5 /XRef stream with mixed type-1
    (uncompressed) + type-2 (compressed-into-/ObjStm) entries. The
    two compressed objects live inside an ObjStm container.
  * ``incremental.pdf`` — two revisions: the first revision uses a
    classical xref table; the second appends a second xref table
    (with /Prev pointing back) that updates object 3 and adds
    object 4. Tests that the reader follows /Prev and merges.

Run: ``python3 generate_fixtures.py``.
The script aborts if any oracle disagreement is detected.
"""

from __future__ import annotations

import struct
import subprocess
import sys
import zlib
from pathlib import Path

# Resolve workspace-relative paths against this file's own directory —
# the script is self-contained and can be run from anywhere.
HERE = Path(__file__).resolve().parent


# ---------------------------------------------------------------- builders


def _header(version: bytes = b"1.4") -> bytearray:
    """Common PDF header + binary-safe marker comment. The four
    high-bit bytes in the magic comment signal binary content to
    any text-mode transfer so line-ending rewrites can't mangle
    the file. PDF 32000 §7.5.2."""
    out = bytearray()
    out += b"%PDF-" + version + b"\n"
    out += b"%\xe2\xe3\xcf\xd3\n"
    return out


def _classical_xref(offsets: list[tuple[int, int, int]], size: int) -> bytes:
    """Render a classical xref table from subsection descriptors.

    Each subsection is ``(first_id, count, start_offset_index)``,
    where ``start_offset_index`` indexes into a flat offsets list
    in the caller's layout — too clever; instead callers pass the
    full offset list directly via the subsection-of-offsets API
    below. We keep this helper as the low-level renderer.
    """
    # Unused in this module after the refactor; kept as a spec
    # anchor for what an xref table's bytes look like.
    raise NotImplementedError("use _render_classical_xref instead")


def _render_classical_xref(
        subsections: list[tuple[int, list[tuple[int, int, bytes]]]]) -> bytes:
    """Render a classical xref table.

    ``subsections`` is a list of ``(first_id, entries)`` pairs, each
    entry being ``(offset, generation, status)`` where ``status``
    is ``b"n"`` (in-use) or ``b"f"`` (free). Entries are emitted
    verbatim using the 20-byte fixed-width format per §7.5.4.
    """
    out = bytearray()
    out += b"xref\n"
    for first_id, entries in subsections:
        out += f"{first_id} {len(entries)}\n".encode("latin-1")
        for offset, gen, status in entries:
            # 20 bytes total: 10-digit offset + space + 5-digit gen +
            # space + status + space + \n. The trailing "\n" is one
            # of the two legal EOL forms per §7.5.4; the other is
            # " \r" or " \n" preceded by a space — we go with "\n"
            # because both oracles accept it.
            out += f"{offset:010d} {gen:05d} ".encode("latin-1")
            out += status + b" \n"
    return bytes(out)


def build_minimal() -> bytes:
    """Baseline 3-object PDF with a single-subsection xref table.

    Offsets below are computed by tracking cumulative length as
    objects are appended; keeping the computation explicit here
    (rather than walking a tree) makes the fixture bytes easy to
    reason about.
    """
    body = _header()

    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"

    obj2_off = len(body)
    body += b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"

    obj3_off = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")

    xref_off = len(body)
    body += _render_classical_xref([
        (0, [
            (0, 65535, b"f"),
            (obj1_off, 0, b"n"),
            (obj2_off, 0, b"n"),
            (obj3_off, 0, b"n"),
        ]),
    ])

    body += b"trailer\n<< /Size 4 /Root 1 0 R >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def build_two_subsections() -> bytes:
    """xref table split into two subsections.

    We skip object id 4 intentionally: the xref table declares
    ``0 1`` (object 0 only) followed by ``1 3`` (objects 1..3),
    but also we want the gap to be within the ID range. To make
    that work with /Size=5 and object 4 omitted entirely, we'd
    need /Size=5 but only emit objects 0,1,2,3 — that would mean
    object 4 is implicitly a free slot. Instead, we use the form
    that actually stresses subsection parsing: ``0 2`` + ``3 2``,
    skipping object ids 2 and declaring two distinct ranges.

    Concretely: object 0 is the free-list head, object 1 is the
    Catalog. There is NO object 2. Objects 3 and 4 are Pages and
    Page. /Size is 5. A reader that fuses the two subsections into
    one would either crash reading a missing offset for id 2 or
    report 5 entries (incorrect — id 2 was never declared).
    """
    body = _header()

    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 3 0 R >>\nendobj\n"

    obj3_off = len(body)
    body += b"3 0 obj\n<< /Type /Pages /Kids [4 0 R] /Count 1 >>\nendobj\n"

    obj4_off = len(body)
    body += (b"4 0 obj\n<< /Type /Page /Parent 3 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")

    xref_off = len(body)
    body += _render_classical_xref([
        (0, [
            (0, 65535, b"f"),
            (obj1_off, 0, b"n"),
        ]),
        (3, [
            (obj3_off, 0, b"n"),
            (obj4_off, 0, b"n"),
        ]),
    ])

    body += b"trailer\n<< /Size 5 /Root 1 0 R >>\n"
    body += f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return bytes(body)


def _render_xref_stream(entries: list[tuple[int, int, int]],
                        size: int,
                        root_ref: bytes,
                        stream_obj_id: int,
                        body: bytearray,
                        predictor: int = 0,
                        index: "list[tuple[int, int]] | None" = None) -> bytes:
    """Append a PDF 1.5 /XRef stream object to ``body`` in place and
    return the bytes to append after it (startxref + %%EOF).

    ``entries`` is a list of ``(type, field2, field3)`` per the
    /XRef stream encoding: type 0 = free (field2=next_free_id,
    field3=gen), type 1 = uncompressed (field2=offset, field3=gen),
    type 2 = compressed (field2=stream_id, field3=index).

    W is fixed to ``[1 3 2]`` — 1 byte type, 3 bytes field2
    (supports offsets up to 16 MiB — plenty for fixtures), 2 bytes
    field3 (supports generations up to 65535 and stream indices
    up to 65535 — plenty for fixtures).
    """
    xref_off = len(body)

    # Pack each entry as 1 + 3 + 2 = 6 bytes, big-endian. This is
    # the straightforward PDF 1.5 /XRef stream encoding.
    rowlen = 6  # W = [1 3 2]
    raw = bytearray()
    for etype, f2, f3 in entries:
        raw += bytes([etype])
        raw += struct.pack(">I", f2)[1:]   # 3 bytes of a 4-byte big-endian int
        raw += struct.pack(">H", f3)        # 2 bytes big-endian

    stream_bytes = bytes(raw)
    extra = ""
    if predictor >= 10:
        # PNG-predict each row (filter tag 2 = "Up", deltas vs the
        # previous row) so a reader MUST reverse the predictor after
        # FlateDecode. /Columns == the W row width; 1 byte/pixel.
        prev = bytes(rowlen)
        enc = bytearray()
        for r in range(0, len(stream_bytes), rowlen):
            row = stream_bytes[r:r + rowlen]
            enc.append(2)  # PNG Up filter tag
            enc += bytes((row[i] - prev[i]) & 0xFF for i in range(rowlen))
            prev = row
        stream_bytes = bytes(enc)
        extra += (f" /DecodeParms << /Predictor {predictor} "
                  f"/Columns {rowlen} >>")

    index_str = ""
    if index is not None:
        flat = " ".join(f"{s} {c}" for s, c in index)
        index_str = f" /Index [ {flat} ]"

    compressed = zlib.compress(stream_bytes)
    stream_dict = (
        f"<< /Type /XRef /Size {size} /Root {root_ref.decode('latin-1')} "
        f"/W [1 3 2]{index_str}{extra} /Length {len(compressed)} "
        f"/Filter /FlateDecode >>"
    ).encode("latin-1")
    body += f"{stream_obj_id} 0 obj\n".encode("latin-1")
    body += stream_dict + b"\nstream\n"
    body += compressed + b"\nendstream\nendobj\n"

    tail = f"startxref\n{xref_off}\n%%EOF\n".encode("latin-1")
    return tail


def build_xref_stream() -> bytes:
    """PDF 1.5 with an /XRef stream (no classical table).

    The xref stream is itself an object (id = 4) and appears at
    the tail of the file. Offsets for objects 1..3 are tracked as
    the body is built; offset for object 4 is the xref stream
    object's own starting offset.
    """
    body = _header(b"1.5")

    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"

    obj2_off = len(body)
    body += b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"

    obj3_off = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")

    # Object 4 is the xref stream — its offset is where we're about
    # to write. The stream's own entry points at that offset.
    obj4_off = len(body)
    entries = [
        (0, 0, 0xFFFF),       # obj 0: free, next=0, gen=FFFF
        (1, obj1_off, 0),     # obj 1: inuse
        (1, obj2_off, 0),     # obj 2: inuse
        (1, obj3_off, 0),     # obj 3: inuse
        (1, obj4_off, 0),     # obj 4: inuse (self)
    ]
    tail = _render_xref_stream(
        entries=entries, size=5, root_ref=b"1 0 R",
        stream_obj_id=4, body=body)
    body += tail
    return bytes(body)


def build_xref_stream_predictor() -> bytes:
    """PDF 1.5 /XRef stream whose binary table is PNG-predicted
    (/DecodeParms << /Predictor 12 /Columns 6 >>). A reader that
    FlateDecodes but skips the per-row predictor reversal reads
    garbage offsets — this fixture pins that reversal."""
    body = _header(b"1.5")
    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
    obj2_off = len(body)
    body += b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
    obj3_off = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")
    obj4_off = len(body)
    entries = [
        (0, 0, 0xFFFF),
        (1, obj1_off, 0),
        (1, obj2_off, 0),
        (1, obj3_off, 0),
        (1, obj4_off, 0),
    ]
    tail = _render_xref_stream(
        entries=entries, size=5, root_ref=b"1 0 R",
        stream_obj_id=4, body=body, predictor=12)
    body += tail
    return bytes(body)


def build_xref_stream_index() -> bytes:
    """PDF 1.5 /XRef stream with a NON-contiguous /Index. The xref
    stream itself is object 10, so /Index has two subsections —
    ``[0 4]`` (objs 0-3) and ``[10 1]`` (the stream). A reader that
    ignores /Index and numbers rows 0..n-1 maps the stream entry to
    object 4 instead of 10, so this fixture pins /Index handling."""
    body = _header(b"1.5")
    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
    obj2_off = len(body)
    body += b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
    obj3_off = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")
    obj10_off = len(body)
    entries = [
        (0, 0, 0xFFFF),       # subsection [0 4]: obj 0 free
        (1, obj1_off, 0),     #                   obj 1
        (1, obj2_off, 0),     #                   obj 2
        (1, obj3_off, 0),     #                   obj 3
        (1, obj10_off, 0),    # subsection [10 1]: obj 10 (the xref stream)
    ]
    tail = _render_xref_stream(
        entries=entries, size=11, root_ref=b"1 0 R",
        stream_obj_id=10, body=body, index=[(0, 4), (10, 1)])
    body += tail
    return bytes(body)


def build_objstm() -> bytes:
    """PDF 1.5 /XRef stream with mixed uncompressed + compressed
    entries.

    Objects 1 and 2 (Catalog, Pages) are packed inside an /ObjStm
    container (object 4). Object 3 (Page) is stored uncompressed
    for contrast. The xref stream (object 5) records objects 1,2
    as type-2 (compressed) pointing at stream id 4, and objects
    3,4,5 as type-1 (uncompressed).
    """
    body = _header(b"1.5")

    # Build the ObjStm body first (we need its compressed length
    # to compute its /Length entry).
    obj1_text = b"<< /Type /Catalog /Pages 2 0 R >>"
    obj2_text = b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>"
    # ObjStm "first" header: pairs of <object_id> <offset-within-payload>.
    # Offsets are within the PAYLOAD (after the header), per §7.5.7.
    # We pack both objects with a single space separator.
    payload = obj1_text + b" " + obj2_text
    off_obj1 = 0
    off_obj2 = payload.index(obj2_text)
    header_text = f"1 {off_obj1} 2 {off_obj2} ".encode("latin-1")
    objstm_uncompressed = header_text + payload
    objstm_compressed = zlib.compress(objstm_uncompressed)

    # Object 3 is an uncompressed Page object living outside the ObjStm.
    obj3_text = (b"<< /Type /Page /Parent 2 0 R"
                 b" /MediaBox [0 0 100 100] /Resources << >> >>")

    # Emit object 4 (the ObjStm).
    obj4_off = len(body)
    body += f"4 0 obj\n".encode("latin-1")
    body += (
        f"<< /Type /ObjStm /N 2 /First {len(header_text)} "
        f"/Length {len(objstm_compressed)} /Filter /FlateDecode >>\n"
    ).encode("latin-1")
    body += b"stream\n" + objstm_compressed + b"\nendstream\nendobj\n"

    # Emit object 3 (uncompressed Page). By convention in PDF 1.5
    # compressed-object layout, uncompressed objects can appear
    # either before or after the ObjStm; here we put it after so
    # both ordering cases are exercised across the fixture set.
    obj3_off = len(body)
    body += b"3 0 obj\n" + obj3_text + b"\nendobj\n"

    # Emit the xref stream (object 5).
    obj5_off = len(body)
    entries = [
        (0, 0, 0xFFFF),         # obj 0: free
        (2, 4, 0),              # obj 1: compressed, stream=4, idx=0
        (2, 4, 1),              # obj 2: compressed, stream=4, idx=1
        (1, obj3_off, 0),       # obj 3: uncompressed
        (1, obj4_off, 0),       # obj 4: uncompressed (the ObjStm itself)
        (1, obj5_off, 0),       # obj 5: uncompressed (the xref stream itself)
    ]
    tail = _render_xref_stream(
        entries=entries, size=6, root_ref=b"1 0 R",
        stream_obj_id=5, body=body)
    body += tail
    return bytes(body)


def build_incremental() -> bytes:
    """Classical xref + incremental update with /Prev chain.

    Revision 1 contains objects 1..3 with a single xref table.
    Revision 2 appends an updated object 3 (new /MediaBox) and a
    new object 4, plus a second xref table whose trailer carries
    /Prev = <first_xref_offset>. A correct reader walks /Prev and
    merges: object 3 resolves to the revision-2 copy, objects 1,2
    are inherited from revision 1, object 4 is introduced by
    revision 2.
    """
    body = _header()

    # Revision 1 — initial document.
    obj1_off = len(body)
    body += b"1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"

    obj2_off = len(body)
    body += b"2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"

    obj3_off_v1 = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 100 100] /Resources << >> >>\nendobj\n")

    xref1_off = len(body)
    body += _render_classical_xref([
        (0, [
            (0, 65535, b"f"),
            (obj1_off, 0, b"n"),
            (obj2_off, 0, b"n"),
            (obj3_off_v1, 0, b"n"),
        ]),
    ])
    body += b"trailer\n<< /Size 4 /Root 1 0 R >>\n"
    body += f"startxref\n{xref1_off}\n%%EOF\n".encode("latin-1")

    # Revision 2 — updated object 3 (different MediaBox) + new object 4.
    # Object 3's generation stays at 0 since we're not deleting it —
    # it's a structural update, not a deletion-and-replacement.
    obj3_off_v2 = len(body)
    body += (b"3 0 obj\n<< /Type /Page /Parent 2 0 R"
             b" /MediaBox [0 0 200 200] /Resources << >> >>\nendobj\n")

    obj4_off = len(body)
    body += b"4 0 obj\n<< /Info (appended in revision 2) >>\nendobj\n"

    # Revision 2's xref has only the changed/new entries. Object 0
    # still needs to be present (free list head) per §7.5.4.
    xref2_off = len(body)
    body += _render_classical_xref([
        (0, [
            (0, 65535, b"f"),
        ]),
        (3, [
            (obj3_off_v2, 0, b"n"),
            (obj4_off, 0, b"n"),
        ]),
    ])
    body += (
        f"trailer\n<< /Size 5 /Root 1 0 R /Prev {xref1_off} >>\n"
    ).encode("latin-1")
    body += f"startxref\n{xref2_off}\n%%EOF\n".encode("latin-1")

    return bytes(body)


# ---------------------------------------------------------------- oracle glue


def _oracle(script: str, pdf_path: Path) -> str:
    """Run a sibling oracle script on ``pdf_path`` and return its
    stdout. Sibling scripts are expected to print the canonical
    textual form to stdout on exit code 0, and raise otherwise."""
    r = subprocess.run(
        [sys.executable, str(HERE / script), str(pdf_path)],
        capture_output=True, text=True, timeout=15, check=False,
    )
    if r.returncode != 0:
        raise RuntimeError(
            f"{script} failed on {pdf_path.name}: {r.stderr.strip()}")
    return r.stdout


def _check_agreement(pdf_path: Path) -> str:
    """Run both oracles and assert byte-identical canonical output.

    Returns the agreed-upon canonical text, or raises RuntimeError
    naming the fixture and showing the first diverging line so the
    author can decide whether to fix the fixture or tighten one
    oracle adapter.
    """
    q = _oracle("oracle_qpdf.py", pdf_path)
    m = _oracle("oracle_mutool.py", pdf_path)
    if q == m:
        return q
    # Diagnostic — surface the first divergence.
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
    "minimal.pdf": build_minimal,
    "two_subsections.pdf": build_two_subsections,
    "xref_stream.pdf": build_xref_stream,
    "xref_stream_predictor.pdf": build_xref_stream_predictor,
    "xref_stream_index.pdf": build_xref_stream_index,
    "objstm.pdf": build_objstm,
    "incremental.pdf": build_incremental,
}


def main() -> int:
    """Build every fixture, verify oracle agreement, write the pair
    (``.pdf`` + ``.canonical``). Exits nonzero on first disagreement."""
    for name, builder in FIXTURES.items():
        pdf_path = HERE / name
        pdf_path.write_bytes(builder())
        canonical = _check_agreement(pdf_path)
        canonical_path = pdf_path.with_suffix(".canonical")
        canonical_path.write_text(canonical)
        print(f"  {name:28s}  {canonical_path.name} "
              f"({len(canonical.splitlines())} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
