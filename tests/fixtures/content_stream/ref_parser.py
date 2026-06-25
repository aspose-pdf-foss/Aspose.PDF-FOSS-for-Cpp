#!/usr/bin/env python3
"""Reference operand canonicaliser for content_stream fixtures.

This module is shared by the pypdf and pdfminer.six oracle
adapters. It is NOT an oracle itself — its job is to take the
normalised ``Value`` tuples each oracle produces and render them
into the freeze-gate canonical form.

Canonical form per content_stream.yaml:

    null                   -> "null"
    true / false           -> "true" / "false"
    int N                  -> decimal digits, signed (no leading '+')
    real X                 -> shortest round-trip decimal with '.'
                              (never scientific; integer-valued
                              reals emit as "N.0")
    name /foo              -> "/foo" with '#XX' escapes for bytes
                              outside 0x21..0x7E and the
                              delimiter set
    string <bytes>         -> "<" + uppercase-hex(bytes) + ">"
    array [ v1 … vn ]      -> "[ " + " ".join(canon(vi)) + " ]"
                              (empty -> "[ ]")
    dict << /K1 v1 … >>    -> "<< " + " ".join("/K "+canon(v)) + " >>"
                              (keys ASCII-sorted on decoded bytes;
                              empty -> "<< >>")
    ref N M                -> "N M R"

Each Value is a Python tuple whose first element is the kind
tag; using tuples instead of classes keeps the canonical()
dispatch a plain if-chain and the whole module short.
"""

from __future__ import annotations

from typing import Union


# Tagged-value shapes:
#   ("null",)
#   ("bool", bool)
#   ("int", int)
#   ("real", float, str)         # str = original decimal digits
#                                # used to preserve precision when
#                                # round-tripping the canonical form
#   ("name", bytes)              # decoded bytes (no leading '/')
#   ("string", bytes)            # decoded bytes (source kind dropped)
#   ("array", list[Value])
#   ("dict", list[tuple[bytes, Value]])   # (decoded_key, value)
#   ("ref", int, int)            # (id, gen)
Value = tuple


# Delimiter characters that trigger '#XX' escaping inside a name.
# Per PDF 32000 §7.3.5 — '(', ')', '<', '>', '[', ']', '{', '}',
# '/', '%' cannot appear literally inside a name. Space (0x20)
# and other whitespace are excluded too (outside 0x21..0x7E).
_NAME_DELIMITERS = set(b"()<>[]{}/%#")


def canonicalise(value: Value) -> str:
    """Render a tagged ``Value`` tuple to its canonical string form.
    Delegates to small per-kind helpers so each branch stays short."""
    kind = value[0]
    if kind == "null":
        return "null"
    if kind == "bool":
        return "true" if value[1] else "false"
    if kind == "int":
        return str(value[1])
    if kind == "real":
        # Keep the exact source-digit form when provided — preserves
        # any precision the PDF author chose (e.g. "0.50" vs "0.5"
        # wouldn't matter for the value but could matter for
        # round-trip equality between oracles).
        _, number, source = value
        return _canonical_real(number, source)
    if kind == "name":
        return _canonical_name(value[1])
    if kind == "string":
        return "<" + value[1].hex().upper() + ">"
    if kind == "array":
        items = value[1]
        if not items:
            return "[ ]"
        return "[ " + " ".join(canonicalise(v) for v in items) + " ]"
    if kind == "dict":
        entries = value[1]
        if not entries:
            return "<< >>"
        # ASCII-sorted on the DECODED key bytes — two oracles may
        # emit entries in different source orders and still agree
        # on the canonical form because we sort them here.
        sorted_entries = sorted(entries, key=lambda kv: kv[0])
        parts = []
        for k, v in sorted_entries:
            parts.append(_canonical_name(k) + " " + canonicalise(v))
        return "<< " + " ".join(parts) + " >>"
    if kind == "ref":
        return f"{value[1]} {value[2]} R"
    raise ValueError(f"unknown value kind: {kind!r}")


def canonicalise_operation(op: bytes, operands: list[Value]) -> str:
    """Render one Operation (op + operand list) to a single line
    without a trailing newline. Callers join with '\\n' and add
    a terminating '\\n' to match the driver's expected format.

    Zero operands -> "<op>"; non-zero -> "<o1> <o2> … <op>".
    Operator byte-string is emitted verbatim as ASCII — PDF
    operator tokens are restricted to printable ASCII by §7.8.
    """
    op_str = op.decode("ascii")
    if not operands:
        return op_str
    operand_strs = [canonicalise(v) for v in operands]
    return " ".join(operand_strs) + " " + op_str


def canonicalise_stream(operations: list[tuple[bytes, list[Value]]]
                        ) -> str:
    """Render a list of (op, operands) pairs to the full canonical
    freeze-gate text: one line per Operation, trailing newline
    after each line including the last. Empty list -> empty
    string (no trailing newline)."""
    if not operations:
        return ""
    lines = [canonicalise_operation(op, args) for op, args in operations]
    return "\n".join(lines) + "\n"


# -------------------------------------------------------------
# Helpers
# -------------------------------------------------------------


def _canonical_real(number: float, source: str) -> str:
    """Render a real number. Prefers the source-digit form if it
    parses back to the same float — preserves trailing zeros and
    author precision choices. Falls back to Python's repr() which
    produces the shortest round-trip decimal with '.' and never
    scientific notation for the magnitudes PDFs use in practice.

    Integer-valued reals (5.0, -3.0) must retain a '.0' suffix so
    they stay distinguishable from integers at the canonical level.
    """
    # Try the source form first.
    if source:
        try:
            if float(source) == number:
                # Ensure there's a '.' present (oracle might have
                # stripped it from the source form).
                if "." not in source and "e" not in source.lower():
                    return source + ".0"
                return source
        except ValueError:
            pass
    # Fallback: repr gives 'shortest round-trip' decimal.
    s = repr(number)
    if "e" in s.lower():
        # Shouldn't happen for PDF-range numbers; keep for safety.
        s = f"{number:.15f}".rstrip("0").rstrip(".")
    if "." not in s:
        s += ".0"
    return s


def _canonical_name(decoded: bytes) -> str:
    """Render a name's decoded bytes to "/<escaped>" form.

    Bytes outside the printable-ASCII range minus the delimiter
    set are emitted as '#XX' (two uppercase hex digits). This is
    the canonical form mandated by §7.3.5 — multiple source
    encodings (e.g. "/F1" vs "/F#31") decode to the same bytes
    and must normalise to the same output, which is the bare
    "/F1" since '1' is printable and not a delimiter.
    """
    out = ["/"]
    for byte in decoded:
        if 0x21 <= byte <= 0x7e and byte not in _NAME_DELIMITERS:
            out.append(chr(byte))
        else:
            out.append(f"#{byte:02X}")
    return "".join(out)
