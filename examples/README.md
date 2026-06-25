# Examples

Small standalone programs showing how to call into the public
`Aspose::Pdf::` API. Each `.cpp` is self-contained and builds along
with the rest of the project.

A handful of tiny sample PDFs (< 1 KB each) lives alongside in
`pdfs/` so the examples run out of the box — no fixtures to
fetch from elsewhere.

## Build

The examples are wired into the top-level CMake build:

```bash
cmake -S . -B build
cmake --build build
```

After build, executables land at `build/examples/<name>`.

## Run (in numeric order — each example builds on the previous)

```bash
cd build/examples
SAMPLES=../../pdfs

# 1. Open a PDF, count its pages, iterate via the 1-based
#    Pages()[i] indexer and print each Page::Number().
./01_pages "$SAMPLES/two_pages.pdf"

# 2. Read the /Info dictionary (Title, Author, …).
./02_read_metadata "$SAMPLES/with_info.pdf"

# 3. Edit metadata (SetTitle + Info().Author/Creator/Add) + Save.
#    The lib appends an incremental update; original bytes preserved.
./03_write_metadata "$SAMPLES/with_info.pdf" /tmp/with_my_metadata.pdf

# 4. Read back what 03 wrote — our edits round-trip, original
#    fields the setters didn't touch are preserved verbatim.
./02_read_metadata /tmp/with_my_metadata.pdf

# 5. Extract every word of text via TextAbsorber::Visit(Document).
./04_text_extraction "$SAMPLES/hello_world.pdf"
./04_text_extraction "$SAMPLES/two_lines.pdf"

# 6. Save with no edits — output is byte-identical to the input.
./05_save_roundtrip "$SAMPLES/two_pages.pdf" /tmp/copy.pdf

# 7. Construct rendering devices + round-trip their properties.
./06_devices_surface

# 8. Rasterise page 1 of two_pages.pdf to PNG (the end-to-end
#    rendering pipeline through PngDevice::Process(Page, ostream)).
./07_render_png "$SAMPLES/two_pages.pdf" 1 /tmp/page1.png

# 9. Rasterise every page of two_pages.pdf into a single multi-page
#    TIFF via TiffDevice::Process(Document, fromPage, toPage, ostream).
./08_render_tiff "$SAMPLES/two_pages.pdf" /tmp/all_pages.tiff

# 10. Same fixture, 8-bit palettised TIFF (256-colour palette via
#     median-cut quantisation through TiffSettings.Depth).
./09_indexed_tiff "$SAMPLES/two_pages.pdf" 1 /tmp/page1_8bpp.tiff

# 11. Render a text-bearing PDF to PNG. text_hello.pdf carries an
#     embedded TrueType font, so its glyphs paint correctly through
#     the foundation::page_renderer raster path.
./10_render_text_pdf "$SAMPLES/text_hello.pdf" /tmp/text.png

# 12. Render a Standard14-only PDF — hello_world.pdf references
#     `/BaseFont /Helvetica` with NO embedded font program. The
#     foundation::standard14_outlines fallback supplies bytes from
#     system Helvetica (macOS), system Arial (Windows), or the
#     bundled Liberation Sans substitute (anywhere — covers
#     fontless Linux containers, CI, etc.).
./11_render_standard14 "$SAMPLES/hello_world.pdf" /tmp/std14.png

# 13. Build a 10-page "Feature Showcase" PDF entirely FROM SCRATCH —
#     the mirror image of every example above (which all open an
#     existing file). One program exercising the whole creation
#     surface: positioned + wrapped text, a Standard-14 font grid, a
#     watermark, the bundled pdflib.png, AcroForm fields, the annotation
#     gallery, tables (with a column-spanning row), vector graphics,
#     a bar chart, form flattening and a clickable bookmark outline.
#     Takes only an OUTPUT path (no input PDF).
./12_create_features /tmp/showcase.pdf
# …then look at it with the read/render examples above:
./01_pages /tmp/showcase.pdf            # -> 10 pages
./04_text_extraction /tmp/showcase.pdf  # -> all section text
./07_render_png /tmp/showcase.pdf 4 /tmp/showcase_p4.png   # the embedded pdflib.png
```

## What each example shows

| File | API surfaces | Sample input |
|---|---|---|
| `01_pages.cpp` | `Document(filename)`, `Pages().Count()`, `Pages()[i]` (1-based), `Page::Number()` | any |
| `02_read_metadata.cpp` | `Document::Info()` accessor + 6 read-only `DocumentInfo` properties | `with_info.pdf` (or any) |
| `03_write_metadata.cpp` | `Document::SetTitle`, `DocumentInfo` setters, `DocumentInfo::Add`, `Save` (incremental update) | `with_info.pdf` |
| `04_text_extraction.cpp` | `Text::TextAbsorber`, `Visit(Document)`, `Text()`, `HasErrors()` | `hello_world.pdf`, `two_lines.pdf` |
| `05_save_roundtrip.cpp` | `Document::Save` byte-verbatim path | any |
| `06_devices_surface.cpp` | `Devices::PngDevice` / `JpegDevice` / `BmpDevice` / `TextDevice` ctors + properties; `Text::TextExtractionOptions` | none (no input) |
| `07_render_png.cpp` | `Document::Pages()[N]` indexer + `PngDevice::Process(Page, std::ostream&)` end-to-end | any |
| `08_render_tiff.cpp` | `TiffDevice::Process(Document, fromPage, toPage, std::ostream&)` — multi-page TIFF | any |
| `09_indexed_tiff.cpp` | `TiffSettings::Depth(ColorDepth::Format8bpp)` + `TiffDevice::Process(Page, std::ostream&)` — palettised TIFF (median-cut palette) | any |
| `10_render_text_pdf.cpp` | `PngDevice::Process(Page, std::ostream&)` end-to-end on a text-bearing page (embedded TrueType font) | `text_hello.pdf` |
| `11_render_standard14.cpp` | Same Process, but on a PDF that references `/Helvetica` without an embedded font program — exercises the `foundation::standard14_outlines` fallback (system font → bundled Liberation) | `hello_world.pdf` |
| `12_create_features.cpp` | **From-scratch creation:** `Document` + `Pages().Add()` + `Save`; `Text::TextBuilder` (`AppendText` / `AppendParagraph`), `FontRepository::FindFont`; `Page::AddImage` (bundled `pdflib.png`); `Table` / `Row` / `Cell` (`ColSpan`); `Drawing::Graph` (`Line`/`Rectangle`/`Circle`/`Ellipse`); `WatermarkArtifact`; the annotation gallery (`Highlight`/`Underline`/`Squiggly`/`StrikeOut`/`Square`/`Circle`/`Line`/`Ink`/`Text`/`FreeText`/`Stamp`/`Link`/`FileAttachment`); AcroForm fields (`TextBox`/`Checkbox`/`RadioButton`/`ComboBox`/`ListBox`) + `Form::Flatten`; `Outlines` bookmarks (`XYZExplicitDestination`) | none (output only) |

## Notes

* **Example 03 requires** the input PDF to already have an `/Info`
  dictionary in its trailer — the incremental-update writer patches
  an existing `/Info` object rather than synthesising a new one.
  `with_info.pdf` is bundled for this reason. Inputs without `/Info`
  cause `Save()` to throw `std::runtime_error`.

* **Example 12 is the only *creation* example** — every other one
  opens an existing PDF; 12 builds one from nothing and is the
  natural counterpart to read with 01 / 04 / 07. It sets colours and
  text decorations through the API for completeness, but the bundled
  rasteriser paints monochrome line-art + black text + embedded
  images + watermark, so the rendered result is black-on-white (open
  the output in any viewer to see the full interactive structure:
  form fields, annotations, bookmarks). It deliberately does **not**
  set `Document::Info()` — a from-scratch document has no `/Info`
  object for the incremental-update writer to patch (see the
  Example 03 note above), so doing so would throw at `Save()`.

* **Example 06 doesn't drive a device end-to-end** — it only
  constructs them and round-trips properties. Examples 07 and 08
  are the end-to-end rendering ones: `Document → Pages()[N] →
  PngDevice::Process → PNG bytes` (single page) and
  `Document → TiffDevice::Process(Document, from, to, …) → multi-
  page TIFF` (page range).

* **Standard14 fonts render via fallback** — PDFs that reference
  Helvetica / Times-Roman / Courier (× 4 styles each — 12 of the
  14 in v1 scope) without embedding font data now paint correctly.
  `foundation::standard14_outlines` probes per-OS default font
  directories first (`/System/Library/Fonts`, `/usr/share/fonts`,
  `%WINDIR%\Fonts`, etc. for system Helvetica / Arial / etc.) and
  falls back to bundled Liberation Sans / Serif / Mono — explicit
  metric-equivalent substitutes shipped under SIL OFL 1.1, ~4 MB
  compiled into the static lib. Symbol and ZapfDingbats are
  deferred (no permissively-licensed metric-equivalent substitute
  located yet).

* **Caller-pluggable palette quantisation** — TiffDevice has 5
  ctor overloads that take an `Aspose::Pdf::IIndexBitmapConverter&`
  for callers who want their own palette-generation algorithm
  (corporate brand palette, faster but lossier quantiser, etc.).
  Subclass `IIndexBitmapConverter` (`Get1Bpp` / `Get4Bpp` / `Get8Bpp`
  returning `Aspose::Pdf::Drawing::Bitmap`) and pass an instance to
  one of those ctors. See
  `tests/i_index_bitmap_converter_smoke_test.cpp` for a runnable
  custom-converter example. Default median-cut (used by example 09)
  is the no-converter path.

* The examples are intentionally small + comment-light — the `.cpp`
  source itself is the documentation. Read them top to bottom in
  numeric order for a tour of the public API.

## Sample PDFs

Tiny synthetic fixtures generated by the project's own test
infrastructure. Each is hand-written PDF source — no glyphs from
licensed fonts, no compressed streams.

| File | Size | What it carries |
|---|---|---|
| `two_pages.pdf` | 687 B | A two-page document, no metadata, no text |
| `with_info.pdf` | 688 B | One blank page + a populated `/Info` dictionary (Title, Author, Producer, Keywords) |
| `hello_world.pdf` | 592 B | One page rendering "Hello World" via Helvetica-equivalent encoding |
| `two_lines.pdf` | 630 B | One page with two text lines on separate Tj operators |
| `text_hello.pdf` | 2.5 KB | One page with a 442-byte embedded TrueType font; glyphs render correctly via `PngDevice::Process` |

If you want richer inputs (real-world PDFs with images, fonts,
encryption, …), drop them in any directory and pass the path as
argv[1]. The examples don't depend on the bundled samples.
