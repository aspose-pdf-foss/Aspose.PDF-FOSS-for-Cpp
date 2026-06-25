#!/usr/bin/env python3
"""freetype-py cff adapter — independent second oracle for the
cff primitive. Canonical form must byte-match
oracle_fonttools.py's output.

freetype-py wraps the FreeType 2 C library; FreeType reads
raw CFF byte streams natively (the major-version byte at offset
0 is the magic) and provides outline points + tags via
FT_Outline. We decompose the outline through the
``outline.decompose`` callback API and synthesise a Z (close-
path) at every contour boundary, mirroring fontTools'
RecordingPen output.

For CID fonts FreeType doesn't expose glyph names (the CID
font program has no /CharStrings name table); we read CIDs
via ``FT_Get_CID_From_Glyph_Index`` (loaded via ctypes — the
freetype-py public Python API doesn't expose it) and emit
``cid<N>`` to match the fontTools oracle's normalised form.
"""
from __future__ import annotations

import ctypes
import ctypes.util
import sys
from pathlib import Path

import freetype


# Load libfreetype directly so we can call FT_Get_CID_From_Glyph_Index,
# which freetype-py wraps but does not expose. This is the same
# library freetype-py is using internally — they share a process
# heap so passing FT_Face pointers across the boundary is safe.
def _open_libfreetype() -> ctypes.CDLL:
    """Locate the freetype shared library on macOS / Linux. We
    try the system finder first, then a few common Homebrew /
    MacPorts paths, raising a clear error if none work."""
    candidates: list[str] = []
    sys_path = ctypes.util.find_library("freetype")
    if sys_path:
        candidates.append(sys_path)
    candidates += [
        "/opt/homebrew/lib/libfreetype.dylib",
        "/usr/local/lib/libfreetype.dylib",
        "/opt/homebrew/opt/freetype/lib/libfreetype.dylib",
        "/usr/lib/x86_64-linux-gnu/libfreetype.so.6",
        "/usr/lib/aarch64-linux-gnu/libfreetype.so.6",
    ]
    last_err: Exception | None = None
    for p in candidates:
        try:
            return ctypes.CDLL(p)
        except OSError as e:
            last_err = e
            continue
    raise RuntimeError(
        f"could not load libfreetype; tried {candidates}: "
        f"{last_err}")


_LIB = _open_libfreetype()
_FT_Get_CID = _LIB.FT_Get_CID_From_Glyph_Index
_FT_Get_CID.argtypes = [ctypes.c_void_p, ctypes.c_uint,
                        ctypes.POINTER(ctypes.c_uint)]
_FT_Get_CID.restype = ctypes.c_int


def _cid_for_gid(face: freetype.Face, gid: int) -> int:
    """Resolve glyph-id → CID for a CID-keyed font. Caller is
    responsible for first verifying face.is_cid_keyed; calling
    this on a non-CID face returns garbage."""
    cid = ctypes.c_uint(0)
    err = _FT_Get_CID(face._FT_Face, gid, ctypes.byref(cid))
    if err != 0:
        raise RuntimeError(
            f"FT_Get_CID_From_Glyph_Index(gid={gid}) "
            f"failed: error={err}")
    return cid.value


class CanonicalRecorder:
    """Walks an FT_Outline via the decompose callbacks and
    emits the canonical event list (M / L / C / Z) used by the
    cff freeze gate.

    Subtlety: FreeType's outline-decompose API closes contours
    by emitting a final line_to (or cubic_to) back to the
    contour-start point, not by signalling closepath. fontTools'
    BasePen, by contrast, gets an explicit _closePath callback
    from the Type 2 interpreter at endchar. To make the two
    streams byte-match we BUFFER one contour at a time, and on
    the next move_to (or on finish()) we flush the buffer
    after dropping the redundant trailing line/curve that
    re-touches the contour-start point. The result is M ... Z
    with no spurious final L. If a contour ends WITHOUT a
    redundant trailing line (rare in CFF but legal — e.g. a
    glyph whose program drew an open subpath then endchar'd)
    we still emit Z to mirror fontTools' implicit-close-at-
    endchar behaviour."""

    def __init__(self) -> None:
        self.events: list[tuple] = []
        self._buf: list[tuple] = []
        self._start: tuple[int, int] | None = None

    def _flush(self) -> None:
        if not self._buf:
            return
        # FreeType closes a contour by emitting a final line_to
        # (or rarely cubic_to) back to the contour-start point.
        # For LINE-TYPE closes the trailing line is purely an
        # artefact of FreeType's outline representation — fontTools
        # never emits it because the Type 2 interpreter signals
        # endchar/closepath directly. Drop those.
        # For CUBIC-TYPE closes the trailing cubic is a REAL
        # curve emitted by the program (its control points carry
        # real geometry); we keep it and just append Z.
        last = self._buf[-1]
        if last[0] == "L" and self._start is not None:
            end = (last[-2], last[-1])
            if end == self._start:
                self._buf.pop()
        self.events.extend(self._buf)
        self.events.append(("Z",))
        self._buf = []
        self._start = None

    # The freetype-py decompose callbacks pass FT_Vector
    # pointers; .x and .y are the raw 26.6 fixed-point integer
    # values, but with FT_LOAD_NO_SCALE they degenerate to raw
    # font-unit integers (no fractional bits set), so direct
    # int conversion is exact.
    def move_to(self, p, ctx=None):
        self._flush()
        self._start = (int(p.x), int(p.y))
        self._buf.append(("M", int(p.x), int(p.y)))

    def line_to(self, p, ctx=None):
        self._buf.append(("L", int(p.x), int(p.y)))

    def conic_to(self, p1, p2, ctx=None):
        # CFF Type 2 generates ONLY cubic curves; a quadratic
        # arrives only when the source isn't CFF. Surface the
        # mismatch so the fixture is rejected at oracle time.
        raise RuntimeError("unexpected quadratic curve in CFF")

    def cubic_to(self, p1, p2, p3, ctx=None):
        self._buf.append(("C",
                          int(p1.x), int(p1.y),
                          int(p2.x), int(p2.y),
                          int(p3.x), int(p3.y)))

    def finish(self):
        self._flush()


def _escape_font_name(raw: bytes) -> str:
    """%XX-encode bytes outside 0x21..0x7E (and the percent
    sign itself). Mirrors oracle_fonttools._escape_font_name
    byte-for-byte."""
    out = []
    for b in raw:
        if 0x21 <= b <= 0x7E and b != ord('%'):
            out.append(chr(b))
        else:
            out.append(f"%{b:02X}")
    return "".join(out)


def adapt(cff_path: Path) -> str:
    face = freetype.Face(str(cff_path))
    is_cid = face.is_cid_keyed
    num_glyphs = face.num_glyphs

    # FreeType derives unitsPerEm from CFF FontMatrix
    # automatically; for the default FontMatrix [0.001 ...] it
    # reports 1000.
    units_per_em = face.units_per_EM

    # FT_Face::family_name is the CFF font name for non-CID
    # fonts and the CID font name for CID. Both match the
    # PostScript name used by fontTools' cff.fontNames[0].
    raw_name = face.postscript_name or face.family_name or b""
    if isinstance(raw_name, str):
        raw_name = raw_name.encode("latin-1", errors="replace")

    lines: list[str] = []
    lines.append(f"unitsPerEm {units_per_em}")
    lines.append(f"isCID {1 if is_cid else 0}")
    lines.append(f"fontName {_escape_font_name(raw_name)}")
    lines.append(f"numGlyphs {num_glyphs}")

    for gid in range(num_glyphs):
        face.load_glyph(
            gid,
            freetype.FT_LOAD_NO_SCALE | freetype.FT_LOAD_NO_HINTING)
        slot = face.glyph
        outline = slot.outline
        advance = int(slot.advance.x)

        if is_cid:
            label = f"cid{_cid_for_gid(face, gid)}"
        else:
            # FT_Get_Glyph_Name returns bytes; decode as ASCII
            # since CFF/PostScript names are ASCII by spec.
            try:
                label_bytes = face.get_glyph_name(gid)
            except freetype.ft_errors.FT_Exception:
                label_bytes = b".notdef" if gid == 0 \
                              else f"gid{gid}".encode()
            label = label_bytes.decode("ascii", errors="replace")

        lines.append(f"glyph {gid} {label} {advance}")

        recorder = CanonicalRecorder()
        if len(outline.contours) > 0:
            outline.decompose(
                context=None,
                move_to=recorder.move_to,
                line_to=recorder.line_to,
                conic_to=recorder.conic_to,
                cubic_to=recorder.cubic_to,
            )
            recorder.finish()

        for ev in recorder.events:
            if ev[0] == "M":
                lines.append(f"  M {ev[1]} {ev[2]}")
            elif ev[0] == "L":
                lines.append(f"  L {ev[1]} {ev[2]}")
            elif ev[0] == "C":
                lines.append(
                    f"  C {ev[1]} {ev[2]} {ev[3]} {ev[4]} "
                    f"{ev[5]} {ev[6]}")
            elif ev[0] == "Z":
                lines.append("  Z")

    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_freetype.py <fixture.cff>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
