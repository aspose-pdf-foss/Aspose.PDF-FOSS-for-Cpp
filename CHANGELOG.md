# Changelog

All notable changes to Aspose.PDF FOSS for C++ are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The public API is the set of
headers installed under `include/aspose/` plus `include/aspose.pdf.foss.hpp`; anything under
`include/internal/` is an implementation detail and may change in any release.

## [Unreleased]

### Added

- `PageCollection::Add(const Page&)` and `PageCollection::Insert(int, const Page&)` accept a page
  from another document: the page and the objects it reaches are deep-imported. Annotations and
  unsaved edits on the source page are not carried over.
- `Annotation::Flatten()` removes the annotation from its page's collection; on `Save()` its
  appearance is drawn into the page content and the annotation (with its popup) is removed. An
  annotation without an appearance is simply removed.
- `Field::Flatten()` flattens a single form field: its widgets are drawn into their pages and the
  field is removed from the form, while the other fields stay interactive.
- `PdfAnnotationEditor::FlatteningAnnotations()` flattens every annotation and form field of the
  document; the `FlattenSettings` overload honours `HideButtons`; the `(start, end, types)` overload
  flattens only the given annotation types on the given pages (`Widget` includes form fields).
  Annotations the library does not model, such as FreeText or Ink, are covered too; one without an
  appearance is left in place.

### Changed

- `Forms::Signature::Verify()` now throws `std::logic_error` instead of always returning `true`.
  The library has no signature verification yet, so the old result reported unchecked signatures
  as valid.
- `PdfFileSignature::VerifySignature` and `VerifySigned` throw `std::logic_error` for a present
  signature instead of returning `true` when it merely existed and covered the whole file. They
  still return `false` for an unknown name or a blank signature field.

### Fixed

- `PdfFileInfo::GetPageWidth`, `GetPageHeight`, `GetPageRotation`, `GetPageXOffset`, and
  `GetPageYOffset`, and `PdfPageEditor::GetPageSize` and `GetPageRotation` now return the page's
  real media box and rotation instead of fixed US-Letter values. An out-of-range page number throws
  `std::out_of_range`.
- `PdfFileInfo(path, password)` now opens encrypted files with the given password; a wrong
  password throws `std::runtime_error`. Previously the password was ignored.
- `Form::Flatten()` no longer loses field values. On `Save()` each visible widget's appearance is
  drawn into its page content, the widgets are removed from the page, and `/AcroForm` is removed
  from the catalog. Fields loaded from the file keep their own appearance unless their value was
  changed. Previously the fields and their values were simply dropped.
- Changing the value of a field loaded from a file now also regenerates its appearance (`/AP`),
  so viewers show the new value instead of the old one. Check boxes keep the name of their own
  "on" state instead of always using `/Yes`.
- Adding or deleting an annotation on a page of a loaded document no longer drops the page's other
  annotations that the annotation collection does not model, such as form field widgets, popups
  and FreeText/Ink/Line/Polygon annotations. Previously the form fields on that page disappeared.
- A page whose `/Annots` is an indirect reference to an array (`/Annots 4 0 R`) is now handled
  like an inline array: its annotations are loaded, and adding an annotation, adding a form field
  or signing keeps them. Previously they were not loaded and were dropped on save.

## [1.0.1] - 2026-09-29

Packaging-only release; the library code and public API are unchanged from 1.0.0.

### Added

- NuGet package `Aspose.PDF.Cpp.FOSS` for Visual Studio C++ projects: static libraries for x64,
  x86, and ARM64 (Release `/MD` and Debug `/MDd`). Its MSBuild `.targets` adds the include path,
  links the matching library, and raises the project to C++20; it warns when a project uses the
  static CRT. The release workflow builds the package, verifies it with a Visual Studio consumer
  project, attaches it to the GitHub release, and publishes it to nuget.org after manual approval.

### Fixed

- The Debug library in the Windows release archive now embeds its debug information (`/Z7`), so
  linking it no longer produces LNK4099 "PDB not found" warnings.

## [1.0.0] - 2026-09-29

First public release: a free, open-source, dependency-free C++20 library for reading, rendering,
editing, and creating PDF documents, with a public API that is a strict subset of Aspose.PDF for
.NET.

### Added

#### Documents and pages

- `Document` — open existing PDFs (`Document(path)`, `Document(path, password)` for encrypted files,
  `Document(path, LoadOptions)`), create documents from scratch (`Document()`), and save them with
  `Save(path)`; byte-verbatim round-trips and an incremental `/Info` update that preserves the
  original bytes.
- SVG import: `Document(path, SvgLoadOptions)` converts an SVG file to a single-page PDF (v1
  conversion subset).
- `PageCollection` with a 1-based indexer, `Add`, `Insert`, `Delete`, and `Count`; `Page` with page
  size, paragraphs, annotations, artifacts, and `AddImage`.
- `DocumentInfo` typed metadata accessors (title, author, subject, keywords, creator, producer,
  dates) plus `Add`/`Remove`/`ClearCustomData`; read access to XMP metadata via `Metadata` and
  `XmpValue`.
- Page labels (`PageLabel`, `PageLabelCollection`), embedded files (`Document::EmbeddedFiles()`,
  `FileSpecification`), outlines (`OutlineCollection`, `OutlineItemCollection`) and named
  destinations (`NamedDestinationCollection`, explicit destination subtypes).

#### Text

- Text extraction with `Text::TextAbsorber` (whole document or single page) and
  `Text::TextFragmentAbsorber` (positioned fragments with font, size, and colour).
- Text authoring with `Text::TextBuilder`, `TextFragment`, `TextParagraph`, `TextState`, and
  `FontRepository::FindFont`.
- `Text::Encoding` — a `System.Text.Encoding`-style charset codec (UTF-8, UTF-16LE, UTF-16BE,
  Latin-1, Windows-1252) used by `TextDevice`.
- Standard-14 fonts without an embedded `/FontFile` render via system fonts, falling back to
  bundled Liberation substitutes (SIL OFL 1.1) for Helvetica, Times-Roman, and Courier.

#### Rendering

- Anti-aliased page rasteriser with no third-party dependency, exposed through `PngDevice`,
  `JpegDevice`, `BmpDevice`, and `TiffDevice` (single page, or a multi-page document range in one
  call).
- `TiffDevice` 1/4/8/24-bpp output with median-cut palette quantisation, or a caller-supplied
  `IIndexBitmapConverter`; `TiffSettings`, `Resolution`, and `RenderingOptions`.
- Built-in codecs implemented from scratch: Flate, LZW, RunLength, ASCII85, ASCIIHex, CCITT, DCT
  (JPEG), JBIG2, JPX (JPEG 2000), PNG, TIFF, and BMP; inline images (`BI`/`ID`/`EI`).

#### Content creation

- Tables (`Table`, `Row`, `Cell`) with column widths, borders, background colours, and column
  spans.
- Vector graphics via `Drawing::Graph` with `Line`, `Rectangle`, `Circle`, and `Ellipse`.
- Rotated, semi-transparent overlays via `WatermarkArtifact`; `FloatingBox`, `Hyperlink`.

#### Annotations and forms

- Annotations with pre-generated `/AP` appearance streams: `Highlight`, `Underline`, `Squiggly`,
  `StrikeOut`, `Square`, `Circle`, `Line`, `Ink`, `Text`, `FreeText`, `Stamp`, `Link` (with
  `GoToAction`/`GoToURIAction`), and `FileAttachment`.
- AcroForm fields — `TextBoxField`, `CheckboxField`, `RadioButtonField`, `ComboBoxField`,
  `ListBoxField`, `ButtonField` — through `Document::Form()`, with `Form::Flatten()`.

#### Security and signing

- `Document::Encrypt` / `Decrypt` with RC4-40, RC4-128, AES-128, and AES-256 (PDF 2.0, R=6),
  governed by the `Permissions` flags enum.
- Detached PKCS#7 (`adbe.pkcs7.detached`) signatures with a byte-exact `/ByteRange` via
  `Facades::PdfFileSignature`.

#### Facades

- `Aspose::Pdf::Facades` API: `PdfConverter`, `PdfExtractor`, `PdfFileSecurity`,
  `PdfFileSignature`, `PdfBookmarkEditor`, `PdfFileEditor` (concatenate and split), `PdfFileStamp`
  (header, footer, page numbers), `PdfFileInfo`, `PdfPageEditor`, `PdfContentEditor`,
  `PdfAnnotationEditor`, `PdfXmpMetadata`, and `FormEditor`.

#### Packaging and build

- Single-entry header `aspose.pdf.foss.hpp`, which includes the whole public API.
- CMake package export: `find_package(aspose_pdf_foss CONFIG)` provides the
  `aspose_pdf_foss::aspose_pdf_foss` target (also available as an alias for `add_subdirectory`
  consumers). The target requires C++20 and passes that requirement on to consumers.
- `cmake --install` installs only the public headers, the static library, the CMake package files,
  and the license texts (MIT, OFL 1.1 for the bundled fonts, third-party notices).
- Prebuilt release archives for Linux x64 (GCC 13) and Windows x64 (MSVC, Release and Debug), each
  smoke-tested as a `find_package` consumer before publishing.
- MSVC warning C4250 (inheritance via dominance in the facade hierarchy) is suppressed inside the
  facade headers, so consumers building at `/W4` see no warnings from the library.
- 12 runnable examples under `examples/` and a GoogleTest suite; CI on Linux (GCC 13, Clang 16)
  and Windows (MSVC).

### Known limitations

- The `XmpValue` helpers `IsDateTime`, `IsField`, `IsNamedValue`, `IsRaw`, `IsNamedValues`, and
  `IsStructure` are stubs that always return `false`.
- Standard-14 fallback fonts cover Helvetica, Times-Roman, and Courier (four styles each); Symbol
  and ZapfDingbats have no bundled fallback.
- Setting metadata on a from-scratch document and saving throws: the incremental writer can only
  patch an existing `/Info` object.
- Encryption permissions are enforced by PDF viewers, not by this library.

[Unreleased]: https://github.com/aspose-pdf-foss/Aspose.PDF-FOSS-for-Cpp/compare/v1.0.1...HEAD
[1.0.1]: https://github.com/aspose-pdf-foss/Aspose.PDF-FOSS-for-Cpp/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/aspose-pdf-foss/Aspose.PDF-FOSS-for-Cpp/releases/tag/v1.0.0
