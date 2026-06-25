# Contributing to Aspose.PDF FOSS for C++

Thanks for your interest in contributing! This document covers how to
build, test, and submit changes.

## Building

Requirements: a C++20 compiler (clang ≥ 16, gcc ≥ 13, or MSVC 2022 ≥
17.5), CMake ≥ 3.22, and Python 3 (used by a build step that embeds the
bundled font outlines).

```bash
cmake -S . -B build
cmake --build build -j
```

The static library lands at `build/libaspose_pdf_foss.a` (`.lib` on
MSVC). See [`README.md`](README.md) for the full requirements and a
quick-start tour of the API.

## Running the tests

```bash
cmake --build build
cd build && ctest --output-on-failure
```

The test suite uses [GoogleTest](https://github.com/google/googletest),
which CMake fetches automatically at configure time (this needs network
access on the first configure).

## Repository layout

| Path | Contents |
|------|----------|
| `include/aspose/pdf/` | Public, consumer-facing API headers |
| `include/internal/`   | Internal foundation-primitive headers |
| `src/public/`         | Public-API implementation |
| `src/internal/`       | Foundation primitives (codecs, parsers, rasteriser, …) |
| `tests/`              | GoogleTest unit + smoke tests |
| `examples/`           | Runnable demo programs |
| `pdfs/`               | Small sample PDFs used by the examples |

Public-API code under `src/public/` calls into the foundation primitives
under `src/internal/`; the primitives never reach back into the public
API.

## Coding conventions

- Match the style of the surrounding code (indentation, naming, comment
  density). The codebase is C++20 and dependency-free at runtime — please
  keep it that way; do not introduce third-party runtime dependencies.
- Keep public headers under `include/aspose/pdf/` clean: the public
  surface mirrors the canonical Aspose.PDF API names and shapes.
- Add or update tests under `tests/` for any behavioural change, and make
  sure `ctest` is green before opening a pull request.

## Submitting a pull request

1. Fork the repository and create a topic branch.
2. Make your change with accompanying tests.
3. Ensure `cmake --build build` and `ctest` both pass.
4. Open a pull request describing the change and the motivation.

By submitting a contribution you agree that it is licensed under the
project's [MIT License](LICENSE).
