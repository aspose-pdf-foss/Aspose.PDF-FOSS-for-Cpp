#!/usr/bin/env python3
"""qpdf objects adapter — run ``qpdf --show-object`` for every in-use
object in a fixture and normalise the combined output into the
canonical form used by the objects-primitive freeze gate.

qpdf's per-object format on v12.3 (observed):

    non-stream:
        << /Pages 2 0 R /Type /Catalog >>

    stream:
        Object is stream.  Dictionary:
        << /Length 30 /Filter /FlateDecode /Type /XRef >>

We strip the stream preamble and canonicalise the dictionary via
ref_parser, tagged as a stream.

In-use object IDs are discovered by reading ``qpdf --show-xref`` and
filtering to ``uncompressed`` entries — the objects primitive's v1
scope excludes ObjStm-compressed entries per objects.shared.yaml.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

# Importing the sibling module works when the script is invoked with
# Python's default cwd handling (``python3 oracle_qpdf.py <pdf>``),
# which is how generate_fixtures.py calls it.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from ref_parser import (
    canonical, parse_object_body, wrap_as_stream)


_XREF_LINE = re.compile(
    r"^(?P<id>\d+)/(?P<gen>\d+):\s+"
    r"(?P<kind>uncompressed|compressed)")


def _xref_inuse(pdf: Path) -> list[tuple[int, int]]:
    """Return (id, generation) for every in-use entry — both
    uncompressed (type-1) and compressed (type-2, packed in an
    /ObjStm). ``qpdf --show-object`` resolves either kind, so the
    objects gate dumps the extracted compressed objects too
    (§7.5.7); compressed objects always have generation 0."""
    r = subprocess.run(
        ["qpdf", "--show-xref", str(pdf)],
        capture_output=True, text=True, timeout=10, check=False)
    if r.returncode != 0:
        raise RuntimeError(f"qpdf --show-xref: {r.stderr.strip()}")
    out: list[tuple[int, int]] = []
    for raw in r.stdout.splitlines():
        m = _XREF_LINE.match(raw.strip())
        if not m:
            continue
        out.append((int(m.group("id")), int(m.group("gen"))))
    out.sort()
    return out


def _show_object(pdf: Path, oid: int, gen: int) -> bytes:
    """Run ``qpdf --show-object=ID,GEN`` and return its raw stdout
    as bytes. We use text=False because PDF strings may contain
    non-UTF-8 bytes; the ref parser operates on bytes."""
    r = subprocess.run(
        ["qpdf", f"--show-object={oid},{gen}", str(pdf)],
        capture_output=True, timeout=10, check=False)
    if r.returncode != 0:
        raise RuntimeError(
            f"qpdf --show-object={oid},{gen}: "
            f"{r.stderr.decode('utf-8', 'replace').strip()}")
    return r.stdout


_STREAM_PREAMBLE = b"Object is stream.  Dictionary:"


def _canonicalise_object(body: bytes) -> str:
    """Parse one qpdf per-object stdout and emit the canonical text
    for its value (NOT including the `<id> <gen>` prefix).

    Strips the stream preamble if present, tags the resulting dict
    as a stream.
    """
    # Strip trailing whitespace/newlines.
    body = body.rstrip()
    if body.startswith(_STREAM_PREAMBLE):
        # Drop preamble line, everything else is a dict.
        nl = body.find(b"\n")
        if nl < 0:
            raise ValueError("qpdf stream preamble has no following dict")
        dict_bytes = body[nl + 1:].strip()
        v = parse_object_body(dict_bytes)
        if v[0] != "dict":
            raise ValueError(
                f"qpdf stream dictionary did not parse as dict: {v[0]}")
        return canonical(wrap_as_stream(v))
    # Non-stream: the whole body is a direct-value expression.
    v = parse_object_body(body)
    return canonical(v)


def canonical_text(pdf: Path) -> str:
    """Produce the full canonical text for a fixture — one line per
    in-use object, sorted by (id, gen), trailing newline."""
    lines: list[str] = []
    for oid, gen in _xref_inuse(pdf):
        body = _show_object(pdf, oid, gen)
        lines.append(f"{oid} {gen} {_canonicalise_object(body)}")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write("usage: oracle_qpdf.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical_text(Path(sys.argv[1])))
