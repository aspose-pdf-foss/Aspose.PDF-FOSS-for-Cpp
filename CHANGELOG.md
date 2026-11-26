# Changelog

All notable changes to Aspose.PDF FOSS for C++ are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). The public API is the set of
headers installed under `include/aspose/` plus `include/aspose.pdf.foss.hpp`; anything under
`include/internal/` is an implementation detail and may change in any release.

## [Unreleased]

### Added

- Examples 13 - 16: signing and verifying with a PKCS#12 certificate, the Facades editors
  (stamps, attachments, document actions, viewer preferences, page zoom, bookmark XML), embedded
  media (RichMedia) and fax (CCITT Group 4) TIFF output. Example notes no longer describe the
  removed `/Info` limitation.

- `RichMediaAnnotation` embeds video or audio: `SetContent(name, bytes)`, `SetPoster(png/jpeg)`,
  `CustomPlayer(swf)`, `AddCustomData`; Save writes `/RichMediaContent` (assets, configuration,
  instance) and `/RichMediaSettings` (activation on click / page open / page visible) with the
  poster, or a frame with a play triangle, as appearance. Without a player the instance plays the
  media directly. `Update()` checks that content is set. RichMedia annotations used to be dropped
  on save; an annotation type that cannot be saved now throws instead of disappearing.

- `TiffDevice` writes `CompressionType::CCITT3` (T.4, one-dimensional) and `CCITT4` (T.6) TIFFs:
  bi-level WhiteIsZero pages, whatever the colour depth setting — these used to throw. The TIFF
  reader now decodes CCITT (Modified Huffman, Group 3, Group 4) in WhiteIsZero and BlackIsZero
  files. Checked against libtiff: Pillow reads our files, and we read ImageMagick's.

- `TextMarkupAnnotation::GetMarkedText`: the page text under the QuadPoints (lines separated by a
  newline, words by a space; character extents use the text extractor's 0.5 em estimate).
  Text-markup annotations loaded from a file now carry their QuadPoints, and the QuadPoints set
  on a new one are written instead of its rectangle's.
- `PrinterMarkAnnotation::AddPrinterMarks`: trim, bleed and registration marks, CMYK colour bars
  and page information in the page margins outside the trim box, saved as `/PrinterMark`
  annotations (`/MN`) with drawn appearances. Trim, bleed and registration mark annotations now
  place themselves from the page's trim / bleed / media boxes; printer-mark annotations used to be
  dropped on save.

- Forms: `Form::GetFieldsInRect` (fields lying wholly inside a rectangle);
  `Form::MakeFormAnnotationsIndependent` (at Save, a page's widgets whose field also has widgets
  on other pages move into copies of the field, named `<name>0`, `<name>1`…; radio groups and
  group fields are left alone); `CheckboxField::AddOption` (new check boxes: a parent field with
  one widget per option, each with its own export value) and `CheckboxField::Clone`.

- `PdfBookmarkEditor::ExportBookmarksToXML` / `ImportBookmarksWithXML` (the Aspose.PDF bookmark
  XML: nested `<Title>` with style, page destination, named destination or URI, open state) and
  `ExportBookmarksToHtml` / `ExtractBookmarksToHTML` (a nested list of `file.pdf#page=N` links).
- `PdfAnnotationEditor` import: `ImportAnnotationsFromXfdf` / `ImportAnnotationFromXfdf` (XFDF
  per ISO 19444-1 — text, markup, shapes, ink, lines, polygons, stamps, links with URI, popups,
  replies), `ImportAnnotationsFromFdf` (FDF, also without a cross-reference table) and
  `ImportAnnotations` from other PDFs (page N onto page N, appearance streams and popups
  included, form widgets skipped); the annotation-type overloads filter what is imported.
  `ModifyAnnotations` (colour, contents, author, subject, open state of same-type annotations)
  and `ModifyAnnotationsAuthor`.
- `PdfFileSecurity::SetPrivilege(privilege)` without passwords: encrypts with empty user and
  owner passwords, as Aspose.PDF does.

- `PdfPageEditor::ApplyChanges` and `MovePosition` now act: the staged Rotation (multiples of
  90), Zoom, PageSize, horizontal / vertical alignment and move offset redraw each page in
  `ProcessPages` (all when empty) — content scaled, turned and placed in the new page size, the
  media box resized, annotations moved along — and `TransitionType` / `TransitionDuration` /
  `DisplayDuration` set the page's `/Trans` and `/Dur`. Unset properties leave the page as it is.

- `PdfContentEditor` editors that used to do nothing: `AddDocumentAttachment` /
  `DeleteAttachments`; `AddDocumentAdditionalAction` (JavaScript on document open — the
  `/OpenAction` — close, save and print events) and `RemoveDocumentOpenAction`;
  `ChangeViewerPreference` / `GetViewerPreference` with the new `ViewerPreference` flags (page
  layout and mode, toolbar / menu / window UI, direction, duplex, print scaling; every set flag
  is applied); `ReplaceImage`; and stamp editing — `DeleteStamp`, `DeleteStampById[s]`,
  `HideStampById` / `ShowStampById`, `MoveStamp` / `MoveStampById` — for the stamps
  `PdfFileStamp` draws, which now carry their `StampId`. Stamp indexes are 0-based and Move*
  takes the stamp's new lower-left corner (the parameters were `offsetX` / `offsetY`).

- Digital signatures with your own certificate: `PdfFileSignature::Sign` / `Certify` sign with the
  RSA or EC key (ECDSA on P-256 / P-384 / P-521, with SHA-256 / 384 / 512 and RFC 6979
  deterministic nonces) in a PKCS#12 file, either the `Signature(pfx, password)` one or the one from
  `SetCertificate` (PBES2 / AES and legacy 3DES / RC2 files; a wrong password throws). Without a
  certificate the bundled test key is used, as before.
- Signature verification: `PdfFileSignature::VerifySignature` / `VerifySigned` and
  `Signature::Verify` (on a loaded document's signature field) check the digest of the signed byte
  ranges and the RSA (PKCS#1 v1.5 or RSASSA-PSS with MGF1) or ECDSA signature
  (`adbe.pkcs7.detached`, `ETSI.CAdES.detached`, `adbe.pkcs7.sha1`). They return false for a
  modified document. Certificate trust and revocation are not checked.
- `Certify` writes a DocMDP certification (`/Reference` transform with the chosen access
  permissions, catalog `/Perms /DocMDP`); `GetAccessPermissions` and `IsCertified` read it from
  loaded documents.
- `RemoveSignature` (removes the field, or with `keepFieldEmpty` leaves it empty),
  `RemoveSignatures`, `ContainsUsageRights` / `RemoveUsageRights` (`/Perms /UR3`) and
  `IsLtvEnabled` (`/DSS`) now work instead of doing nothing or returning false.

- `PdfFileEditor` page layout: `MakeBooklet` (pages in folding order, padded to a multiple of 4,
  optionally on a given page size), `MakeNUp` (pages of several files side by side or stacked),
  `AddMargins` / `AddMarginsPct`, `ResizeContents` / `ResizeContentsPct` (with
  `ContentsResizeParameters` in units, percents or Auto) and `AddPageBreak`, plus their `Try*`
  forms. Booklet and N-up pages carry the source pages' content only, not their annotations or
  form fields. `ContentsResizeValue::IsAuto()` added.

- `FormEditor` now edits fields instead of ignoring the calls: `SetFieldAttribute`,
  `SetFieldAppearance` / `GetFieldAppearance`, `SetFieldLimit`, `SetFieldCombNumber`, `MoveField`,
  `Single2Multiple`, `SetFieldAlignment` / `SetFieldAlignmentV`, `RenameField`, `AddListItem` /
  `DelListItem`, `CopyInnerField`, `CopyOuterField` (from another PDF) and `DecorateField` (font,
  size, alignment, caption, items and position from `FormFieldFacade`). `AddField` fills list,
  combo and radio fields from `Items` (radio layout from `RadioHoriz`, `RadioGap`,
  `RadioButtonItemSize`) and makes `MultiLineText` fields multi-line.
- `FormEditor` actions: `SetFieldScript` / `AddFieldScript` (JavaScript on push buttons),
  `SetSubmitUrl` / `SetSubmitFlag` and `AddSubmitBtn` (form submission to a URL as FDF, HTML,
  XFDF or PDF), and `RemoveFieldAction`.

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
- `Form::AddFieldAppearance(field, page, rect)` shows a field at one more place (for example the
  same name on every page); all places share the field's value. `Form::RemoveFieldAppearance(field,
  index)` removes a place (1-based; the last one cannot be removed). Both work for new fields and
  for fields loaded from a file; radio buttons are rejected, since their places are their options.
- `FreeTextAnnotation::SetTextStyle(from, to, styles)` makes characters `[from, to)` bold and/or
  italic, drawn with the matching face of the `DefaultAppearance` font family (Helvetica, Times or
  Courier) and written to the annotation's rich text (`/RC`).

### Changed

- `Forms::Signature::Verify()` no longer always returns `true`: for a signature read from a
  loaded document it verifies it, and for a `Signature` not bound to a signed document it throws
  `std::logic_error`. `PdfFileSignature::VerifySignature` / `VerifySigned` no longer report a
  signature as valid just because it exists and covers the whole file.
- Methods that used to do nothing or return an empty value now throw `std::logic_error`
  ("not implemented — blocked on capability '…'") so a missing feature is visible:
  `CheckboxField::Clone` is no longer `noexcept`. `grep PENDING src` lists the rest.

### Fixed

- Attachments and XMP metadata written at Save after an earlier incremental edit (for example a
  `PdfFileStamp` stamp) could reuse the ids of the objects that edit added, overwriting them: the
  trailer `/Size` read from a multi-revision file can be stale. New ids now also stay above every
  existing object.
- Document information set on a document without an `/Info` dictionary — created from scratch,
  or loaded from a file that has none — is saved in a new `/Info` object referenced from the
  trailer. Saving used to throw.
- Adding, deleting and importing pages (`PageCollection`, `PdfFileEditor`) now works on documents
  with a nested `/Pages` tree: it is flattened first, with inherited resources, boxes and
  rotation pinned on each page. These operations used to throw for such files.
- `Page::Annotations()` now includes Line, Ink, Polygon, PolyLine, FreeText and FileAttachment
  annotations read from a file (with their end points, ink strokes, vertices, attached file
  name); they used to be left out of the collection.
- Edits to annotations loaded from a file — contents, author (`Title`), subject, colour, a text
  annotation's open state — are now saved; they used to be dropped because loaded annotations
  were never rewritten. Loading reads those properties, and new annotations now write their
  author, subject, colour and open state.
- `Document(path)` opens an encrypted file that has no user password (decrypting it, with
  `IsEncrypted()` true) instead of reading it as if it were not encrypted.
- `PdfFileInfo::HasOpenPassword`, `HasEditPassword` and `PasswordType` report how an encrypted
  document was opened instead of always "no password"; `HasCollection` reads the catalog
  `/Collection` instead of always `false`.
- `PdfFileStamp::PageWidth` / `PageHeight` return the first page's size instead of US Letter.
- `PdfBookmarkEditor::ExtractBookmarks(bool upperLevel)`, `(title)` and `(parent)` filter the
  bookmarks (top level only, a titled subtree, a bookmark's children) instead of returning all.
- Edits to form fields loaded from a file are no longer lost on save. Previously only a changed
  value was written; renaming, read-only / required / no-export, moving, multi-line, comb,
  `MaxLen`, alignment, annotation flags, option lists and `DefaultAppearance` font and size were
  silently dropped. Only the changed keys of the original object are rewritten.
- `TextAbsorber` and `TextDevice` now extract text drawn inside Form XObjects (`/Name Do`), such as
  page headers from many PDF producers and form fields or annotations burned in by `Flatten`. Line
  breaks follow the real position on the page (`cm` and the form's `/Matrix`).
- Text field appearances now follow the field's settings instead of a fixed one-line Helvetica
  12 pt:
  - New fields use their `DefaultAppearance` font and size (size 0 = auto-fit) and
    `TextHorizontalAlignment`; text boxes honour `Multiline` (word wrapping), `ForceCombs` with
    `MaxLen` (one character per cell). `/DA`, `/Q`, `/MaxLen` and the comb flag are written, and
    the font is added to `/AcroForm /DR`.
  - Fields loaded from a file keep their own `/DA` font, size and colour when their value is
    changed or when they are flattened without an appearance: the `/DR` font is referenced as is,
    and `/Q`, multi-line, comb and `/MaxLen` apply.
  - Password fields show asterisks instead of their value.
- FreeText annotations now carry the required `/DA` entry, their appearance uses the font size
  from `DefaultAppearance` instead of a fixed 11 pt, and long text wraps within the box.
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
