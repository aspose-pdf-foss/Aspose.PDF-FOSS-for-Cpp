# Go-parity tests

These tests port the **public-API** test suite of the `aspose-pdf-foss-for-go`
lib into this C++ lib. Because the two libs expose different public surfaces —
Go uses an idiomatic flat API (`Open`, `PageCount`, `AddText`, …) while this lib
exposes the **canonical Aspose.PDF .NET** surface — the port is **intent-based**,
not a 1:1 translation: each Go test's *behaviour* is re-expressed against the
canonical C++ API.

## What is ported

Every Go test that exercises a capability tracing to the canonical Aspose.PDF
public surface (document open/save, page geometry & rotation, annotations,
text, images, fonts, forms, encryption, metadata, navigation, tables, …).

## What is skipped (and why)

- **`*_internal_test.go`** — exercise Go's invented internals; nothing to map.
- **Invented API** — capabilities with no canonical equivalent: the structural
  `Validate()` linter, `OptimizeImages`, `ImageToDocument*`, `Document.Reorder`,
  the `*SVG` pre-parse handle / `SVGFontResolver`, idiomatic aggregate option
  structs and `PageSizes()`-style helpers. (See `go-vs-cpp-api-gap-analysis.md`.)
- **Go-specific semantics** — e.g. `Document.Rotate(angle, pageNums…)`
  varargs/accumulation and invalid-angle validation; the canonical analogue is
  the per-page `Page.Rotate` *property* (absolute), which is what the ported
  tests assert.
- **Stream-only paths** — `WriteTo(io.Writer)` etc.; `System.IO.Stream` is off
  the cpp catalog, so file-based `Save` is asserted instead.

Each ported file notes, in comments, any Go test it deliberately did not carry
over.
