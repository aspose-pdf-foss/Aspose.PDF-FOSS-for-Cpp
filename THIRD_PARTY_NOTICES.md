# Third-Party Notices

Aspose.PDF FOSS for C++ has **no third-party runtime dependencies** — the
compiled library links against nothing but the C++ standard library, and
all codecs (TIFF, JPEG, PNG, …) are implemented from scratch within the
library.

The following third-party materials are bundled or fetched only for the
purposes noted below.

## Bundled

### Liberation Fonts (SIL Open Font License 1.1)

The library bundles the Liberation font family (outlines embedded into a
generated source file) to provide metrically-compatible substitutes for
the Standard-14 PDF fonts (`Helvetica`, `Times-Roman`, `Courier`, and
their styles).

- Upstream: https://github.com/liberationfonts/liberation-fonts
- License: SIL Open Font License, Version 1.1 — https://openfontlicense.org
- Copyright: © Red Hat, Inc., with Reserved Font Name "Liberation".

The full OFL 1.1 license text is distributed with the font sources under
`src/internal/standard14_outlines.fonts/`.

## Fetched at build time (tests only)

### GoogleTest (BSD 3-Clause)

The test suite uses GoogleTest, which CMake fetches via `FetchContent` at
configure time. It is **not** part of the shipped library and is not
required to build or consume the library.

- Upstream: https://github.com/google/googletest
- License: BSD 3-Clause — https://github.com/google/googletest/blob/main/LICENSE
- Copyright: © Google Inc.
