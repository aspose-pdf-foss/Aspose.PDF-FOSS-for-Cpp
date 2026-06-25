#!/usr/bin/env python3
"""One-shot corpus builder for the cff fixture set.

Subsets a small set of SIL-OFL-licensed source fonts to a
handful of glyphs each, extracts the resulting /CFF table, and
writes the raw CFF bytes as a fixture (e.g.
``masaramgondi.cff``). After running this script, run
``generate_fixtures.py`` to materialise the .canonical sidecars.

Sources:
  * /System/Library/Fonts/Supplemental/Noto*.otf — Apple
    redistributes the upstream Google Noto build under SIL OFL.
    We pick small, structurally varied fonts (different glyph
    counts, cmap ranges, hint complexity).
  * Source Han Sans CN Light (downloaded once into
    /tmp/source-han-sans-cn-light.otf if missing) —
    Adobe-OFL CID-keyed font, the canonical CIDFontType0C
    case for PDF.

The resulting .cff fixtures are committed; the source OTFs are
NOT (some are >5MB; we re-derive on demand). NOTICE.txt
records license terms for the redistributed fixture bytes.
"""
from __future__ import annotations

import io
import sys
import urllib.request
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options

HERE = Path(__file__).resolve().parent

SHS_URL = (
    "https://github.com/adobe-fonts/source-han-sans/raw/"
    "release/SubsetOTF/CN/SourceHanSansCN-Light.otf")
SHS_LOCAL = Path("/tmp/source-han-sans-cn-light.otf")


def _ensure_shs() -> Path:
    """Download the Source Han Sans CN Light OTF if absent.
    The file is ~8MB; we cache in /tmp because the corpus
    builder is rarely re-run."""
    if SHS_LOCAL.exists() and SHS_LOCAL.stat().st_size > 1_000_000:
        return SHS_LOCAL
    print(f"downloading {SHS_URL} → {SHS_LOCAL}")
    urllib.request.urlretrieve(SHS_URL, SHS_LOCAL)
    return SHS_LOCAL


def _subset(otf_path: Path, gids: list[int],
            output_path: Path) -> None:
    """Subset an OTF down to ``gids`` (a list of glyph IDs),
    extract the resulting /CFF table, and write the raw bytes
    to ``output_path``. The driver opens ``output_path`` as a
    raw CFF byte stream (Parse(span<byte>))."""
    f = TTFont(str(otf_path))
    glyph_order = f.getGlyphOrder()
    glyphs = [glyph_order[g] for g in gids if g < len(glyph_order)]
    opts = Options()
    opts.layout_features = []
    opts.glyph_names = True
    opts.notdef_outline = True
    opts.recommended_glyphs = False
    opts.legacy_kern = False
    opts.drop_tables = ["GPOS", "GSUB", "GDEF", "BASE", "JSTF",
                        "DSIG", "vhea", "vmtx"]
    s = Subsetter(options=opts)
    s.populate(glyphs=glyphs)
    s.subset(f)

    # Re-save to a memory buffer and re-read to ensure the CFF
    # offsets are recomputed for the subset glyph set.
    buf = io.BytesIO()
    f.save(buf)
    buf.seek(0)
    g = TTFont(buf)
    cff_bytes = g.getTableData("CFF ")
    output_path.write_bytes(cff_bytes)
    print(f"wrote {output_path.name} ({len(cff_bytes)} bytes, "
          f"{len(glyphs)} glyphs)")


def main() -> int:
    sources = [
        # Each tuple: (source_otf, glyph_ids_to_keep, output_name).
        # Glyph IDs include 0 (.notdef) plus a handful of
        # structurally interesting glyphs. We pick low GIDs
        # because these tend to be the simpler ASCII / basic
        # script glyphs the oracle is most likely to agree on.
        (Path("/System/Library/Fonts/Supplemental/"
              "NotoSansMasaramGondi-Regular.otf"),
         [0, 1, 2, 3, 4, 5, 6, 7],
         "masaramgondi.cff"),
        (Path("/System/Library/Fonts/Supplemental/"
              "NotoSerifYezidi-Regular.otf"),
         [0, 1, 2, 3, 4, 5],
         "yezidi.cff"),
        (Path("/System/Library/Fonts/Supplemental/"
              "NotoSansTifinagh-Regular.otf"),
         [0, 1, 2, 3, 4, 5, 6],
         "tifinagh.cff"),
    ]
    cid_source = _ensure_shs()
    sources.append(
        (cid_source, [0, 1, 2, 3, 4, 5, 6, 7],
         "sourcehansans_cn.cff"))

    rc = 0
    for src, gids, name in sources:
        out = HERE / name
        if not src.exists():
            print(f"SKIP {name}: source missing {src}",
                  file=sys.stderr)
            rc = 1
            continue
        try:
            _subset(src, gids, out)
        except Exception as e:
            print(f"FAIL {name}: {e}", file=sys.stderr)
            rc = 1
    return rc


if __name__ == "__main__":
    sys.exit(main())
