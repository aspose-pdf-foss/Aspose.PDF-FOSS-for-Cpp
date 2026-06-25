#!/usr/bin/env python3
"""fontTools cffLib adapter — emit the canonical form used by
the cff primitive's two_oracle_agreement freeze gate.

Canonical form (one record per line, trailing newline after last):

    unitsPerEm <N>
    isCID <0|1>
    fontName <postscript-name>     (bytes outside 0x21..0x7E
                                     encode as "%XX" uppercase)
    numGlyphs <N>
    glyph <gid> <name-or-cid> <advance>
      M <x> <y>                    (moveto)
      L <x> <y>                    (lineto)
      C <x1> <y1> <x2> <y2> <x3> <y3>   (curveto, cubic)
      Z                            (closepath)

Coordinates are integer font units (CFF stores everything as
integer deltas; the running cursor stays integer). Path lines
are indented two ASCII spaces.

For non-CID fonts <name-or-cid> is the glyph's PostScript name
("A", ".notdef"). For CID fonts it's "cid<N>" where N is the
CID in decimal (no padding) — fontTools labels CID glyphs as
"cid00001"-style internally; we strip the zero-padding for the
canonical form so the freetype oracle (which reports raw CIDs
via FT_Get_CID_From_Glyph_Index) can match byte-for-byte.
"""
from __future__ import annotations

import sys
from pathlib import Path

from fontTools.cffLib import CFFFontSet
from fontTools.misc.psCharStrings import T2WidthExtractor
from fontTools.pens.basePen import BasePen


class CanonicalRecorder(BasePen):
    """fontTools BasePen subclass that records every drawing
    command in the form expected by the canonical output. Path
    coordinates are rounded to int (CFF coords are integers in
    practice; fontTools emits them as floats for math
    convenience and we round back). _closePath / _endPath both
    emit Z because Type 2 endchar with an open subpath
    implicitly closes per Tech Note #5177 §4.1, and our anchor
    canonical form treats both the same way."""

    def __init__(self, glyphSet):
        super().__init__(glyphSet)
        self.events: list[tuple] = []
        self._open = False

    def _moveTo(self, p):
        # If a previous subpath is still open at this moveTo,
        # close it first — Type 2 multi-subpath glyphs (a
        # letter with a counter, like "B") have implicit
        # closes between subpaths.
        if self._open:
            self.events.append(("Z",))
        self.events.append(("M", round(p[0]), round(p[1])))
        self._open = True

    def _lineTo(self, p):
        self.events.append(("L", round(p[0]), round(p[1])))

    def _curveToOne(self, p1, p2, p3):
        self.events.append(("C",
                            round(p1[0]), round(p1[1]),
                            round(p2[0]), round(p2[1]),
                            round(p3[0]), round(p3[1])))

    def _qCurveToOne(self, p1, p2):
        # Type 2 charstrings produce ONLY cubic curves. A
        # quadratic in CFF data is a fontTools bug or a non-
        # CFF font slipped in. Surface loudly.
        raise RuntimeError("unexpected quadratic curve in CFF")

    def _closePath(self):
        if self._open:
            self.events.append(("Z",))
            self._open = False

    def _endPath(self):
        # Same canonical emission as _closePath — see class
        # docstring.
        if self._open:
            self.events.append(("Z",))
            self._open = False


def _escape_font_name(raw: bytes) -> str:
    """Encode bytes outside 0x21..0x7E as %XX uppercase. Inside
    that range each byte renders as its ASCII glyph. PostScript
    names are conventionally ASCII so this is usually a
    pass-through."""
    out = []
    for b in raw:
        if 0x21 <= b <= 0x7E and b != ord('%'):
            out.append(chr(b))
        else:
            out.append(f"%{b:02X}")
    return "".join(out)


def _normalise_cid_name(name: str) -> str:
    """fontTools labels CID glyphs as "cid00001" / "cid12345"
    with zero padding to 5 digits. The canonical form uses
    plain decimal ("cid1" / "cid12345"). Strip leading zeroes
    after the "cid" prefix."""
    if name.startswith("cid") and name[3:].isdigit():
        return f"cid{int(name[3:])}"
    return name


def _glyph_label(td, gid: int, glyph_name: str) -> str:
    """For CID fonts return cid<N>; for non-CID return the
    glyph name. fontTools labels CID glyphs as "cidNNNNN"
    EXCEPT for the .notdef glyph at gid 0, which it labels
    ".notdef" — but freetype reports its CID as 0 (the spec
    says .notdef IS CID 0 in a CID-keyed font). Special-case
    that mapping so the two oracles align."""
    if hasattr(td, "ROS"):
        if glyph_name == ".notdef":
            return "cid0"
        return _normalise_cid_name(glyph_name)
    return glyph_name


def adapt(cff_path: Path) -> str:
    cff_bytes = Path(cff_path).read_bytes()
    cff = CFFFontSet()
    import io
    cff.decompile(io.BytesIO(cff_bytes), otFont=None)

    font_name = cff.fontNames[0]
    td = cff[font_name]
    is_cid = hasattr(td, "ROS")

    # FontMatrix[0] scales font units to text space. Default
    # is 0.001, giving unitsPerEm = 1000.
    fm = getattr(td, "FontMatrix", [0.001, 0, 0, 0.001, 0, 0])
    units_per_em = round(1.0 / fm[0])

    charset = td.charset  # list of glyph names, one per gid
    num_glyphs = len(charset)

    lines: list[str] = []
    lines.append(f"unitsPerEm {units_per_em}")
    lines.append(f"isCID {1 if is_cid else 0}")
    lines.append(f"fontName {_escape_font_name(font_name.encode())}")
    lines.append(f"numGlyphs {num_glyphs}")

    # CharStrings are indexed by glyph name (the values stored
    # in `charset`). For each gid 0..N-1, find the name, fetch
    # the CharString, and execute it through CanonicalRecorder.
    for gid, name in enumerate(charset):
        cs = td.CharStrings[name]
        # Resolve advance width by running T2WidthExtractor —
        # it executes the program until it sees the first
        # stack-clearing operator and resolves the width
        # against default/nominalWidthX from the per-glyph
        # Private DICT (different per-FontDict for CID
        # fonts). gotWidth==0 means the glyph used
        # defaultWidthX implicitly.
        priv = cs.private
        extractor = T2WidthExtractor(
            localSubrs=priv.Subrs if hasattr(priv, "Subrs") else [],
            globalSubrs=cs.globalSubrs,
            nominalWidthX=priv.nominalWidthX,
            defaultWidthX=priv.defaultWidthX,
        )
        extractor.execute(cs)
        advance = round(float(extractor.width))
        label = _glyph_label(td, gid, name)
        lines.append(f"glyph {gid} {label} {advance}")

        # T2CharString.draw runs the Type 2 program directly
        # against the supplied pen — no GlyphSet indirection
        # needed. Subroutines (local + global) are expanded
        # internally; pen receives only moveto/lineto/curveto
        # / closePath / endPath calls.
        recorder = CanonicalRecorder(glyphSet=None)
        cs.draw(recorder)
        # Final implicit close at endchar if subpath is still
        # open. The recorder's _closePath/_endPath handle this
        # for explicit close ops; we close any leftover
        # subpath here for endchar-without-close programs.
        if recorder._open:
            recorder.events.append(("Z",))

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
        print("usage: oracle_fonttools.py <fixture.cff>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
