#!/usr/bin/env python3
"""Generate the fixture corpus for the page_renderer foundation
primitive's lossy_oracle_agreement_pdf freeze gate.

Two outputs per fixture:

  1. The .pdf itself — a tiny document whose /Catalog → /Pages
     → leaf has controlled MediaBox + content-stream so the
     renderer's output is deterministic.
  2. One .pN.pixels sidecar per page — RGBA8 raster of the
     page rendered at 72 DPI, computed as the per-pixel mean of
     ``mutool draw`` + ``pdftocairo``. The freeze gate's PSNR
     comparison is against this blended sidecar (see
     the project spec).

Layouts:

  * blank.pdf         — 100x100 page, no /Contents. Renders to
                        an opaque white image of the requested
                        DPI dimensions.
  * red_rect.pdf      — 100x100 page. Content stream paints a
                        50x50 red rectangle at (25,25). Used to
                        verify the rasterizer pipe-through:
                        non-corner pixels at (50,50) ought to
                        be red, corners should still be white.

Run: ``python3 generate_fixtures.py``. Requires mutool +
pdftocairo on PATH for sidecar generation.
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).parent

# Sidecar render DPI must match what the gate driver passes to
# Render() and what the spec pins as the natural 1:1 ratio.
SIDECAR_DPI = 72.0


def _build(objects: list[bytes], trailer: bytes) -> bytes:
    header = b"%PDF-1.5\n%\xe2\xe3\xcf\xd3\n"
    out = bytearray(header)
    offsets: list[int] = []
    for i, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{i} 0 obj\n".encode()
        out += body
        out += b"\nendobj\n"
    xref_pos = len(out)
    n = len(objects)
    out += f"xref\n0 {n + 1}\n".encode()
    out += b"0000000000 65535 f \n"
    for off in offsets:
        out += f"{off:010d} 00000 n \n".encode()
    out += b"trailer\n<<"
    out += trailer
    out += f">>\nstartxref\n{xref_pos}\n%%EOF".encode()
    return bytes(out)


def _write(path: Path, contents: bytes) -> None:
    path.write_bytes(contents)
    print(f"  wrote {path.name}  ({len(contents)} bytes)")


def build_blank() -> None:
    """100x100pt page, no /Contents — renderer output is fully
    opaque white at the configured DPI."""
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] /Resources << >> >>",
    ]
    trailer = b" /Size 4 /Root 1 0 R "
    _write(HERE / "blank.pdf", _build(objects, trailer))


def build_red_rect() -> None:
    """100x100pt page. Content stream:

        1 0 0 rg            % set non-stroking colour to red
        25 25 50 50 re      % rect (25,25)-(75,75) in PDF user space
        f                   % fill non-zero winding
    """
    cs = b"1 0 0 rg\n25 25 50 50 re\nf\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "red_rect.pdf", _build(objects, trailer))


def build_stroke_outline() -> None:
    """100x100pt page. Content stream:

        1 0 0 RG            % stroking colour: red
        5 w                 % line width: 5 pt
        25 25 50 50 re      % rect (25,25)-(75,75)
        S                   % stroke (no fill)

    With a 5-pt stroke centered on the path, the rect interior
    at (50,50) stays white; pixels close to the path edge
    (e.g. (25,50) or (50,25)) land inside the stroke and read
    as red.
    """
    cs = b"1 0 0 RG\n5 w\n25 25 50 50 re\nS\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "stroke_outline.pdf", _build(objects, trailer))


def build_cm_translate() -> None:
    """100x100pt page. Discriminating fixture for the `cm` operator
    (CTM concatenation). Content stream:

        1 0 0 rg
        q
        1 0 0 1 10 5 cm   % translate user-space by (10, 5)
        25 25 50 50 re    % rect in the post-cm local frame
        f
        Q

    The cm matrix `[1 0 0 1 10 5]` is a translation. PDF 32000
    §8.4.4 + §8.3.5 specify that each new cm left-multiplies
    the CTM (most recent cm closest to local coords), so:

        state.ctm_new = m × state.ctm_old   (correct)
        state.ctm_new = state.ctm_old × m   (wrong)

    Translation alone commutes with most matrices, BUT NOT with
    page_ctm (which has a y-flip via d=-1). The two orderings
    diverge on the f component:

        m × page_ctm = {1, 0, 0, -1, 10,  95}
        page_ctm × m = {1, 0, 0, -1, 10, 105}

    With the rect at local (25,25)-(75,75):
      * Correct device verts: (35, 70)-(85, 20). Image
        coords y∈[20,70].
      * Wrong device verts:   (35, 80)-(85, 30). Image
        coords y∈[30,80].

    Discriminator pixels:
      (60, 25): inside iff correct. Red iff correct, white
                iff wrong.
      (60, 75): inside iff wrong. Red iff wrong, white iff
                correct.

    Each pixel is the complement of the other, so a smoke that
    asserts both rules out "rect not drawn at all" / "rect
    drawn double" failure modes that a single-pixel check
    would miss.
    """
    cs = (b"1 0 0 rg\n"
          b"q\n"
          b"1 0 0 1 10 5 cm\n"
          b"25 25 50 50 re\n"
          b"f\n"
          b"Q\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "cm_translate.pdf", _build(objects, trailer))


def build_bezier_fill() -> None:
    """100x100pt page. Content stream constructs a closed cubic
    Bézier path that bulges out from a baseline (50,20)→(50,80)
    via two off-curve control points and fills it green.

        0 1 0 rg
        20 20 m
        20 80 80 80 80 20 c   % cubic from (20,20) -> (80,20)
        h                     % close back to (20,20)
        f

    The path encloses roughly the lower half of the page; the
    pixel at (50,30) falls inside, while (50,90) is clearly
    outside.
    """
    cs = (b"0 1 0 rg\n"
          b"20 20 m\n"
          b"20 80 80 80 80 20 c\n"
          b"h\nf\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "bezier_fill.pdf", _build(objects, trailer))


def _build_tt_subset_hello() -> bytes:
    """Build a tiny TrueType font with rectangle-shaped glyphs
    for H/e/l/o/space. Each glyph is a single filled contour;
    proportions are picked so a fixed-pixel pixel-test can hit
    the H stem and miss the corner reliably.

    The smoke test only checks 'centre pixel inside the run is
    non-white' / 'corner is white' — so the glyphs don't need
    to be readable, just to land predictably in their bbox."""
    from fontTools.fontBuilder import FontBuilder
    from fontTools.pens.ttGlyphPen import TTGlyphPen

    fb = FontBuilder(unitsPerEm=1000, isTTF=True)
    glyph_order = [".notdef", "space", "H", "e", "l", "o"]
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap({
        0x20: "space",
        0x48: "H",
        0x65: "e",
        0x6c: "l",
        0x6f: "o",
    })

    def rect_glyph(x0: int, y0: int, x1: int, y1: int):
        pen = TTGlyphPen(None)
        pen.moveTo((x0, y0))
        pen.lineTo((x1, y0))
        pen.lineTo((x1, y1))
        pen.lineTo((x0, y1))
        pen.closePath()
        return pen.glyph()

    def empty_glyph():
        pen = TTGlyphPen(None)
        return pen.glyph()

    glyphs = {
        ".notdef": rect_glyph(50, 0, 550, 700),
        "space":   empty_glyph(),
        "H":       rect_glyph(50, 0, 550, 700),
        "e":       rect_glyph(50, 0, 450, 500),
        "l":       rect_glyph(50, 0, 150, 700),
        "o":       rect_glyph(50, 0, 450, 500),
    }
    fb.setupGlyf(glyphs)
    metrics = {
        ".notdef": (600, 50),
        "space":   (300, 0),
        "H":       (600, 50),
        "e":       (500, 50),
        "l":       (200, 50),
        "o":       (500, 50),
    }
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupOS2(sTypoAscender=800, sTypoDescender=-200,
                usWinAscent=800, usWinDescent=200)
    fb.setupNameTable({
        "familyName": "HelloRect",
        "styleName":  "Regular",
    })
    fb.setupPost()
    import io
    buf = io.BytesIO()
    fb.font.save(buf)
    return buf.getvalue()


_F1_WIDTHS_BLOCK = (
    # /FirstChar 32 /LastChar 111 → 80 entries indexed by (char-32):
    #   index  0 (char 32 space) = 300
    #   index 40 (char 72 'H')   = 600
    #   index 69 (char 101 'e')  = 500
    #   index 76 (char 108 'l')  = 200
    #   index 79 (char 111 'o')  = 500
    # all others zero. Five rows of 16 entries each.
    b"/FirstChar 32 /LastChar 111 "
    b"/Widths [300 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "       # 0..15
    b"0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "                  # 16..31
    b"0 0 0 0 0 0 0 0 600 0 0 0 0 0 0 0 "                # 32..47
    b"0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "                  # 48..63
    b"0 0 0 0 0 500 0 0 0 0 0 0 200 0 0 500] "           # 64..79
)


def _f1_font_block(font_id: int, descriptor_id: int,
                   program_id: int) -> tuple[bytes, bytes, bytes]:
    """Build the (font_dict, font_descriptor, program_stream) trio
    for the HelloRect TrueType subset. Caller passes the object IDs
    that will be assigned at xref-emit time so cross-references are
    correctly back-patched.
    """
    import zlib
    tt_program = _build_tt_subset_hello()
    tt_compressed = zlib.compress(tt_program)
    font_dict = (
        b"<< /Type /Font /Subtype /TrueType /BaseFont /HelloRect "
        b"/FontDescriptor " + str(descriptor_id).encode() + b" 0 R "
        + _F1_WIDTHS_BLOCK + b">>"
    )
    descriptor = (
        b"<< /Type /FontDescriptor /FontName /HelloRect "
        b"/FontFile2 " + str(program_id).encode() + b" 0 R "
        b"/Flags 32 /FontBBox [0 0 600 800] "
        b"/ItalicAngle 0 /Ascent 800 /Descent -200 "
        b"/CapHeight 700 /StemV 80 >>"
    )
    program_stream = (
        b"<< /Length " + str(len(tt_compressed)).encode() +
        b" /Filter /FlateDecode /Length1 " +
        str(len(tt_program)).encode() +
        b" >>\nstream\n" + tt_compressed + b"\nendstream"
    )
    return font_dict, descriptor, program_stream


def build_text_hello() -> None:
    """200x80pt page. Two text runs:

       BT /F1 24 Tf 20 30 Td (Hello) Tj ET
       BT /F2 24 Tf 130 30 Td <01> Tj ET

    /F1 is the rectangle-glyph TrueType subset built above —
    'H' is a 600x700 FUnit rect at upem=1000, so at fontsize 24
    it spans roughly PDF x ∈ [21.2, 33.2] and y ∈ [30, 46.8].
    /F2 is the Yezidi CFF fixture (see Fixtures/cff/yezidi.cff)
    with /Encoding /Differences mapping byte 0x01 → /elifyezi —
    the glyph bbox is the renderer's actual CFF outline, so the
    smoke test asserts non-white anywhere inside its drawn
    region rather than at a hand-picked pixel.
    """
    import zlib

    tt_program = _build_tt_subset_hello()
    tt_compressed = zlib.compress(tt_program)
    cff_path = HERE.parent / "cff" / "yezidi.cff"
    cff_program = cff_path.read_bytes()
    cff_compressed = zlib.compress(cff_program)

    cs = (b"BT /F1 24 Tf 20 30 Td (Hello) Tj ET\n"
          b"BT /F2 24 Tf 130 30 Td <01> Tj ET\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )

    f1_descriptor = (
        b"<< /Type /FontDescriptor /FontName /HelloRect "
        b"/FontFile2 7 0 R /Flags 32 /FontBBox [0 0 600 800] "
        b"/ItalicAngle 0 /Ascent 800 /Descent -200 "
        b"/CapHeight 700 /StemV 80 >>"
    )
    f1_program_stream = (
        b"<< /Length " + str(len(tt_compressed)).encode() +
        b" /Filter /FlateDecode /Length1 " +
        str(len(tt_program)).encode() +
        b" >>\nstream\n" + tt_compressed + b"\nendstream"
    )
    f2_descriptor = (
        b"<< /Type /FontDescriptor /FontName /NotoSerifYezidi "
        b"/FontFile3 10 0 R /Flags 4 /FontBBox [0 0 1000 1000] "
        b"/ItalicAngle 0 /Ascent 800 /Descent -200 "
        b"/CapHeight 700 /StemV 80 >>"
    )
    f2_program_stream = (
        b"<< /Length " + str(len(cff_compressed)).encode() +
        b" /Filter /FlateDecode /Subtype /Type1C >>\n"
        b"stream\n" + cff_compressed + b"\nendstream"
    )

    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 80] "
        b"/Contents 4 0 R "
        b"/Resources << /Font << /F1 5 0 R /F2 8 0 R >> >> >>",
        contents_obj,
        b"<< /Type /Font /Subtype /TrueType /BaseFont /HelloRect "
        b"/FontDescriptor 6 0 R "
        + _F1_WIDTHS_BLOCK +
        b">>",
        f1_descriptor,
        f1_program_stream,
        b"<< /Type /Font /Subtype /Type1 /BaseFont /NotoSerifYezidi "
        b"/FontDescriptor 9 0 R "
        b"/Encoding << /Type /Encoding "
        b"/Differences [1 /elifyezi] >> "
        b"/FirstChar 1 /LastChar 1 /Widths [600] >>",
        f2_descriptor,
        f2_program_stream,
    ]
    trailer = b" /Size 11 /Root 1 0 R "
    _write(HERE / "text_hello.pdf", _build(objects, trailer))


def build_image_xobject() -> None:
    """200x80pt page with a text run AND an image XObject draw:

       BT /F1 24 Tf 20 30 Td (Hello) Tj ET    % regression for Beat 3b
       q 100 0 0 50 50 15 cm /Im0 Do Q        % image drawn at PDF
                                              % (50, 15)-(150, 65)

    The cm `[100 0 0 50 50 15]` scales the unit square to a
    100x50 PDF-pt rectangle and translates its corner to
    (50, 15). Per PDF spec §8.9.5 the image's data row 0
    (top of the image data) lands at PDF user (0, 1) of the
    unit square (top-left). After page_ctm (y-flip; page
    height 80) the image occupies image x∈[50,150],
    y∈[15,65] (top-down convention).

    /Im0 is a 4x4 solid-red DeviceRGB DCTDecode JPEG.
    Smoke pixels:
       (100, 40) — centre of the image bbox, must be red.
       (180, 40) — to the right of the image, must be white.
       (25, 40)  — H glyph stem, must be non-white (Beat 3b
                   regression — confirms text path still works
                   when XObjects are also resolved on the page).
    """
    import io
    import zlib
    from PIL import Image

    # 4x4 solid red. Quality=100 + subsampling=0 keeps the
    # JPEG-decoded output within ±5 of (255, 0, 0) per
    # channel (the PixelEquals tolerance the smoke uses).
    src = Image.new("RGB", (4, 4), color=(255, 0, 0))
    jpeg_buf = io.BytesIO()
    src.save(jpeg_buf, format="JPEG", quality=100,
             subsampling=0)
    jpeg_bytes = jpeg_buf.getvalue()

    tt_program = _build_tt_subset_hello()
    tt_compressed = zlib.compress(tt_program)

    cs = (b"BT /F1 24 Tf 20 30 Td (Hello) Tj ET\n"
          b"q 100 0 0 50 50 15 cm /Im0 Do Q\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )

    f1_descriptor = (
        b"<< /Type /FontDescriptor /FontName /HelloRect "
        b"/FontFile2 7 0 R /Flags 32 /FontBBox [0 0 600 800] "
        b"/ItalicAngle 0 /Ascent 800 /Descent -200 "
        b"/CapHeight 700 /StemV 80 >>"
    )
    f1_program_stream = (
        b"<< /Length " + str(len(tt_compressed)).encode() +
        b" /Filter /FlateDecode /Length1 " +
        str(len(tt_program)).encode() +
        b" >>\nstream\n" + tt_compressed + b"\nendstream"
    )

    # /Im0 — 4x4 RGB JPEG, /DCTDecode. /Width and /Height are
    # informational; foundation::dctdecode reads dimensions
    # from the JPEG SOF marker.
    im0_stream = (
        b"<< /Type /XObject /Subtype /Image "
        b"/Width 4 /Height 4 /BitsPerComponent 8 "
        b"/ColorSpace /DeviceRGB /Filter /DCTDecode "
        b"/Length " + str(len(jpeg_bytes)).encode() + b" >>\n"
        b"stream\n" + jpeg_bytes + b"\nendstream"
    )

    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 80] "
        b"/Contents 4 0 R "
        b"/Resources << /Font << /F1 5 0 R >> "
        b"/XObject << /Im0 8 0 R >> >> >>",
        contents_obj,
        b"<< /Type /Font /Subtype /TrueType /BaseFont /HelloRect "
        b"/FontDescriptor 6 0 R "
        + _F1_WIDTHS_BLOCK +
        b">>",
        f1_descriptor,
        f1_program_stream,
        im0_stream,
    ]
    trailer = b" /Size 9 /Root 1 0 R "
    _write(HERE / "image_xobject.pdf", _build(objects, trailer))


def build_two_pages() -> None:
    """Two 100x100pt pages, distinct fills:

      page 1: red 50x50 rect at PDF user (25,25)-(75,75) — same
              shape as red_rect.pdf
      page 2: green 50x50 rect at PDF user (25,25)-(75,75)

    Used by the TiffDevice multi-page smoke (ProcessRange) to
    confirm each IFD in the produced TIFF carries the correct
    page's pixel data — page 1 centre decodes red, page 2 centre
    decodes green. Distinguishes a body that emits one IFD twice
    or stitches the wrong page bodies into the wrong directories.
    """
    cs1 = b"1 0 0 rg\n25 25 50 50 re\nf\n"
    cs2 = b"0 1 0 rg\n25 25 50 50 re\nf\n"
    contents1 = (
        b"<< /Length " + str(len(cs1)).encode() + b" >>\n"
        b"stream\n" + cs1 + b"endstream"
    )
    contents2 = (
        b"<< /Length " + str(len(cs2)).encode() + b" >>\n"
        b"stream\n" + cs2 + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 5 0 R /Resources << >> >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 6 0 R /Resources << >> >>",
        contents1,
        contents2,
    ]
    trailer = b" /Size 7 /Root 1 0 R "
    _write(HERE / "two_pages.pdf", _build(objects, trailer))


def build_even_odd_fill() -> None:
    """100x100pt page. Self-intersecting path filled with `f*`
    (even-odd winding):

        1 0 0 rg
        20 20 60 60 re      % outer rect (20,20)-(80,80), wound CCW
        40 40 20 20 re      % inner rect (40,40)-(60,60), wound CCW
        f*

    Even-odd evaluates winding parity, not signed sum:
      * outer ring  PDF (30,30) → 1 boundary crossing  → odd  → filled
      * inner area  PDF (50,50) → 2 boundary crossings → even → NOT
                                                           filled

    Non-zero (`f`) treats both subpaths as positive winding → the
    inner area is filled too. Discriminator pixel at PDF (50,50) →
    image (50,50) is white iff f* honoured, red iff f wrongly used.
    Companion pixel at PDF (30,30) → image (30,70) is red under
    both rules; it confirms the outer subpath drew at all.
    """
    cs = (b"1 0 0 rg\n"
          b"20 20 60 60 re\n"
          b"40 40 20 20 re\n"
          b"f*\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "even_odd_fill.pdf", _build(objects, trailer))


def build_bezier_v_y() -> None:
    """200x80pt page. Two filled curve-and-line shapes — one curve
    via `v`, one via `y`:

        1 0 0 rg                         % red bump via v
        20 20 m
        50 60 80 20 v                    % cp1 = current = (20,20)
                                         % cp2 = (50,60), end = (80,20)
        80 20 l                          % no-op (already at end)
        20 20 l
        h
        f

        0 1 0 rg                         % green bump via y
        120 20 m
        140 60 180 20 y                  % cp1 = (140,60)
                                         % cp2 = end = (180,20)
        180 20 l                         % no-op
        120 20 l
        h
        f

    PDF §8.5.2.3: `v` reuses the current point as the first control
    point; `y` reuses the endpoint as the second control. A renderer
    that mishandles either op draws a different (or empty) bump.

    Each closed shape's interior lies between the line at PDF y=20
    and the curve apex (≈ y=35). Discriminator pixels:
      PDF (38, 25) → image (38, 55): inside the v-bump (red).
      PDF (158, 25) → image (158, 55): inside the y-bump (green).
    """
    cs = (b"1 0 0 rg\n"
          b"20 20 m\n"
          b"50 60 80 20 v\n"
          b"80 20 l\n"
          b"20 20 l\n"
          b"h\nf\n"
          b"0 1 0 rg\n"
          b"120 20 m\n"
          b"140 60 180 20 y\n"
          b"180 20 l\n"
          b"120 20 l\n"
          b"h\nf\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 80] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "bezier_v_y.pdf", _build(objects, trailer))


def build_fill_stroke_combos() -> None:
    """100x100pt page. Four small shapes painted with B / B* / b / b*:

        0 0 0 RG  3 w                         % black stroke, width 3

        1 0 0 rg
        10 60 30 30 re
        B                                     % fill (NZ) + stroke

        0 1 0 rg
        60 60 30 30 re
        B*                                    % fill (EO) + stroke

        0 0 1 rg
        10 10 m
        40 10 l
        25 35 l
        b                                     % close + fill (NZ)
                                              %        + stroke

        1 1 0 rg
        60 10 m
        90 10 l
        75 35 l
        b*                                    % close + fill (EO)
                                              %        + stroke

    The two top shapes use `re`, which auto-closes the subpath; B /
    B* therefore differ only in the winding rule (immaterial for a
    convex rect). The two bottom triangles are OPEN — the third edge
    only paints because b / b* close the path before painting.

    Discriminator pixels (image space, page height 100):
      (25, 25) image: red    centre of B  rect (PDF 25, 75)
      (75, 25) image: green  centre of B* rect (PDF 75, 75)
      (25, 85) image: blue   inside b  triangle (PDF 25, 15)
      (75, 85) image: yellow inside b* triangle (PDF 75, 15)
    Each smoke pixel verifies its operator drew the fill; a renderer
    that mistakes b/b* for B/B* leaves the triangle's third edge
    unstroked AND the fill's left/right boundary undefined.
    """
    cs = (b"0 0 0 RG\n3 w\n"
          b"1 0 0 rg\n"
          b"10 60 30 30 re\n"
          b"B\n"
          b"0 1 0 rg\n"
          b"60 60 30 30 re\n"
          b"B*\n"
          b"0 0 1 rg\n"
          b"10 10 m\n"
          b"40 10 l\n"
          b"25 35 l\n"
          b"b\n"
          b"1 1 0 rg\n"
          b"60 10 m\n"
          b"90 10 l\n"
          b"75 35 l\n"
          b"b*\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "fill_stroke_combos.pdf", _build(objects, trailer))


def build_device_gray() -> None:
    """100x100pt page. Exercises `g` (DeviceGray fill) + `G`
    (DeviceGray stroke) — distinct ops from rg / RG and a separate
    code path through the colour-space machinery:

        0.5 g                             % fill colour DeviceGray 0.5
        20 60 60 30 re                    % rect (20,60)-(80,90)
        f
        0 G                               % stroke colour DeviceGray 0
        5 w
        20 10 60 30 re                    % rect (20,10)-(80,40)
        S

    Discriminator pixels (image space, page height 100):
      (50, 25) image: mid-gray (~128 RGB) iff `g` honoured. A
                       renderer that ignores the op leaves the
                       background opaque white.
      (50, 90) image: black-ish stroke band along PDF y=10 ± 2.5,
                       iff `G` honoured. Centre of the stroked
                       rect (image (50, 75)) stays white.
    """
    cs = (b"0.5 g\n"
          b"20 60 60 30 re\n"
          b"f\n"
          b"0 G\n"
          b"5 w\n"
          b"20 10 60 30 re\n"
          b"S\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "device_gray.pdf", _build(objects, trailer))


def build_line_subpath() -> None:
    """100x100pt page. Triangle via `m` + 2 × `l` + `h` + `f`:

        0 0 1 rg
        20 20 m
        80 20 l
        50 80 l
        h
        f

    Every other path-fixture in this corpus uses `re` rectangles.
    A renderer that handles `re` but mishandles raw m/l/h subpaths
    fails this fixture even with a green sweep elsewhere.

    At PDF y=50 the triangle interior spans x∈[35, 65]. Discriminator
    pixel: PDF (50, 50) → image (50, 50) must be blue.
    """
    cs = (b"0 0 1 rg\n"
          b"20 20 m\n"
          b"80 20 l\n"
          b"50 80 l\n"
          b"h\nf\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "line_subpath.pdf", _build(objects, trailer))


def build_text_state() -> None:
    """200x80pt page. Three lines of "Hello" / "Hello Hello"
    exercising the Tm + TD + T* + Tc + Tw text-state machinery:

        BT
        /F1 24 Tf
        1 0 0 1 20 60 Tm                 % Tm sets Tm AND Tlm
        2 Tc                             % char spacing
        4 Tw                             % word spacing (ASCII space)
        (Hello) Tj                       % line 1, baseline (20, 60)
        0 -20 TD                         % new line, sets Tl=20
        (Hello) Tj                       % line 2, baseline (20, 40)
        T*                               % uses Tl to advance
        (Hello Hello) Tj                 % line 3, baseline (20, 20),
                                         % space gets +Tw shift
        ET

    The HelloRect TrueType subset has rect-shaped H/e/l/o/space
    glyphs. At fontsize 24 with ascent 800/1000, each H stem spans
    PDF y∈[baseline, baseline+16.8] and PDF x∈[~21.2, ~33.2].

    Discriminator pixels (image space; page height 80, y-flipped):
      (25, 14) image: line 1 H stem  (PDF y ≈ [60, 76.8])
      (25, 34) image: line 2 H stem  (PDF y ≈ [40, 56.8])
      (25, 54) image: line 3 H stem  (PDF y ≈ [20, 36.8])
    All three must be non-white. A renderer that drops T* (no
    advance) stacks lines 2 and 3 on line 1; one that ignores TD's
    leading-set side effect raises an error or shifts line 3 to the
    wrong y.
    """
    font_dict, descriptor, program_stream = _f1_font_block(
        font_id=5, descriptor_id=6, program_id=7)
    cs = (b"BT\n"
          b"/F1 24 Tf\n"
          b"1 0 0 1 20 60 Tm\n"
          b"2 Tc\n"
          b"4 Tw\n"
          b"(Hello) Tj\n"
          b"0 -20 TD\n"
          b"(Hello) Tj\n"
          b"T*\n"
          b"(Hello Hello) Tj\n"
          b"ET\n")
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 80] "
        b"/Contents 4 0 R "
        b"/Resources << /Font << /F1 5 0 R >> >> >>",
        contents_obj,
        font_dict,
        descriptor,
        program_stream,
    ]
    trailer = b" /Size 8 /Root 1 0 R "
    _write(HERE / "text_state.pdf", _build(objects, trailer))


def build_tj_array_kerning() -> None:
    """200x80pt page. Single text run with TJ-array kerning:

        BT /F1 24 Tf 20 30 Td [(H)-2000(e)] TJ ET

    PDF §9.4.3 — TJ numeric elements between strings shift the text
    matrix by ``-num × Tfs / 1000`` user-space units. With Tfs=24
    and num=-2000, the shift is +48 pt, on top of H's natural
    advance (~14.4 pt with H width 600 FUnits at upem 1000).

    Glyph positions (HelloRect rect-shaped):
      'H' lands at PDF x ≈ [21.2, 33.2] — same regardless of
                                          kerning honour.
      'e' lands at PDF x ≈ [83.6, 94.4] iff TJ kerning HONOURED.
      'e' lands at PDF x ≈ [35.6, 46.4] iff TJ kerning IGNORED.

    The two 'e' positions are non-overlapping, so each is a clean
    discriminator (image y=50 picks the rect-glyph mid-height; PDF
    y=30 baseline → image y = 80 - 30 = 50 sits inside the H/e
    glyph rect's image y∈[~30.8, ~50]):
      (88, 44) image: inside 'e' iff TJ kerning HONOURED.
      (40, 44) image: inside 'e' iff TJ kerning IGNORED.
    A correct renderer fills (88, 44) with non-white and (40, 44)
    with white.
    """
    font_dict, descriptor, program_stream = _f1_font_block(
        font_id=5, descriptor_id=6, program_id=7)
    cs = b"BT /F1 24 Tf 20 30 Td [(H)-2000(e)] TJ ET\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R "
        b"/MediaBox [0 0 200 80] "
        b"/Contents 4 0 R "
        b"/Resources << /Font << /F1 5 0 R >> >> >>",
        contents_obj,
        font_dict,
        descriptor,
        program_stream,
    ]
    trailer = b" /Size 8 /Root 1 0 R "
    _write(HERE / "tj_array_kerning.pdf", _build(objects, trailer))


def _read_png_rgba(path: Path) -> tuple[int, int, bytes]:
    """Decode a PNG into (width, height, RGBA8 bytes).

    Both oracles render to PNG (mutool's -F png and pdftocairo's
    -png). PIL collapses any colour-mode mismatch (RGB vs RGBA
    vs greyscale) to RGBA8 via .convert(); the .tobytes() output
    is row-major top-left-first, channel order R G B A.
    """
    from PIL import Image
    with Image.open(path) as im:
        rgba = im.convert("RGBA")
        return rgba.width, rgba.height, rgba.tobytes()


def _blend_rgba(a: bytes, b: bytes) -> bytes:
    """Per-pixel arithmetic mean of two equal-length RGBA8
    buffers. Banker-style truncation toward zero — the +1 / 2
    trick rounds halves up, but for fixture sidecars where the
    two oracles already agreed (gate authoring rejects fixtures
    where they disagree above tolerance) the off-by-one rarely
    matters; we go with `(a + b) // 2` for symmetry.
    """
    if len(a) != len(b):
        raise ValueError(
            f"blend: length mismatch {len(a)} vs {len(b)}")
    out = bytearray(len(a))
    for i in range(len(a)):
        out[i] = (a[i] + b[i]) // 2
    return bytes(out)


def _render_via_mutool(pdf: Path, page_index: int,
                       out_dir: Path) -> Path:
    """Render `page_index` (0-based) via ``mutool draw`` at
    SIDECAR_DPI to a PNG under ``out_dir``. Mutool's CLI takes
    a 1-based page argument, so the +1 conversion lives here.
    """
    page_1 = page_index + 1
    out_path = out_dir / f"mutool_p{page_index}.png"
    subprocess.run(
        ["mutool", "draw",
         "-r", f"{SIDECAR_DPI:g}",
         "-F", "png",
         "-o", str(out_path),
         str(pdf), str(page_1)],
        check=True, capture_output=True)
    return out_path


def _render_via_pdftocairo(pdf: Path, page_index: int,
                            out_dir: Path) -> Path:
    """Render `page_index` (0-based) via ``pdftocairo`` at
    SIDECAR_DPI to a PNG. pdftocairo's -f / -l flags are
    1-based; -singlefile suppresses the `<root>-<page>` suffix
    so the output lands at `<root>.png` exactly.
    """
    page_1 = page_index + 1
    out_root = out_dir / f"pdftocairo_p{page_index}"
    subprocess.run(
        ["pdftocairo",
         "-r", f"{SIDECAR_DPI:g}",
         "-f", str(page_1),
         "-l", str(page_1),
         "-png",
         "-singlefile",
         str(pdf), str(out_root)],
        check=True, capture_output=True)
    return Path(str(out_root) + ".png")


def _build_sidecars_for(pdf: Path, page_count: int) -> None:
    """Render each 0..page_count-1 page via both oracles, blend
    per-pixel, write `<base>.p<index>.pixels`. Asserts that the
    two oracles agree on dimensions; a dimension mismatch
    indicates an authoring-time bug worth surfacing loudly."""
    base = pdf.with_suffix("")
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        for page_index in range(page_count):
            mu_png = _render_via_mutool(pdf, page_index, td_path)
            ca_png = _render_via_pdftocairo(
                pdf, page_index, td_path)
            mu_w, mu_h, mu_rgba = _read_png_rgba(mu_png)
            ca_w, ca_h, ca_rgba = _read_png_rgba(ca_png)
            if (mu_w, mu_h) != (ca_w, ca_h):
                raise RuntimeError(
                    f"{pdf.name} p{page_index}: "
                    f"oracle dimension disagreement — "
                    f"mutool={mu_w}x{mu_h} "
                    f"pdftocairo={ca_w}x{ca_h}")
            blended = _blend_rgba(mu_rgba, ca_rgba)
            sidecar = base.parent / (
                f"{base.name}.p{page_index}.pixels")
            sidecar.write_bytes(blended)
            print(
                f"  wrote {sidecar.name}  "
                f"({len(blended)} bytes; "
                f"{mu_w}x{mu_h} RGBA8)")


# Page count of each fixture — kept as a literal table because
# it's tiny and the alternative (introspect the just-written PDF)
# would re-parse it. Update when adding a fixture.
_PAGE_COUNTS: dict[str, int] = {
    "blank.pdf": 1,
    "red_rect.pdf": 1,
    "stroke_outline.pdf": 1,
    "bezier_fill.pdf": 1,
    "cm_translate.pdf": 1,
    "text_hello.pdf": 1,
    "image_xobject.pdf": 1,
    "two_pages.pdf": 2,
    "even_odd_fill.pdf": 1,
    "bezier_v_y.pdf": 1,
    "fill_stroke_combos.pdf": 1,
    "device_gray.pdf": 1,
    "line_subpath.pdf": 1,
    "text_state.pdf": 1,
    "tj_array_kerning.pdf": 1,
    "image_smask_predictor.pdf": 1,
}


def build_image_smask_predictor() -> None:
    """100x100pt page showing a 4x4 solid-green image whose /SMask is a
    fully-opaque DeviceGray image stored with a TIFF Predictor 2
    (/DecodeParms << /Predictor 2 /Colors 1 /Columns 4 ... >>).

    The mask's post-prediction samples are 0xFF everywhere (alpha =
    opaque). With TIFF horizontal differencing each stored row is
    [0xFF, 0x00, 0x00, 0x00]; a renderer that does NOT reverse the
    predictor reads alpha as opaque only in column 0 and transparent
    in columns 1-3, so the page interior renders white instead of
    green. Discriminator pixel: centre (50,50) is green only when the
    predictor is reversed (regression guard for §7.4.4.4 predictors on
    image / SMask FlateDecode streams)."""
    import zlib
    w = h = 4
    base_raw = bytes([0x00, 0xC0, 0x00]) * (w * h)      # solid green RGB
    base_z = zlib.compress(base_raw)
    # Un-predicted mask row is [0xFF]*4; TIFF-pred-2 stored row is the
    # horizontal differences: [0xFF, 0x00, 0x00, 0x00].
    mask_pred = bytes([0xFF, 0x00, 0x00, 0x00]) * h
    mask_z = zlib.compress(mask_pred)

    cs = b"q\n100 0 0 100 0 0 cm\n/Im0 Do\nQ\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    base_obj = (
        b"<< /Type /XObject /Subtype /Image /Width 4 /Height 4 "
        b"/ColorSpace /DeviceRGB /BitsPerComponent 8 /SMask 6 0 R "
        b"/Filter /FlateDecode /Length " + str(len(base_z)).encode()
        + b" >>\nstream\n" + base_z + b"\nendstream"
    )
    mask_obj = (
        b"<< /Type /XObject /Subtype /Image /Width 4 /Height 4 "
        b"/ColorSpace /DeviceGray /BitsPerComponent 8 "
        b"/Filter /FlateDecode /DecodeParms << /Predictor 2 /Colors 1 "
        b"/Columns 4 /BitsPerComponent 8 >> /Length "
        + str(len(mask_z)).encode()
        + b" >>\nstream\n" + mask_z + b"\nendstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << /XObject << /Im0 5 0 R >> >> >>",
        contents_obj,
        base_obj,
        mask_obj,
    ]
    trailer = b" /Size 7 /Root 1 0 R "
    _write(HERE / "image_smask_predictor.pdf", _build(objects, trailer))


def build_image_indexed() -> None:
    """100x100pt page showing a 2x1 1-bit /Indexed image whose base is a
    [/CalRGB ...] space (palette: index 0 = black, index 1 = white),
    FlateDecode'd with a PNG predictor. Left half = index 0 = black.

    Exercises three independent gaps, each of which drops or garbles the
    image (field reproducer 34026.pdf, a full-page 1-bit Indexed/CalRGB
    scan): (1) the Indexed base must be resolved recursively — a literal
    "/DeviceRGB only" check rejects the CalRGB base and the page stays
    white; (2) sub-byte (1-bit) indices must be unpacked through the
    palette, not routed to the 1=black /ImageMask bilevel path (which
    would invert); (3) the byte-wise PNG predictor stride must be derived
    from the 1-bit sample depth — using an 8-bit stride leaves the
    per-row filter byte in place and shears the image. Discriminator:
    left half (30,50) is black only when all three are handled."""
    import zlib
    # 2x1, 1-bit indices, MSB-first: bits [0, 1] -> 0x40; 1 byte/row.
    # PNG predictor 15, "None" filter (type 0) row: 0x00 prefix + 0x40.
    pred_rows = bytes([0x00, 0x40])
    data_z = zlib.compress(pred_rows)
    img_obj = (
        b"<< /Type /XObject /Subtype /Image /Width 2 /Height 1 "
        b"/ColorSpace [ /Indexed [ /CalRGB << /WhitePoint [0.95 1.0 1.09] "
        b">> ] 1 <000000ffffff> ] /BitsPerComponent 1 "
        b"/Filter /FlateDecode /DecodeParms << /Predictor 15 /Colors 1 "
        b"/Columns 2 /BitsPerComponent 1 >> /Length "
        + str(len(data_z)).encode() + b" >>\nstream\n" + data_z
        + b"\nendstream"
    )
    cs = b"q\n100 0 0 100 0 0 cm\n/Im0 Do\nQ\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << /XObject << /Im0 5 0 R >> >> >>",
        contents_obj,
        img_obj,
    ]
    trailer = b" /Size 6 /Root 1 0 R "
    _write(HERE / "image_indexed.pdf", _build(objects, trailer))


def build_pages_no_type() -> None:
    """100x100pt page whose /Pages tree root carries /Kids but NO /Type
    (and the /Page leaf is otherwise normal), filling a red rect. /Type
    is required on /Pages nodes (§7.7.3.2) but real PDFs frequently omit
    it; the page-tree walk must infer "Pages" from /Kids rather than
    reject the document (field reproducers 32988-1.pdf, 33204-1.pdf,
    33211.pdf opened-failed with 'Page-tree root missing /Type'). A
    renderer that rejects the root renders nothing. Discriminator:
    centre (50,50) is red."""
    cs = b"1 0 0 rg\n25 25 50 50 re\nf\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        # Page-tree root: /Kids but deliberately NO /Type /Pages.
        b"<< /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "pages_no_type.pdf", _build(objects, trailer))


def build_content_lzw_early0() -> None:
    """100x100pt page whose content stream ("0 0 0 rg / 25 25 50 50 re /
    f" → a black 50x50 centre rect) is LZWDecode-compressed with TIFF
    EarlyChange 0 but carries NO /DecodeParms (so the PDF default
    EarlyChange 1 applies). A decoder that only tries EarlyChange 1
    fails ("code out of range") and the page stays blank (field
    reproducer 34203.pdf, whose LZW content streams are EarlyChange 0);
    a lenient decoder retries the other convention. Discriminator:
    centre (50,50) is black. The LZW bytes were produced by
    foundation::lzw::Encode(EarlyChange::Tiff)."""
    lzw = (b"\x80\x0c\x04\x10\x28\x11\xc8\xce\x0a\x19\x0d\x44\x10\x91"
           b"\x00\xd6\x05\x0e\x10\x1c\x8c\xa0\xa3\x30\x2a\x02")
    contents_obj = (
        b"<< /Filter /LZWDecode /Length " + str(len(lzw)).encode()
        + b" >>\nstream\n" + lzw + b"\nendstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "content_lzw_early0.pdf", _build(objects, trailer))


def build_separation_fill() -> None:
    """100x100pt page filling a 50x50 rect via a /Separation colour space
    (alternate /DeviceCMYK, a Type-2 tint transform with C1 = [0 1 1 0] =
    red) at tint 1. The renderer must evaluate the tint transform into the
    alternate space → RGB; one that ignores the /Separation space treats
    `1 scn` as DeviceGray 1.0 and paints the rect white (field reproducer
    33408.pdf, whose PANTONE banner renders grey). Discriminator: centre
    (50,50) is red, not white/grey."""
    cs = b"/CS0 cs\n1 scn\n25 25 50 50 re\nf\n"
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\n"
        b"stream\n" + cs + b"endstream"
    )
    sep_cs = b"[ /Separation /MySpot /DeviceCMYK 6 0 R ]"
    tint_fn = (
        b"<< /FunctionType 2 /Domain [ 0 1 ] /C0 [ 0 0 0 0 ] "
        b"/C1 [ 0 1 1 0 ] /N 1 >>"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << /ColorSpace << /CS0 5 0 R >> >> >>",
        contents_obj,
        sep_cs,
        tint_fn,
    ]
    trailer = b" /Size 7 /Root 1 0 R "
    _write(HERE / "separation_fill.pdf", _build(objects, trailer))


def build_ccitt_decode_pair() -> None:
    """A pair of 100x100pt pages drawing the SAME 32x16 CCITTFaxDecode
    (G4) bilevel image — one without /Decode, one with /Decode [1 0].
    /Decode [1 0] inverts a 1-bpc image's polarity (§8.9.5.2), so the
    two renders must be exact pixelwise inverses; a renderer that
    ignores /Decode on a bilevel base image produces identical (not
    inverted) output and, on a full-page scan with /Decode [1 0],
    floods the page black (field reproducer 35063.pdf). The relative
    (inverse) assertion in the gtest avoids depending on the absolute
    CCITT/BlackIs1 polarity. Requires Pillow (TIFF group4) at authoring
    time only; the .pdf pair is committed."""
    import io
    from PIL import Image, ImageDraw
    w, h = 32, 16
    im = Image.new("1", (w, h), 1)            # white
    ImageDraw.Draw(im).rectangle([0, 0, w // 2 - 1, h - 1], fill=0)  # left half black
    buf = io.BytesIO()
    im.save(buf, format="TIFF", compression="group4")
    data = buf.getvalue()
    tif = Image.open(io.BytesIO(data))
    off = tif.tag_v2[273][0]            # StripOffsets
    cnt = tif.tag_v2[279][0]            # StripByteCounts
    g4 = data[off:off + cnt]            # raw CCITT G4 (== PDF CCITTFaxDecode K<0)

    def make(decode: bool, name: str) -> None:
        img = (
            b"<< /Type /XObject /Subtype /Image /Width 32 /Height 16 "
            b"/ColorSpace /DeviceGray /BitsPerComponent 1 "
            b"/Filter /CCITTFaxDecode /DecodeParms << /K -1 /Columns 32 "
            b"/Rows 16 >> " + (b"/Decode [ 1 0 ] " if decode else b"")
            + b"/Length " + str(len(g4)).encode() + b" >>\nstream\n" + g4
            + b"\nendstream"
        )
        cs = b"q\n100 0 0 100 0 0 cm\n/Im0 Do\nQ\n"
        contents_obj = (
            b"<< /Length " + str(len(cs)).encode() + b" >>\n"
            b"stream\n" + cs + b"endstream"
        )
        objects = [
            b"<< /Type /Catalog /Pages 2 0 R >>",
            b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
            b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
            b"/Contents 4 0 R /Resources << /XObject << /Im0 5 0 R >> >> >>",
            contents_obj,
            img,
        ]
        _write(HERE / name, _build(objects, b" /Size 6 /Root 1 0 R "))

    make(False, "image_ccitt_nodecode.pdf")
    make(True, "image_ccitt_decode.pdf")


def build_content_ascii85_flate() -> None:
    """100x100pt page whose content stream draws a black 40x40 rectangle
    at the centre, stored with a multi-filter [/ASCII85Decode
    /FlateDecode] chain. A renderer that implements only FlateDecode
    decodes the content to nothing and the page stays white (field
    reproducer 33300-2.pdf, whose content streams are all ASCII85 +
    Flate). Discriminator: centre (50,50) is black."""
    import zlib, base64
    cs = b"0 0 0 rg\n30 30 40 40 re\nf\n"
    # Filter order [/ASCII85Decode /FlateDecode] = decode ASCII85 first,
    # then Flate; so the stored bytes are ASCII85(Flate(content)).
    encoded = base64.a85encode(zlib.compress(cs)) + b"~>"
    contents_obj = (
        b"<< /Filter [ /ASCII85Decode /FlateDecode ] /Length "
        + str(len(encoded)).encode() + b" >>\nstream\n" + encoded
        + b"\nendstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "content_ascii85_flate.pdf", _build(objects, trailer))


def build_inline_image() -> None:
    """100x100pt page whose content stream draws a §8.9.7 inline image
    (BI / ID / EI): a 1x1 DeviceRGB red pixel scaled by the CTM to cover
    the whole page. A renderer that drops inline-image samples leaves the
    page white (field reproducer 33433.pdf, a single page-filling inline
    image). Discriminator: centre (50,50) is red. Uses abbreviated keys
    (/W /H /CS /BPC) and the /RGB colour-space abbreviation to exercise
    the renderer's expansion of inline-image abbreviations."""
    # Unit image square mapped to the 100x100 page; one RGB sample = red.
    cs = (
        b"q\n"
        b"100 0 0 100 0 0 cm\n"
        b"BI /W 1 /H 1 /CS /RGB /BPC 8 ID \xff\x00\x00 EI\n"
        b"Q\n"
    )
    contents_obj = (
        b"<< /Length " + str(len(cs)).encode() + b" >>\nstream\n"
        + cs + b"\nendstream"
    )
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] "
        b"/Contents 4 0 R /Resources << >> >>",
        contents_obj,
    ]
    trailer = b" /Size 5 /Root 1 0 R "
    _write(HERE / "inline_image.pdf", _build(objects, trailer))


def main() -> None:
    print(f"Generating page_renderer fixtures into {HERE}/:")
    build_blank()
    build_red_rect()
    build_stroke_outline()
    build_bezier_fill()
    build_cm_translate()
    build_text_hello()
    build_image_xobject()
    build_two_pages()
    build_even_odd_fill()
    build_bezier_v_y()
    build_fill_stroke_combos()
    build_device_gray()
    build_line_subpath()
    build_text_state()
    build_tj_array_kerning()
    build_image_smask_predictor()
    build_image_indexed()
    build_pages_no_type()
    build_content_lzw_early0()
    build_separation_fill()
    build_ccitt_decode_pair()
    build_content_ascii85_flate()
    build_inline_image()
    print(
        f"\nGenerating .pixels sidecars (mutool draw + "
        f"pdftocairo @ {SIDECAR_DPI:g} DPI, blended):")
    for fixture_name, page_count in _PAGE_COUNTS.items():
        pdf = HERE / fixture_name
        if not pdf.is_file():
            raise RuntimeError(
                f"sidecar pass: missing fixture {pdf}")
        _build_sidecars_for(pdf, page_count)


if __name__ == "__main__":
    main()
