# XMP fixture corpus

Fixture corpus for the `foundation::xmp` primitive. Each
fixture is a self-contained XMP packet (`.xmp` file).
Canonical sidecars are produced by oracle agreement
between two independent XMP libraries.

## Oracle requirements

Two truly-independent code paths in Python:

- **python-xmp-toolkit** (`oracle_libxmp.py`) — wraps Adobe's
  XMP Toolkit C library (libexempi). Install:
  ```
  brew install exempi
  pip install python-xmp-toolkit
  ```
- **lxml** (`oracle_lxml.py`) — wraps libxml2 (GNOME), parses
  the XMP packet as plain XML and walks the RDF tree
  directly. Install:
  ```
  pip install lxml
  ```

Independence: libexempi and libxml2 are unrelated codebases
maintained by different teams; the two adapters share no
common parser code. Both libraries are MIT/BSD-equivalent;
neither is linked into the foundation library — they run
only at fixture-authoring time.

## Fixture catalogue

- `simple_text.xmp` — Alt + Seq + simple-text + 3 namespaces
  (dc, xmp, pdf). The standard happy path.
- `multibyte_title.xmp` — UTF-8 title with CJK + accented
  Latin + symbol characters. Validates UTF-8 round-trip.
- `attribute_form.xmp` — properties as attributes on
  `rdf:Description` (no nested elements). RDF allows this
  shape and many writers emit it.
- `empty.xmp` — `rdf:Description` with no properties.
  Validates empty-output handling.
