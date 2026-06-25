#!/usr/bin/env python3
"""fontTools truetype adapter — emit the canonical form used by
the truetype primitive's two_oracle_agreement freeze gate.

Canonical form (one record per line, trailing newline after last):

    unitsPerEm <N>
    ascent <N>
    descent <N>
    numGlyphs <N>
    width <glyphIndex> <width>     (one line per glyph, 0..numGlyphs-1)
    cmap <XXXX> <glyphIndex>       (format-4 entries, sorted by code)
    glyph <gid> empty                          (no points)
    glyph <gid> simple <ncontours> <npoints>
      on  <x> <y>                              (one point line)
      off <x> <y>
      ...
      contour_end <pointIndex>                 (one per contour)

Numbers are emitted in base 10 EXCEPT cmap codes, which are 4-digit
uppercase hex (no 0x prefix). A `.notdef` glyph with width 0 is
emitted as `width 0 0`.

Cmap subtable selection: v1 scope is BMP-only. Prefer the Windows-
Unicode BMP subtable (platform 3, encoding 1, format 4); fall back
to the Unicode BMP subtable (platform 0, encoding 3, format 4).
A font with neither subtable is out of v1 scope and this adapter
raises.

Glyph outlines: composites are flattened at adapter time using
the same recursion logic as the C++ body (offset + optional
2x2 transform). Quadratic-Bézier points are preserved as-is —
oracle_freetype's outline.points/.tags after FT_LOAD_NO_SCALE
also preserves the raw glyf point stream, so byte-agreement
between the two oracles holds without quadratic-to-cubic lifting.
"""
from __future__ import annotations

import sys
from pathlib import Path

from fontTools.ttLib import TTFont


def _pick_cmap_subtable(font: TTFont):
    """Return the format-4 Unicode BMP cmap subtable, preferring
    (platform=3, encoding=1) then (platform=0, encoding=3).
    Raises on neither present."""
    cmap = font["cmap"]
    # Windows Unicode BMP first.
    for sub in cmap.tables:
        if (sub.platformID == 3
                and sub.platEncID == 1
                and sub.format == 4):
            return sub
    # Unicode BMP second.
    for sub in cmap.tables:
        if (sub.platformID == 0
                and sub.platEncID == 3
                and sub.format == 4):
            return sub
    raise RuntimeError(
        "no format-4 Unicode BMP cmap subtable found "
        "(v1 scope is BMP-only)")


_MAX_DEPTH = 10


def _expand_glyph(font: TTFont, gid: int, depth: int = 0):
    """Return (points, contour_ends) for `gid`. Composites are
    flattened recursively; `points` is a list of (x, y, on_curve)
    tuples in absolute font units, `contour_ends` is the list
    of inclusive last-point indices per contour."""
    if depth > _MAX_DEPTH:
        raise RuntimeError(
            "truetype: composite glyph recursion depth exceeded")
    glyph_order = font.getGlyphOrder()
    name = glyph_order[gid]
    g = font["glyf"][name]
    if g.numberOfContours == 0:
        return [], []
    if g.numberOfContours > 0:
        # Simple glyph. fontTools exposes coordinates as a
        # GlyphCoordinates iterable of (x, y) and `flags` as
        # an array of byte values; bit 0 is on-curve.
        coords = g.coordinates
        flags = g.flags
        ends = [int(e) for e in g.endPtsOfContours]
        pts = [(int(coords[i][0]), int(coords[i][1]),
                bool(flags[i] & 0x01))
               for i in range(len(coords))]
        return pts, ends
    if g.numberOfContours == -1:
        # Composite. Walk components in declaration order;
        # apply each component's affine transform + offset to
        # the recursed point list and append.
        gid_by_name = {n: i for i, n in enumerate(glyph_order)}
        all_pts: list[tuple[int, int, bool]] = []
        all_ends: list[int] = []
        for c in g.components:
            sub_gid = gid_by_name[c.glyphName]
            sub_pts, sub_ends = _expand_glyph(
                font, sub_gid, depth + 1)
            xform = getattr(c, "transform", None)
            if xform is None:
                a, b, cc, d = 1.0, 0.0, 0.0, 1.0
            else:
                # fontTools stores xform as
                #   ((xscale, scale01), (scale10, yscale))
                (a, b), (cc, d) = xform
            dx = float(getattr(c, "x", 0))
            dy = float(getattr(c, "y", 0))
            base = len(all_pts)
            for (x, y, on) in sub_pts:
                nx = round(a * x + b * y + dx)
                ny = round(cc * x + d * y + dy)
                all_pts.append((int(nx), int(ny), on))
            for e in sub_ends:
                all_ends.append(e + base)
        return all_pts, all_ends
    raise RuntimeError(
        f"truetype: numberOfContours {g.numberOfContours} not "
        "in {-1} ∪ [0, 32767]")


def adapt(ttf_path: Path) -> str:
    font = TTFont(str(ttf_path))
    head = font["head"]
    hhea = font["hhea"]
    maxp = font["maxp"]
    hmtx = font["hmtx"]

    lines: list[str] = []
    lines.append(f"unitsPerEm {head.unitsPerEm}")
    lines.append(f"ascent {hhea.ascent}")
    lines.append(f"descent {hhea.descent}")
    lines.append(f"numGlyphs {maxp.numGlyphs}")

    # Per-glyph advance widths, indexed by glyph id (0..numGlyphs-1).
    # fontTools' hmtx is keyed by GLYPH NAME — convert via the
    # font's glyph order. hmtx[name] is (width, lsb).
    glyph_order = font.getGlyphOrder()
    for gid, name in enumerate(glyph_order):
        width, _lsb = hmtx[name]
        lines.append(f"width {gid} {width}")

    # cmap format-4 entries, sorted by character code.
    sub = _pick_cmap_subtable(font)
    gid_by_name = {name: gid for gid, name in enumerate(glyph_order)}
    for code in sorted(sub.cmap):
        name = sub.cmap[code]
        gid = gid_by_name[name]
        lines.append(f"cmap {code:04X} {gid}")

    # Per-glyph outlines: simple + composite (flattened).
    for gid in range(maxp.numGlyphs):
        pts, ends = _expand_glyph(font, gid)
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
        print("usage: oracle_fonttools.py <fixture.ttf>",
              file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
