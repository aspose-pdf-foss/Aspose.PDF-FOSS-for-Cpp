#!/usr/bin/env python3
"""freetype-py truetype adapter — independent second oracle for
the truetype primitive. Canonical form must byte-match
oracle_fonttools.py's output.

See oracle_fonttools.py for the canonical-form spec. freetype-py
wraps the FreeType 2 C library; it reads the same binary tables
as fontTools but via a completely separate code path (C
reference parser vs. pure-Python reimplementation), so
agreement between the two is a real independence signal.

Cmap subtable selection mirrors oracle_fonttools: prefer
(platform=3, encoding=1) — Windows Unicode BMP — then
(platform=0, encoding=3) — Unicode BMP. freetype-py does not
expose the subtable ``format`` field directly; per OpenType
spec both (3,1) and (0,3) cmap subtables must be format 4 in
compliant TTFs, so we trust the platform/encoding contract.
"""
from __future__ import annotations

import sys
from pathlib import Path

import freetype


def _select_charmap(face: freetype.Face) -> None:
    """Activate the (platform=3, enc=1) charmap, falling back to
    (platform=0, enc=3). Raises if neither exists."""
    for cm in face.charmaps:
        if cm.platform_id == 3 and cm.encoding_id == 1:
            face.set_charmap(cm)
            return
    for cm in face.charmaps:
        if cm.platform_id == 0 and cm.encoding_id == 3:
            face.set_charmap(cm)
            return
    raise RuntimeError(
        "no Unicode BMP charmap (platform 3/1 or 0/3) found "
        "(v1 scope is BMP-only)")


def _load_outline(face: freetype.Face, gid: int):
    """Load gid as a non-scaled, non-hinted, non-bitmap outline.
    Returns (points, contour_ends) where each point is a tuple
    (x, y, on_curve_bool). FreeType auto-expands composites at
    load time, applying any 2x2 transforms in font units."""
    face.load_glyph(
        gid,
        (freetype.FT_LOAD_NO_SCALE
         | freetype.FT_LOAD_NO_HINTING
         | freetype.FT_LOAD_NO_BITMAP))
    outline = face.glyph.outline
    pts = []
    for i, pt in enumerate(outline.points):
        # tags[i] bit 0: 1 = on-curve (anchor), 0 = off-curve
        # (control). Higher bits distinguish quadratic vs cubic
        # off-curves; v1 is TrueType-only so all off-curves are
        # quadratic — bit 0 is the only bit that matters here.
        on_curve = bool(outline.tags[i] & 0x01)
        pts.append((int(pt[0]), int(pt[1]), on_curve))
    ends = [int(e) for e in outline.contours]
    return pts, ends


def adapt(ttf_path: Path) -> str:
    face = freetype.Face(str(ttf_path))

    lines: list[str] = []
    lines.append(f"unitsPerEm {face.units_per_EM}")
    # face.ascender / face.descender come from hhea.ascender /
    # descender by default (sTypoAscent override via OS/2 flag
    # would kick in only when USE_TYPO_METRICS is set on the
    # font, which none of our fixtures does). Both adapters
    # thus report hhea values.
    lines.append(f"ascent {face.ascender}")
    lines.append(f"descent {face.descender}")
    lines.append(f"numGlyphs {face.num_glyphs}")

    # Per-glyph advance widths. FT_LOAD_NO_SCALE returns the
    # raw FUnit value from the hmtx table — no scaling, no
    # hinting, identical to fontTools' hmtx widths.
    for gid in range(face.num_glyphs):
        width = face.get_advance(gid, freetype.FT_LOAD_NO_SCALE)
        lines.append(f"width {gid} {width}")

    # cmap format-4 entries, walked via get_first_char /
    # get_next_char. The iteration terminates when gid returns
    # 0 — FreeType's sentinel. Codes are sorted naturally because
    # get_next_char walks in ascending order.
    _select_charmap(face)
    entries: list[tuple[int, int]] = []
    code, gid = face.get_first_char()
    while gid != 0:
        entries.append((code, gid))
        code, gid = face.get_next_char(code, 0)
    for code, gid in entries:
        lines.append(f"cmap {code:04X} {gid}")

    # Per-glyph outlines. FreeType auto-flattens composites,
    # matching fontTools' manual recursion. Quadratic off-curve
    # points are preserved as-is; consumers lift quad → cubic
    # at consumption time.
    for gid in range(face.num_glyphs):
        pts, ends = _load_outline(face, gid)
        if not pts:
            lines.append(f"glyph {gid} empty")
            continue
        lines.append(
            f"glyph {gid} simple {len(ends)} {len(pts)}")
        for (x, y, on) in pts:
            lines.append(f"  {'on' if on else 'off'} {x} {y}")
        for e in ends:
            lines.append(f"  contour_end {e}")

    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_freetype.py <fixture.ttf>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
