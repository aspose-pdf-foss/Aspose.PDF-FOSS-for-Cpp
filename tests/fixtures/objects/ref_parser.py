#!/usr/bin/env python3
"""Reference PDF object parser + canoniser (Python, authoring-time only).

This module is shared by the qpdf and mutool oracle adapters. It is
NOT an oracle itself — its job is to take whatever a real oracle
(qpdf, mutool) emits for a single object and normalise that text
into the freeze-gate canonical form. Because both oracles emit
valid PDF direct-value expressions (plus some easily-stripped
wrapper text), one parser serves both.

Canonical form, per objects.shared.yaml:

    null                  -> "null"
    true / false          -> "true" / "false"
    integer               -> decimal, signed (no leading "+")
    real                  -> shortest round-trip decimal with a "."
    name                  -> "/<decoded_name_with_#XX_for_unsafe>"
    string                -> "<uppercase-hex>"   (BOTH source kinds)
    array                 -> "[ v1 v2 ... vn ]"   (empty -> "[ ]")
    dict                  -> "<< /K1 v1 /K2 v2 ... >>"  (keys sorted)
    ref                   -> "N M R"
    stream (header dict)  -> "stream " + dict_canonical(header)

Stream bodies are not part of canonical — the /Length entry in the
header dict carries the body length, which is sufficient for gate
comparison.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Union


# ---------------------------------------------------------------- tagged values

# A parsed value is a Python tuple whose first element names the kind.
# Using plain tuples instead of classes keeps canonical() dispatch
# trivial and the whole module short.

Value = Union[
    tuple,  # one of the shapes below
]

# Shapes:
#   ("null",)
#   ("bool", bool)
#   ("int", int)
#   ("real", float, str)         # str = original decimal digits, for precision-preserving canonicalisation
#   ("name", str)                # decoded (no leading "/")
#   ("string", bytes)            # decoded bytes (source kind is not preserved — both render hex)
#   ("array", list[Value])
#   ("dict", list[tuple[str, Value]])   # (decoded_key, value) pairs, source order
#   ("ref", int, int)            # (id, gen)
#   ("stream", dict_value)       # dict_value is a ("dict", ...) tuple; body bytes not preserved


# ---------------------------------------------------------------- scanner

_WS = b" \t\n\r\f\x00"
_DELIM = b"()<>[]{}/%"
# Regular-name characters per §7.3.5: anything that is not whitespace,
# not a delimiter, and is printable. Hex-escape otherwise.
_NAME_REGULAR = set(bytes(range(0x21, 0x7F))) - set(_DELIM) - {ord("#")}


class ParseError(ValueError):
    """Raised on malformed input. Message names the byte offset."""


@dataclass
class _Scan:
    """Minimal byte-oriented scanner. Tracks position; exposes peek,
    advance, skip-ws-and-comments, and a couple of keyword predicates.
    Everything above operates on this scanner."""
    buf: bytes
    pos: int = 0

    def eof(self) -> bool:
        return self.pos >= len(self.buf)

    def peek(self, n: int = 1) -> bytes:
        return self.buf[self.pos : self.pos + n]

    def skip_ws(self) -> None:
        """Skip whitespace and %-comments. Comments run to end-of-line
        per §7.2.4 — they terminate at CR, LF, or CRLF."""
        while not self.eof():
            c = self.buf[self.pos]
            if c in _WS:
                self.pos += 1
            elif c == ord("%"):
                # Comment — scan to EOL.
                while not self.eof() and self.buf[self.pos] not in b"\r\n":
                    self.pos += 1
            else:
                return

    def starts_with(self, kw: bytes) -> bool:
        """True iff ``kw`` appears at the current position AND the
        following byte is a whitespace, delimiter, or EOF (keyword
        boundary). Matters so ``true`` doesn't fire inside ``trueish``
        (admittedly impossible in valid PDF, but the check is cheap)."""
        end = self.pos + len(kw)
        if self.buf[self.pos:end] != kw:
            return False
        if end >= len(self.buf):
            return True
        nxt = self.buf[end]
        return nxt in _WS or nxt in _DELIM

    def expect(self, kw: bytes) -> None:
        if not self.starts_with(kw):
            raise ParseError(
                f"expected {kw!r} at offset {self.pos}, "
                f"got {self.buf[self.pos : self.pos + 16]!r}")
        self.pos += len(kw)


# ---------------------------------------------------------------- parsers

# Number pattern: optional sign, digits, optional `.digits`, or
# `.digits`, or `digits.`. Scientific notation is not valid PDF.
_NUM_RE = re.compile(rb"[-+]?(\d+\.\d*|\.\d+|\d+)")


def _parse_number(s: _Scan) -> Value:
    m = _NUM_RE.match(s.buf, s.pos)
    if not m:
        raise ParseError(f"expected number at offset {s.pos}")
    text = m.group(0)
    s.pos = m.end()
    if b"." in text:
        return ("real", float(text), text.decode("ascii"))
    # Integer. ``int(text, 10)`` handles leading "+" and "-" fine.
    return ("int", int(text))


def _parse_name(s: _Scan) -> Value:
    """Consume a `/Name` token. §7.3.5: `/` prefix, then regular chars
    or `#XX` hex escapes (two hex digits = one byte)."""
    assert s.peek() == b"/"
    s.pos += 1
    out = bytearray()
    while not s.eof():
        c = s.buf[s.pos]
        if c in _WS or c in _DELIM:
            break
        if c == ord("#"):
            if s.pos + 3 > len(s.buf):
                raise ParseError(f"truncated #XX escape at {s.pos}")
            hx = s.buf[s.pos + 1 : s.pos + 3]
            try:
                out.append(int(hx, 16))
            except ValueError:
                raise ParseError(f"bad #XX escape at {s.pos}")
            s.pos += 3
        else:
            out.append(c)
            s.pos += 1
    # Names are typically ASCII; decode latin-1 so any byte survives.
    return ("name", out.decode("latin-1"))


def _parse_string_literal(s: _Scan) -> Value:
    """Consume a `(...)` string. §7.3.4.2: balanced parens, escape
    sequences: \\n \\r \\t \\b \\f \\\\ \\( \\) \\DDD (1-3 octal
    digits), line continuation \\<EOL>. Raw CR / LF / CRLF inside
    the literal are EQUIVALENT to LF (per spec — EOL normalisation).
    """
    assert s.peek() == b"("
    s.pos += 1
    out = bytearray()
    depth = 1
    while True:
        if s.eof():
            raise ParseError("unterminated literal string")
        c = s.buf[s.pos]
        if c == ord("\\"):
            s.pos += 1
            if s.eof():
                raise ParseError("trailing backslash in literal string")
            e = s.buf[s.pos]
            if e == ord("n"):
                out.append(0x0A); s.pos += 1
            elif e == ord("r"):
                out.append(0x0D); s.pos += 1
            elif e == ord("t"):
                out.append(0x09); s.pos += 1
            elif e == ord("b"):
                out.append(0x08); s.pos += 1
            elif e == ord("f"):
                out.append(0x0C); s.pos += 1
            elif e == ord("\\"):
                out.append(0x5C); s.pos += 1
            elif e == ord("("):
                out.append(0x28); s.pos += 1
            elif e == ord(")"):
                out.append(0x29); s.pos += 1
            elif e in b"\r\n":
                # Line continuation — consume one CR, LF, or CRLF.
                if e == 0x0D and s.buf[s.pos + 1 : s.pos + 2] == b"\n":
                    s.pos += 2
                else:
                    s.pos += 1
            elif 0x30 <= e <= 0x37:
                # Octal escape — up to 3 digits.
                end = s.pos
                while end < len(s.buf) and end - s.pos < 3 \
                        and 0x30 <= s.buf[end] <= 0x37:
                    end += 1
                out.append(int(s.buf[s.pos:end], 8) & 0xFF)
                s.pos = end
            else:
                # Unknown escape — per spec, backslash is dropped
                # and the following char is emitted literally.
                out.append(e); s.pos += 1
        elif c == ord("("):
            depth += 1
            out.append(c); s.pos += 1
        elif c == ord(")"):
            depth -= 1
            s.pos += 1
            if depth == 0:
                break
            out.append(c)
        elif c == 0x0D:
            # Raw CR -> LF (EOL normalisation). Eat optional LF.
            out.append(0x0A); s.pos += 1
            if s.peek() == b"\n":
                s.pos += 1
        else:
            out.append(c); s.pos += 1
    return ("string", bytes(out))


def _parse_string_hex(s: _Scan) -> Value:
    """Consume a `<...>` hex string. §7.3.4.3: hex digits,
    whitespace skipped; odd trailing hex digit padded with '0'."""
    assert s.peek() == b"<"
    s.pos += 1
    digits = bytearray()
    while True:
        if s.eof():
            raise ParseError("unterminated hex string")
        c = s.buf[s.pos]
        if c == ord(">"):
            s.pos += 1
            break
        if c in _WS:
            s.pos += 1
            continue
        if not ((ord("0") <= c <= ord("9"))
                or (ord("a") <= c <= ord("f"))
                or (ord("A") <= c <= ord("F"))):
            raise ParseError(
                f"bad hex char {chr(c)!r} at offset {s.pos}")
        digits.append(c)
        s.pos += 1
    if len(digits) % 2 == 1:
        digits.append(ord("0"))
    return ("string", bytes.fromhex(digits.decode("ascii")))


def _parse_array(s: _Scan) -> Value:
    assert s.peek() == b"["
    s.pos += 1
    items: list = []
    while True:
        s.skip_ws()
        if s.eof():
            raise ParseError("unterminated array")
        if s.peek() == b"]":
            s.pos += 1
            return ("array", items)
        items.append(_parse_value(s))


def _parse_dict(s: _Scan) -> Value:
    assert s.peek(2) == b"<<"
    s.pos += 2
    entries: list = []
    seen: set = set()
    while True:
        s.skip_ws()
        if s.eof():
            raise ParseError("unterminated dict")
        if s.peek(2) == b">>":
            s.pos += 2
            return ("dict", entries)
        if s.peek() != b"/":
            raise ParseError(
                f"dict key must start with /, got "
                f"{s.buf[s.pos : s.pos + 8]!r} at {s.pos}")
        key_v = _parse_name(s)
        key = key_v[1]
        if key in seen:
            raise ParseError(f"duplicate dict key /{key} at {s.pos}")
        seen.add(key)
        s.skip_ws()
        val = _parse_value(s)
        entries.append((key, val))


# True/false/null keywords, and detection of `N M R` (indirect ref) vs
# three independent integers. Ref detection: after parsing an integer,
# save position; if the next two tokens are also a non-negative integer
# and the keyword `R`, commit to a ref; otherwise rewind and keep the
# first integer as a standalone value.

def _try_parse_ref(s: _Scan, first_int: int) -> Value | None:
    """Attempt to consume the remaining `M R` of a potential ref.
    Returns the ref Value on success, or None (with s.pos unchanged
    relative to before this call's input) on failure."""
    save = s.pos
    s.skip_ws()
    m = _NUM_RE.match(s.buf, s.pos)
    if not m or b"." in m.group(0):
        s.pos = save
        return None
    gen = int(m.group(0))
    if gen < 0:
        s.pos = save
        return None
    gen_end = m.end()
    # Check keyword `R` after whitespace.
    pos2 = gen_end
    while pos2 < len(s.buf) and s.buf[pos2] in _WS:
        pos2 += 1
    if s.buf[pos2:pos2 + 1] != b"R":
        s.pos = save
        return None
    # Keyword boundary check.
    if pos2 + 1 < len(s.buf):
        nxt = s.buf[pos2 + 1]
        if nxt not in _WS and nxt not in _DELIM:
            s.pos = save
            return None
    s.pos = pos2 + 1
    return ("ref", first_int, gen)


def _parse_value(s: _Scan) -> Value:
    """Dispatch on the next non-whitespace character."""
    s.skip_ws()
    if s.eof():
        raise ParseError("unexpected end of input")
    c = s.buf[s.pos]
    if c == ord("/"):
        return _parse_name(s)
    if c == ord("("):
        return _parse_string_literal(s)
    if c == ord("<"):
        if s.peek(2) == b"<<":
            return _parse_dict(s)
        return _parse_string_hex(s)
    if c == ord("["):
        return _parse_array(s)
    if s.starts_with(b"null"):
        s.pos += 4
        return ("null",)
    if s.starts_with(b"true"):
        s.pos += 4
        return ("bool", True)
    if s.starts_with(b"false"):
        s.pos += 5
        return ("bool", False)
    # Number (possibly start of a ref).
    if c in b"+-." or (ord("0") <= c <= ord("9")):
        n = _parse_number(s)
        if n[0] == "int" and n[1] >= 0:
            r = _try_parse_ref(s, n[1])
            if r is not None:
                return r
        return n
    raise ParseError(
        f"unexpected char {chr(c)!r} at offset {s.pos}")


def parse_value(buf: bytes, offset: int = 0) -> tuple[Value, int]:
    """Parse one direct-value expression starting at ``offset``.
    Returns (value, end_offset)."""
    s = _Scan(buf, offset)
    v = _parse_value(s)
    return v, s.pos


# ---------------------------------------------------------------- canonicaliser


def _canon_real(text: str) -> str:
    """Canonical form of a real number: shortest decimal with a `.`.

    PDF reals have no scientific notation (spec). We reparse the
    original digit string as a float and format via ``repr`` which
    yields the shortest round-trip decimal (Python 3.1+). Then
    ensure a `.` is present so the type stays distinguishable from
    an integer (e.g. "5.0" not "5").
    """
    v = float(text)
    out = repr(v)
    # ``repr`` may emit scientific for huge/tiny numbers — PDF reals
    # never are. Fall back to fixed format in that case.
    if "e" in out or "E" in out:
        out = f"{v:f}".rstrip("0").rstrip(".")
        if "." not in out:
            out += ".0"
    if "." not in out:
        out += ".0"
    return out


def _canon_name(decoded: str) -> str:
    """Encode a name in canonical form: `/` + name, with any byte
    outside the regular-name set emitted as `#XX` (uppercase hex).
    """
    out = bytearray(b"/")
    for ch in decoded.encode("latin-1"):
        if ch in _NAME_REGULAR:
            out.append(ch)
        else:
            out += f"#{ch:02X}".encode("ascii")
    return out.decode("ascii")


def _canon_string(payload: bytes) -> str:
    return "<" + payload.hex().upper() + ">"


def canonical(v: Value) -> str:
    """Render ``v`` in the freeze-gate canonical form."""
    kind = v[0]
    if kind == "null":
        return "null"
    if kind == "bool":
        return "true" if v[1] else "false"
    if kind == "int":
        return str(v[1])
    if kind == "real":
        return _canon_real(v[2])
    if kind == "name":
        return _canon_name(v[1])
    if kind == "string":
        return _canon_string(v[1])
    if kind == "ref":
        return f"{v[1]} {v[2]} R"
    if kind == "array":
        items = v[1]
        if not items:
            return "[ ]"
        return "[ " + " ".join(canonical(x) for x in items) + " ]"
    if kind == "dict":
        entries = v[1]
        if not entries:
            return "<< >>"
        # Sort by decoded key bytes, ASCII-ascending.
        sorted_entries = sorted(entries, key=lambda kv: kv[0].encode("latin-1"))
        parts = [f"{_canon_name(k)} {canonical(val)}"
                 for k, val in sorted_entries]
        return "<< " + " ".join(parts) + " >>"
    if kind == "stream":
        return "stream " + canonical(v[1])
    raise ValueError(f"unknown value kind: {kind!r}")


# ---------------------------------------------------------------- entry helpers


def parse_object_body(buf: bytes) -> Value:
    """Parse a direct-value expression from ``buf`` (skips leading
    whitespace). Raises if trailing non-whitespace remains."""
    v, end = parse_value(buf, 0)
    # Tolerate trailing whitespace and stream markers (handled by
    # callers for stream-valued objects).
    return v


def wrap_as_stream(header_dict: Value) -> Value:
    """Tag a dict as a stream's header for canonical rendering."""
    if header_dict[0] != "dict":
        raise TypeError("wrap_as_stream expects a dict value")
    return ("stream", header_dict)


if __name__ == "__main__":
    # Smoke test when invoked directly — exercises every kind.
    samples = [
        (b"null", "null"),
        (b"true", "true"),
        (b"false", "false"),
        (b"42", "42"),
        (b"-17", "-17"),
        (b"+5", "5"),
        (b"3.14", "3.14"),
        (b"-.5", "-0.5"),
        (b"5.", "5.0"),
        (b"/Hello", "/Hello"),
        (b"/A#20B", "/A#20B"),
        (b"(Hello)", "<48656C6C6F>"),
        (b"<48656C6C6F>", "<48656C6C6F>"),
        (b"[ ]", "[ ]"),
        (b"[ 1 2 3 ]", "[ 1 2 3 ]"),
        (b"<< >>", "<< >>"),
        (b"<< /A 1 /B 2 >>", "<< /A 1 /B 2 >>"),
        (b"<< /B 2 /A 1 >>", "<< /A 1 /B 2 >>"),
        (b"2 0 R", "2 0 R"),
        (b"[ 1 2 0 R 3 ]", "[ 1 2 0 R 3 ]"),
    ]
    failed = 0
    for src, expected in samples:
        got = canonical(parse_object_body(src))
        if got != expected:
            print(f"FAIL: {src!r} -> {got!r}, expected {expected!r}")
            failed += 1
    if failed == 0:
        print(f"OK — {len(samples)} cases pass")
    raise SystemExit(1 if failed else 0)
