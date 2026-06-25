# Aspose.PDF FOSS for C++

[![CI](https://github.com/aspose-pdf-foss/Aspose.PDF-FOSS-for-Cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/aspose-pdf-foss/Aspose.PDF-FOSS-for-Cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.22+-064F8C?logo=cmake)](https://cmake.org/)

A free, open-source PDF library for modern C++ — open and save PDFs, extract text, render pages to raster images, build documents from scratch (text, images, tables, vector graphics, annotations, AcroForm fields, bookmarks), encrypt with RC4-128 / AES-128 / AES-256, and digitally sign with a detached PKCS#7 signature. **No runtime dependency on any commercial PDF stack — the library links against nothing but the C++ standard library; every primitive, including the TIFF / JPEG / PNG codecs and the rasteriser, is implemented from scratch.**

The public API is a strict subset of the public API of the commercial [Aspose.PDF for .NET](https://products.aspose.com/pdf/) library — class names, method names, and shapes mirror it where natural for migrants. Spec references throughout follow ISO 32000-1 (PDF 1.7) and ISO 32000-2 (PDF 2.0).

## Install

The library builds as a static library you link into your project. Add it as a subdirectory of your CMake build, or build and install it standalone (see [Build](#build)).

```cmake
add_subdirectory(aspose.pdf-foss-for-cpp)
target_link_libraries(your_app PRIVATE aspose_pdf_foss)
```

### Requirements

- C++20 compiler (clang ≥ 16, gcc ≥ 13, MSVC 2022 ≥ 17.5)
- CMake ≥ 3.22
- Python 3 (build-time only — a generator step embeds the bundled font outlines into a generated source file)

**Runtime dependencies: none.** The test suite alone fetches [GoogleTest](https://github.com/google/googletest) v1.14.0 via CMake `FetchContent` at configure time; it is not needed to build or consume the library itself.

## Quick Start

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>
#include <fstream>
#include <iostream>

int main() {
    // Open a PDF and count its pages
    Aspose::Pdf::Document doc("input.pdf");
    std::cout << "Pages: " << doc.Pages().Count() << "\n";

    // Extract text
    Aspose::Pdf::Text::TextAbsorber absorber;
    absorber.Visit(doc);
    std::cout << absorber.Text() << "\n";

    // Render page 1 to PNG at 150 DPI
    Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(150));
    std::ofstream out("page1.png", std::ios::binary);
    png.Process(doc.Pages()[1], out);
}
```

See [`examples/`](examples/) for 12 runnable programs covering the whole surface, including a from-scratch [feature showcase](examples/12_create_features.cpp).

## Features

- **Open & save** — open existing PDFs; save with byte-verbatim round-trip, or edit `/Info` metadata via an incremental update that preserves the original bytes
- **Open encrypted PDFs** — supply the user or owner password to `Document(path, password)`; `IsEncrypted()` reports the state
- **Text extraction** — `Text::TextAbsorber` walks a whole `Document` or a single `Page`; `Text::TextFragmentAbsorber` returns positioned fragments with font, size, and colour
- **Render to image** — rasterise pages through `PngDevice`, `JpegDevice`, `BmpDevice`, and `TiffDevice` (single page or multi-page), driven by a dependency-free anti-aliased renderer. TIFF supports 1/4/8/24-bpp output with median-cut palette quantisation
- **Pluggable palette quantisation** — implement `IIndexBitmapConverter` (`Get1Bpp`/`Get4Bpp`/`Get8Bpp`) and pass it to a `TiffDevice` ctor for caller-controlled palette generation; the default is median-cut
- **Standard-14 font fallback** — references to `/Helvetica`, `/Times-Roman`, `/Courier` (× 4 styles each — 12 of the 14 in v1) without an embedded `/FontFile` paint correctly via system fonts, falling back to the bundled Liberation substitutes (SIL OFL 1.1) so glyphs render even in fontless Linux/CI containers
- **Document metadata** — typed `DocumentInfo` accessors (title, author, subject, keywords, creator, producer, dates) plus `Add` / `Remove` / `ClearCustomData` for arbitrary entries; XMP read via `Metadata`
- **Charset codec** — `Encoding` substitutes the BCL `System.Text.Encoding` for `TextDevice` output (utf-8, utf-16-le, utf-16-be, latin-1, windows-1252)
- **Encryption** — `Document::Encrypt(user, owner, permissions, algorithm)` with RC4-128, AES-128, or AES-256 (PDF 2.0, R=6); granular viewer permissions (print, modify, extract, annotate, fill form, accessibility, assemble, high-res print); applied on `Save`. Re-open with the password to round-trip
- **Digital signatures** — `Facades::PdfFileSignature` adds a detached PKCS#7 (`adbe.pkcs7.detached`) signature with a byte-exact `/ByteRange`; output verifies under the OpenSSL CLI
- **Create from scratch** — build documents with positioned and word-wrapped text (`Text::TextBuilder` / `TextFragment` / `TextParagraph`), font lookup (`FontRepository::FindFont`), image placement (`Page::AddImage`, PNG/JPEG), tables, vector graphics, watermarks, annotations, AcroForm fields, and bookmark outlines
- **Tables** — `Table` / `Row` / `Cell` with `ColumnWidths`, per-cell and default borders (`BorderInfo`), background colours, and column-spanning cells (`Cell::ColSpan`); positioned on a page via `Page::Paragraphs().Add(table)`
- **Vector graphics** — `Drawing::Graph` holds `Line`, `Rectangle`, `Circle`, and `Ellipse` shapes drawn directly onto a page
- **Watermarks** — `WatermarkArtifact` overlays or underlays rotated, semi-transparent text on a page via `Page::Artifacts()`
- **Annotations** — a gallery of subtypes: `Highlight`, `Underline`, `Squiggly`, `StrikeOut`, `Square`, `Circle`, `Line`, `Ink`, `Text` (sticky note), `FreeText`, `Stamp`, `Link` (with `GoTo` / `GoToURI` actions), and `FileAttachment`. Each gets a pre-generated `/AP` appearance stream so it renders in any spec-conforming viewer
- **Forms (AcroForm)** — build `TextBox`, `Checkbox`, `RadioButton`, `ComboBox`, `ListBox`, and push-button (`ButtonField`) fields with pre-generated `/AP` appearances; `Form::Flatten()` bakes every field into static page content
- **Outlines (bookmarks)** — build a hierarchical `OutlineItemCollection` tree with bold/italic styling and `XYZExplicitDestination` targets
- **Named destinations & embedded files** — define reusable destinations; attach companion files document-level via `Document::EmbeddedFiles()` (shown in the viewer's attachment panel)
- **Aspose.Pdf.Facades** — the classic facade API surface, with these wired to the real engine: `PdfConverter` (rasterise a PDF to multi-page TIFF or per-page PNG), `PdfExtractor` (text extraction honouring `StartPage`/`EndPage`), `PdfFileSecurity` (encrypt / decrypt / change passwords), `PdfFileSignature` (sign), and `PdfBookmarkEditor` (create / extract bookmarks)

> **Scope note.** This is a v1 release and a deliberate *subset* of Aspose.PDF for .NET. Some facade methods (e.g. `PdfFileEditor` concatenate/split, `PdfFileStamp` header/footer stamping) ship their public surface but are not yet wired to the engine; the feature list above reflects what is exercised by the examples and the test suite.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Out-of-source build under `build/`. The static library lands at `build/libaspose_pdf_foss.a` (or `aspose_pdf_foss.lib` on MSVC).

Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`CMakePresets.json` carries host-conditional Windows-MSVC presets — e.g. `cmake --preset windows-msvc-debug`. Sources with non-ASCII bytes need `/utf-8` under MSVC.

## API Reference

### Opening documents

```cpp
#include <aspose/pdf/document.hpp>

// From a file path
Aspose::Pdf::Document doc("input.pdf");

// Encrypted files: pass the user or owner password
Aspose::Pdf::Document locked("locked.pdf", "secret");
if (locked.IsEncrypted()) { /* ... */ }
```

### Pages

```cpp
#include <aspose/pdf/page_collection.hpp>

auto& pages = doc.Pages();                       // 1-based collection
std::cout << pages.Count() << " pages\n";
for (int i = 1; i <= static_cast<int>(pages.Count()); ++i) {
    std::cout << "page " << pages[i].Number() << "\n";
}

// Build a new page and size it (A4, in PDF points)
Aspose::Pdf::Page& p = doc.Pages().Add();
p.SetPageSize(595.0, 842.0);
```

### Text extraction

```cpp
#include <aspose/pdf/text_absorber.hpp>

Aspose::Pdf::Text::TextAbsorber absorber;
absorber.Visit(doc);              // whole document; or Visit(page) for one page
std::cout << absorber.Text() << "\n";
```

### Rendering to images

```cpp
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/resolution.hpp>
#include <fstream>

// Single page → PNG
Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(150));
std::ofstream pngOut("page1.png", std::ios::binary);
png.Process(doc.Pages()[1], pngOut);

// Every page → one multi-page TIFF
Aspose::Pdf::Devices::TiffDevice tiff(Aspose::Pdf::Devices::Resolution(150));
std::ofstream tiffOut("all_pages.tiff", std::ios::binary);
tiff.Process(doc, 1, static_cast<int>(doc.Pages().Count()), tiffOut);

// 8-bpp palettised TIFF (median-cut palette)
Aspose::Pdf::Devices::TiffSettings settings;
settings.Depth(Aspose::Pdf::Devices::ColorDepth::Format8bpp);
Aspose::Pdf::Devices::TiffDevice indexed(
    Aspose::Pdf::Devices::Resolution(150), settings);
```

For caller-controlled palettes, subclass `Aspose::Pdf::IIndexBitmapConverter` and pass it to a `TiffDevice` ctor.

### Metadata

```cpp
#include <aspose/pdf/document_info.hpp>

// Read
Aspose::Pdf::Document doc("input.pdf");
std::cout << doc.Info().Title() << " / " << doc.Info().Author() << "\n";

// Write + save (incremental update — original bytes preserved)
doc.SetTitle("My Title");
doc.Info().Author("Demo author");
doc.Info().Add("Custom-Key", "Custom value");
doc.Save("output.pdf");
```

> The incremental-update writer patches an existing `/Info` object; a from-scratch document has no `/Info` to patch, so setting metadata on one and saving throws.

### Encryption

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/permissions.hpp>

using Aspose::Pdf::CryptoAlgorithm;
using Aspose::Pdf::Permissions;

Aspose::Pdf::Document doc("input.pdf");
doc.Encrypt("userpass", "ownerpass",
            Permissions::PrintDocument | Permissions::FillForm,
            CryptoAlgorithm::AESx128);
doc.Save("encrypted.pdf");                   // encryption applied on Save

// AES-256, PDF 2.0 (R=6): pass usePdf20 = true
doc.Encrypt("userpass", "ownerpass",
            Permissions::PrintDocument,
            CryptoAlgorithm::AESx256, /*usePdf20=*/true);

// Re-open with the password
Aspose::Pdf::Document reopened("encrypted.pdf", "userpass");
```

`CryptoAlgorithm` covers `RC4x40`, `RC4x128`, `AESx128`, and `AESx256`. `Permissions` is a `[Flags]` enum (ISO 32000-1 §7.6.3.2 Table 22) — compose with `|`. Permissions are enforced by viewers; the library is not a DRM mechanism.

### Digital signatures

```cpp
#include <aspose/pdf/facades/pdf_file_signature.hpp>

Aspose::Pdf::Facades::PdfFileSignature sig{"input.pdf", "signed.pdf"};
// configure the signature (certificate + key), then sign and save —
// a detached PKCS#7 (adbe.pkcs7.detached) signature with a byte-exact
// /ByteRange. Output verifies under the OpenSSL CLI.
```

See [`tests/facades_pdf_file_signature_smoke_test.cpp`](tests/facades_pdf_file_signature_smoke_test.cpp) for the full signing flow.

### Building documents from scratch

The `12_create_features` example builds a 10-page showcase exercising the whole creation surface. Highlights:

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/position.hpp>

namespace pdf = Aspose::Pdf;
namespace txt = Aspose::Pdf::Text;

pdf::Document doc;
pdf::Page& page = doc.Pages().Add();
page.SetPageSize(595.0, 842.0);

// Positioned text
txt::TextFragment frag("Hello from Aspose.PDF FOSS for C++");
frag.TextState().Font(txt::FontRepository::FindFont("Helvetica"));
frag.TextState().FontSize(18.0f);
frag.Position(txt::Position(60.0, 760.0));
txt::TextBuilder{page}.AppendText(frag);

doc.Save("scratch.pdf");
```

**Images** — `page.AddImage(pngBytes, Rectangle(...))` accepts PNG or JPEG byte streams.

**Tables** — `Table` / `Row` / `Cell` with column-spanning rows:

```cpp
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/border_info.hpp>

pdf::Table table;                                    // keep alive until doc.Save()
table.ColumnWidths("250 120 120");
table.Border(pdf::BorderInfo(pdf::BorderSide::All, 0.5f));
pdf::Row& header = table.Rows().Add();
header.BackgroundColor(pdf::Color::LightGray());
header.Cells().Add("Item"); header.Cells().Add("Qty"); header.Cells().Add("Price");
pdf::Row& total = table.Rows().Add();
total.Cells().Add("Total").ColSpan(2);               // span the first two columns
total.Cells().Add("$41.00");
page.Paragraphs().Add(table);                        // stored by reference
```

**Vector graphics** — `Drawing::Graph` holding `Line` / `Rectangle` / `Circle` / `Ellipse`:

```cpp
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/circle.hpp>

namespace draw = Aspose::Pdf::Drawing;
draw::Graph g(495.0, 420.0);                          // keep alive until doc.Save()
g.Left(40.0); g.Top(40.0);
g.Shapes().push_back(std::make_unique<draw::Circle>(180.0f, 500.0f, 70.0f));
page.Paragraphs().Add(g);
```

**Annotations** — pre-generated `/AP` appearances; render in any viewer:

```cpp
#include <aspose/pdf/annotations/highlight_annotation.hpp>

namespace ann = Aspose::Pdf::Annotations;
ann::HighlightAnnotation a(page, pdf::Rectangle(60, 700, 210, 716, true));  // keep alive until Save()
a.Color(pdf::Color::FromRgb(1.0, 0.95, 0.0));
a.Contents("Highlight");
page.Annotations().Add(a);
```

Available subtypes: `Highlight`, `Underline`, `Squiggly`, `StrikeOut`, `Square`, `Circle`, `Line`, `Ink`, `Text`, `FreeText`, `Stamp`, `Link` (with `GoToAction` / `GoToURIAction`), `FileAttachment`.

**Forms (AcroForm)** — fields with pre-generated appearances + `Form::Flatten()`:

```cpp
#include <aspose/pdf/forms/text_box_field.hpp>

namespace frm = Aspose::Pdf::Forms;
frm::TextBoxField name(doc, pdf::Rectangle(190, 700, 470, 720, true));  // keep alive until Save()
name.PartialName("fullName");
name.Value("Alice Sample");
doc.Form().Add(name, /*pageNumber=*/1);
// doc.Form().Flatten();   // bake every field into static page content
```

Field types: `TextBoxField`, `CheckboxField`, `RadioButtonField`, `ComboBoxField`, `ListBoxField`, `ButtonField`.

**Outlines (bookmarks)**:

```cpp
#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>

auto& root = doc.Outlines();
pdf::OutlineItemCollection item(root);               // keep alive until Save()
item.Title("Chapter 1");
item.Bold(true);
item.Destination(ann::XYZExplicitDestination(page, 0.0, 842.0, 0.0));
root.Add(item);
```

> **Lifetime rule for creation.** `Paragraphs`, `Annotations`, `Form`, `Artifacts`, and `Outlines` store the object **by reference**, so every table, graph, annotation, field, watermark, and outline item must outlive `Save()`. The example parks them in an arena of `shared_ptr`s; see [`12_create_features.cpp`](examples/12_create_features.cpp).

### Facades (Aspose.Pdf.Facades)

```cpp
#include <aspose/pdf/facades/pdf_converter.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>

// Rasterise a PDF to a multi-page TIFF (or per-page PNG via GetNextImage)
Aspose::Pdf::Facades::PdfConverter conv;
// ... bind the document, then SaveAsTIFF(...) / HasNextImage() / GetNextImage(...)

// Extract text honouring StartPage / EndPage
Aspose::Pdf::Facades::PdfExtractor ex;
ex.StartPage(1);
```

## Examples

12 runnable examples live in [`examples/`](examples/). After `cmake --build build`, executables land at `build/examples/<name>`. Run them in numeric order — each builds on the previous; see [`examples/README.md`](examples/README.md) for the full tour.

| Example | Shows |
|---|---|
| `01_pages` | open a PDF, count pages, iterate the 1-based indexer |
| `02_read_metadata` / `03_write_metadata` | read `DocumentInfo`; edit + incremental-update save |
| `04_text_extraction` | `TextAbsorber::Visit(Document)` |
| `05_save_roundtrip` | byte-verbatim save |
| `06_devices_surface` | construct + round-trip device properties |
| `07_render_png` | end-to-end `PngDevice::Process(Page, ostream)` |
| `08_render_tiff` | multi-page TIFF via `TiffDevice::Process(Document, …)` |
| `09_indexed_tiff` | 8-bpp palettised TIFF (median-cut) |
| `10_render_text_pdf` | render a page with an embedded TrueType font |
| `11_render_standard14` | render `/Helvetica` with no embedded font (fallback path) |
| `12_create_features` | **from-scratch** 10-page showcase: text, image, tables, graphics, annotations, AcroForm fields, bookmarks |

## Project structure

```
├── include/
│   ├── aspose/pdf/            # Public-API headers (consumer-facing)
│   └── internal/              # Foundation primitive headers
├── src/
│   ├── public/                # Public-API implementation
│   └── internal/              # Foundation primitives (codecs, renderer, crypto)
├── tests/                     # GoogleTest unit + smoke tests
├── examples/                  # 12 runnable demo programs
├── pdfs/                      # Tiny sample PDFs for the examples
└── CMakeLists.txt
```

Public-API bodies under `src/public/` call directly into foundation primitives under `src/internal/`. Foundation primitives never reach back into the public API.

## Tests

```bash
cmake --build build
cd build && ctest
```

A GoogleTest suite covers both the public-API surface and the foundation primitives. GoogleTest is fetched at configure time and is the only test-time dependency.

## License

This project is licensed under the [MIT License](LICENSE).

It bundles metric-compatible **Liberation** fonts (Sans / Serif / Mono), compiled into the static library to render the PDF Standard-14 fonts when no embedded or system font is available. These are licensed under the SIL Open Font License 1.1, which permits bundling with software under any license — see [`src/internal/standard14_outlines.fonts/OFL.txt`](src/internal/standard14_outlines.fonts/OFL.txt). The MIT license covers the library's own code (SPDX: `MIT AND OFL-1.1` for the distribution as a whole).
