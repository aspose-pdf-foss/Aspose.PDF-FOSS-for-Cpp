# Aspose.PDF FOSS for C++

A free, open-source PDF library for modern C++ — read, modify, render, and convert PDF documents with no runtime dependency on any commercial PDF stack.

A strict subset of the public API of the commercial [Aspose.PDF](https://products.aspose.com/pdf/) library.

## Features

- **Read & write PDF** — open existing PDFs, save with full byte-verbatim round-trip, edit `/Info` metadata via incremental update
- **Text extraction** — `Aspose::Pdf::Text::TextAbsorber` walking a `Document` or single `Page`
- **Rendering** — page-to-image via `PngDevice`, `JpegDevice`, `BmpDevice`, `TiffDevice` (single or multi-page); 1bpp / 4bpp / 8bpp / 24bpp TIFF outputs with median-cut palette quantisation
- **Standard14 font fallback** — references to `/Helvetica`, `/Times-Roman`, `/Courier` (×4 styles each, 12 of 14 in v1) without an embedded `/FontFile` paint correctly via the bundled Liberation substitute (SIL OFL 1.1)
- **Pluggable palette quantisation** — implement `IIndexBitmapConverter` and pass it to a `TiffDevice` ctor for caller-controlled palette generation
- **Charset codec** — `foundation::encoding::Encoding` substitutes BCL `System.Text.Encoding` for `TextDevice`'s output (utf-8, utf-16-le, utf-16-be, latin-1, windows-1252)
- **Document metadata** — `DocumentInfo` typed accessors + `Add` / `Remove` / `ClearCustomData`; XMP read via `Metadata`

The C++ surface mirrors the Aspose.PDF API names and shapes.

## Requirements

- C++20 compiler (clang ≥ 16, gcc ≥ 13, MSVC 2022 ≥ 17.5)
- CMake ≥ 3.22
- Python 3 (build-time only — a custom build step runs a generator that embeds the bundled Liberation font outlines into a generated source file)

**Runtime dependencies: none.** The library links against nothing but the C++ standard library — every primitive, including the TIFF / JPEG / PNG codecs, is implemented from scratch within the lib.

**Test-only dependency:** the test suite fetches [GoogleTest](https://github.com/google/googletest) v1.14.0 via CMake `FetchContent` at configure time (requires network access). It is not needed to build or consume the library itself.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Out-of-source build under `build/`. The static library lands at `build/libaspose_pdf_foss.a` (or `.lib` on MSVC).

For a release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

CMakePresets.json carries host-conditional Windows-MSVC presets if you want to use them via `cmake --preset windows-msvc-debug`.

## Quick start

### Open a PDF and iterate pages

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <iostream>

int main() {
    Aspose::Pdf::Document doc("input.pdf");
    auto& pages = doc.Pages();
    std::cout << "Pages: " << pages.Count() << "\n";
    for (int i = 1; i <= static_cast<int>(pages.Count()); ++i) {
        std::cout << "  page " << pages[i].Number() << "\n";
    }
}
```

### Extract text

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/text_absorber.hpp>

Aspose::Pdf::Document doc("input.pdf");
Aspose::Pdf::Text::TextAbsorber absorber;
absorber.Visit(doc);
std::cout << absorber.Text() << "\n";
```

### Render a page to PNG

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>
#include <fstream>

Aspose::Pdf::Document doc("input.pdf");
Aspose::Pdf::Devices::PngDevice png(
    Aspose::Pdf::Devices::Resolution(150));

std::ofstream out("page1.png", std::ios::binary);
png.Process(doc.Pages()[1], out);
```

### Edit metadata + save

```cpp
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

Aspose::Pdf::Document doc("input.pdf");
doc.SetTitle("My Title");
doc.Info().Author("Demo author");
doc.Info().Add("Custom-Key", "Custom value");
doc.Save("output.pdf");  // incremental update — original bytes preserved
```

## Examples

11 runnable examples live in [`examples/`](examples/) — open a PDF, read/write metadata, extract text, render to PNG/TIFF (single + multi-page + 8bpp palettised), demonstrate the device surface. See [`examples/README.md`](examples/README.md) for the full tour.

After `cmake --build build`, the example executables land at `build/examples/01_pages` etc. — one binary per example, runs against the sample PDFs in `pdfs/`.

## Project structure

```
├── include/                   # Public headers
│   ├── aspose/pdf/            # Public-API headers (consumer-facing)
│   └── internal/              # Foundation primitive headers
├── src/
│   ├── public/                # Public-API implementation
│   └── internal/              # Foundation primitives (internal)
├── tests/                     # gtest unit tests
├── examples/                  # 11 runnable demo programs
├── pdfs/                      # Tiny sample PDFs for examples
└── CMakeLists.txt
```

Public-API bodies under `src/public/` call directly into foundation primitives under `src/internal/`. Foundation primitives never reach back into the public API.

## Tests

```bash
cmake --build build
cd build && ctest
```

A gtest suite covering both the public-API surface and the foundation primitives.

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

