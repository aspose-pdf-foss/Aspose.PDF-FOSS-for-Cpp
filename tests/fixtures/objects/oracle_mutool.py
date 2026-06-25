#!/usr/bin/env python3
"""mutool objects adapter — run ``mutool show <pdf> <id>`` for every
in-use object in a fixture and normalise the combined output into
the canonical form used by the objects-primitive freeze gate.

mutool's per-object format on v1.27 (observed):

    non-stream:
        1 0 obj
        <<
          /Type /Catalog
          /Pages 2 0 R
        >>
        endobj

    stream:
        4 0 obj
        <<
          /Type /XRef
          ...
        >>
        stream
        <raw body bytes>
        endstream
        endobj

We strip the ``N G obj\\n`` wrapper (header line) and ``endobj``
footer. For streams we also drop the ``stream\\n<body>\\nendstream``
tail — body bytes are out of scope for the canonical form; their
length is captured by the header dict's /Length entry.

In-use object IDs are discovered by reusing qpdf's --show-xref
(same source as oracle_qpdf.py) — this guarantees both adapters
iterate the same ID set even when mutool internally groups free
entries differently.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ref_parser import (
    canonical, parse_object_body, wrap_as_stream)
from oracle_qpdf import _xref_inuse


def _show_object(pdf: Path, oid: int) -> bytes:
    """Run ``mutool show <pdf> <oid>`` and return its raw stdout.
    mutool does not expose a generation selector — it returns the
    current-generation object, which matches our fixtures (every
    object in our corpus has generation 0)."""
    r = subprocess.run(
        ["mutool", "show", str(pdf), str(oid)],
        capture_output=True, timeout=10, check=False)
    if r.returncode != 0:
        raise RuntimeError(
            f"mutool show {pdf.name} {oid}: "
            f"{r.stderr.decode('utf-8', 'replace').strip()}")
    return r.stdout


def _strip_wrapper(body: bytes, oid: int, gen: int) -> bytes:
    """Return the direct-value expression portion of mutool's output.

    Drops the ``N G obj\\n`` header, the trailing ``endobj`` line,
    and — for streams — the ``stream\\n<body>\\nendstream\\n`` tail
    between the dict and endobj.
    """
    # mutool always prefixes with "<id> <gen> obj\n"; strip it.
    header = f"{oid} {gen} obj\n".encode("ascii")
    if not body.startswith(header):
        raise ValueError(
            f"mutool output missing expected header {header!r}; "
            f"got {body[:32]!r}")
    body = body[len(header):]

    # For stream objects, body contains ...dict... stream\n<bytes>\nendstream\nendobj
    # Locate the first occurrence of b"\nstream\n" after the dict.
    stream_marker = b"\nstream\n"
    sm = body.find(stream_marker)
    if sm >= 0:
        # Non-destructive: split at the stream marker. Everything
        # before is the dict; throw away the rest.
        return body[:sm].rstrip() + b"\nSTREAM_MARKER\n"
    # Non-stream: drop the trailing endobj.
    ending = b"\nendobj\n"
    if body.endswith(ending):
        body = body[:-len(ending)]
    return body


def _canonicalise_object(body: bytes, oid: int, gen: int) -> str:
    """Parse mutool output and emit canonical text for the value."""
    stripped = _strip_wrapper(body, oid, gen)
    is_stream = stripped.endswith(b"\nSTREAM_MARKER\n")
    if is_stream:
        dict_bytes = stripped[:-len(b"\nSTREAM_MARKER\n")].rstrip()
        v = parse_object_body(dict_bytes)
        if v[0] != "dict":
            raise ValueError(
                f"mutool stream header did not parse as dict: {v[0]}")
        return canonical(wrap_as_stream(v))
    v = parse_object_body(stripped)
    return canonical(v)


def canonical_text(pdf: Path) -> str:
    """Produce the full canonical text for a fixture — one line per
    in-use object, sorted by (id, gen), trailing newline."""
    lines: list[str] = []
    for oid, gen in _xref_inuse(pdf):
        body = _show_object(pdf, oid)
        lines.append(f"{oid} {gen} {_canonicalise_object(body, oid, gen)}")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write("usage: oracle_mutool.py <fixture.pdf>\n")
        raise SystemExit(2)
    sys.stdout.write(canonical_text(Path(sys.argv[1])))
