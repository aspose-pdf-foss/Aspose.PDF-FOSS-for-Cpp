cmake_minimum_required(VERSION 3.20)
project(Aspose_PDF_FOSS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Python is needed only at build time, to embed the 12 Liberation
# TTF binaries into a generated `.inc` data table consumed by
# foundation::standard14_outlines. The custom command below uses
# the absolute Python interpreter path so the script runs on
# Windows where `python3.exe` is absent from python.org installs
# (Microsoft Store Python provides a `python3` shim, python.org
# does not).
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Default to Release when no build type is specified. Without this
# CMake leaves CMAKE_BUILD_TYPE empty and uses -O0 -g, which makes
# the rasterizer + TrueType parser ~10x slower than they need to
# be (concretely: full test suite goes from ~2s in Release to
# ~12s in unoptimized). Pass `-DCMAKE_BUILD_TYPE=Debug` explicitly
# when stepping through code in a debugger.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Default build type; override with -DCMAKE_BUILD_TYPE=Debug" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  URL      https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
)
FetchContent_MakeAvailable(googletest)

# ──────────────────────────────────────────────────────────────────────────
# Library
# ──────────────────────────────────────────────────────────────────────────
# Source list — explicit, NOT a recursive glob.
#
# The library compiles only the public-API bodies plus the foundation
# primitives those bodies transitively need. Foundation primitives that
# exist on disk for capabilities not yet shipped (image decoding, font
# rasterisation, etc.) are NOT compiled — they'd pull additional system
# dependencies (libtiff, ...) for no current caller.
#
# Add foundation primitives here as their public-API consumers land.

set(ASPOSE_PDF_FOSS_SOURCES
  # Public API bodies
  src/public/document.cpp
  src/public/document_info.cpp
  src/public/page_collection.cpp
  src/public/page.cpp
  src/public/text_absorber.cpp
  src/public/text_fragment.cpp
  src/public/text_fragment_collection.cpp
  src/public/text_fragment_absorber.cpp
  src/public/text_state.cpp
  src/public/text_builder.cpp
  src/public/page_device.cpp
  src/public/document_device.cpp
  src/public/resolution.cpp
  src/public/page_size.cpp
  src/public/rendering_options.cpp
  src/public/image_device.cpp
  src/public/png_device.cpp
  src/public/jpeg_device.cpp
  src/public/bmp_device.cpp
  src/public/text_device.cpp
  src/public/margins.cpp
  src/public/tiff_settings.cpp
  src/public/tiff_device.cpp
  src/public/xmp_value.cpp
  src/public/metadata.cpp
  src/public/document_privilege.cpp
  src/public/color.cpp
  src/public/point.cpp
  src/public/rectangle.cpp
  src/public/margin_info.cpp
  src/public/base_paragraph.cpp
  src/public/border.cpp
  src/public/characteristics.cpp
  src/public/annotation.cpp
  src/public/markup_annotation.cpp
  src/public/text_markup_annotation.cpp
  src/public/annotation_collection.cpp
  src/public/annotation_selector.cpp
  src/public/default_appearance.cpp
  src/public/text_style.cpp
  src/public/text_annotation.cpp
  src/public/link_annotation.cpp
  src/public/pdf_action.cpp
  src/public/explicit_destinations.cpp
  src/public/named_destinations.cpp
  src/public/outlines.cpp
  src/public/page_labels.cpp
  src/public/graph_info.cpp
  src/public/border_info.cpp
  src/public/cell.cpp
  src/public/cells.cpp
  src/public/row.cpp
  src/public/rows.cpp
  src/public/table.cpp
  src/public/paragraphs.cpp
  src/public/drawing.cpp
  src/public/x_image.cpp
  src/public/artifact.cpp
  src/public/text_paragraph.cpp
  src/public/floating_box.cpp
  src/public/svg_load_options.cpp
  src/public/free_text_annotation.cpp
  src/public/highlight_annotation.cpp
  src/public/underline_annotation.cpp
  src/public/strike_out_annotation.cpp
  src/public/squiggly_annotation.cpp
  src/public/common_figure_annotation.cpp
  src/public/circle_annotation.cpp
  src/public/square_annotation.cpp
  src/public/line_annotation.cpp
  src/public/poly_annotation.cpp
  src/public/polygon_annotation.cpp
  src/public/polyline_annotation.cpp
  src/public/stamp_annotation.cpp
  src/public/caret_annotation.cpp
  src/public/ink_annotation.cpp
  src/public/file_specification.cpp
  src/public/file_attachment_annotation.cpp
  src/public/redaction_annotation.cpp
  src/public/watermark_annotation.cpp
  src/public/popup_annotation.cpp
  src/public/screen_annotation.cpp
  src/public/widget_annotation.cpp
  src/public/printer_mark_annotation.cpp
  src/public/corner_printer_mark_annotation.cpp
  src/public/movie_annotation.cpp
  src/public/sound_annotation.cpp
  src/public/rich_media_annotation.cpp
  src/public/color_bar_annotation.cpp
  src/public/trim_mark_annotation.cpp
  src/public/bleed_mark_annotation.cpp
  src/public/registration_mark_annotation.cpp
  src/public/page_information_annotation.cpp
  src/public/embedded_file_collection.cpp
  src/public/option.cpp
  src/public/option_collection.cpp
  src/public/icon_fit.cpp
  src/public/field.cpp
  src/public/text_box_field.cpp
  src/public/number_field.cpp
  src/public/date_field.cpp
  src/public/rich_text_box_field.cpp
  src/public/choice_field.cpp
  src/public/button_field.cpp
  src/public/checkbox_field.cpp
  src/public/radio_button_option_field.cpp
  src/public/radio_button_field.cpp
  src/public/list_box_field.cpp
  src/public/combo_box_field.cpp
  src/public/barcode_field.cpp
  src/public/signature_field.cpp
  src/public/signature_custom_appearance.cpp
  src/public/signature.cpp
  src/public/pkcs1.cpp
  src/public/pkcs7.cpp
  src/public/pkcs7_detached.cpp
  src/public/doc_mdp_signature.cpp
  src/public/external_signature.cpp
  src/public/xfa.cpp
  src/public/alignment_type.cpp
  src/public/vertical_alignment_type.cpp
  src/public/facade.cpp
  src/public/saveable_facade.cpp
  src/public/pdf_file_info.cpp
  src/public/pdf_file_editor.cpp
  src/public/pdf_extractor.cpp
  src/public/pdf_annotation_editor.cpp
  src/public/pdf_file_security.cpp
  src/public/bookmark.cpp
  src/public/pdf_bookmark_editor.cpp
  src/public/pdf_xmp_metadata.cpp
  src/public/form_field_facade.cpp
  src/public/form_editor.cpp
  src/public/signature_name.cpp
  src/public/pdf_file_signature.cpp
  src/public/pdf_page_editor.cpp
  src/public/pdf_converter.cpp
  src/public/pdf_file_stamp.cpp
  src/public/pdf_content_editor.cpp
  src/public/form.cpp

  # Internal foundation primitives (flat layout — see src/internal/).
  # Headers live in include/internal/<primitive>.hpp; bodies here.
  # Adding a new primitive's body to this list lights it up for the
  # public API consumers; primitives present on disk but not listed
  # are excluded (they'd pull system deps for no current caller).
  src/internal/lexer.cpp
  src/internal/flate.cpp
  src/internal/xref.cpp
  src/internal/objects.cpp
  src/internal/trailer.cpp
  src/internal/pages_tree.cpp
  src/internal/outlines.cpp
  src/internal/pdf_writer_incremental.cpp
  src/internal/agl.cpp
  src/internal/encoding_tables.cpp
  src/internal/encoding.cpp
  src/internal/to_unicode.cpp
  src/internal/content_stream.cpp
  src/internal/aes_cbc.cpp
  src/internal/digest.cpp
  src/internal/bigint.cpp
  src/internal/pkcs7_sign.cpp
  src/internal/rc4.cpp
  src/internal/crypt_filter.cpp
  src/internal/encrypt_parser.cpp
  src/internal/encrypt_writer.cpp
  src/internal/text_extractor.cpp
  src/internal/transform.cpp
  src/internal/colorspace.cpp
  src/internal/rasterizer.cpp
  src/internal/dctdecode.cpp
  src/internal/jpx.cpp
  src/internal/dct_encoder.cpp
  src/internal/ccitt.cpp
  src/internal/jbig2.cpp
  src/internal/standard14_metrics.cpp
  src/internal/lzw.cpp
  src/internal/asciihex.cpp
  src/internal/ascii85.cpp
  src/internal/runlength.cpp
  src/internal/tiff_decoder.cpp
  src/internal/tiff_encoder.cpp
  src/internal/bmp_decoder.cpp
  src/internal/bmp_encoder.cpp
  src/internal/png_decoder.cpp
  src/internal/png_encoder.cpp
  src/internal/truetype.cpp
  src/internal/cff.cpp
  src/internal/glyph_rasterizer.cpp
  src/internal/image_compositor.cpp
  src/internal/page_renderer.cpp
  src/internal/palette_quantiser.cpp
  src/internal/standard14_outlines.cpp
  src/internal/xmp.cpp
  src/internal/binarize_bradley.cpp
)

# ──────────────────────────────────────────────────────────────────────────
# Standard14 outlines bundle generator. The 12 Liberation TTF binaries
# under src/foundation/standard14_outlines/fonts/ are embedded into
# the static lib via a build-time-generated .inc file (avoids
# committing a 27 MB constexpr-array source to git). Re-generates
# whenever any TTF or the generator script changes.
# ──────────────────────────────────────────────────────────────────────────
set(STANDARD14_OUTLINES_GENERATOR
  "${CMAKE_SOURCE_DIR}/src/internal/standard14_outlines.fonts/standard14_outlines.py")
set(STANDARD14_OUTLINES_FONTS_DIR
  "${CMAKE_SOURCE_DIR}/src/internal/standard14_outlines.fonts")
set(STANDARD14_OUTLINES_INC
  "${CMAKE_BINARY_DIR}/standard14_outlines_bundle.inc")
file(GLOB STANDARD14_OUTLINES_TTFS
  CONFIGURE_DEPENDS
  "${STANDARD14_OUTLINES_FONTS_DIR}/*.ttf")
add_custom_command(
  OUTPUT "${STANDARD14_OUTLINES_INC}"
  COMMAND "${Python3_EXECUTABLE}" "${STANDARD14_OUTLINES_GENERATOR}"
          "${STANDARD14_OUTLINES_INC}"
  DEPENDS "${STANDARD14_OUTLINES_GENERATOR}"
          ${STANDARD14_OUTLINES_TTFS}
  COMMENT "Generating standard14_outlines_bundle.inc from Liberation TTFs"
  VERBATIM
)
add_custom_target(standard14_outlines_inc
  DEPENDS "${STANDARD14_OUTLINES_INC}")
set_source_files_properties(
  src/internal/standard14_outlines.cpp
  PROPERTIES OBJECT_DEPENDS "${STANDARD14_OUTLINES_INC}")

add_library(aspose_pdf_foss STATIC ${ASPOSE_PDF_FOSS_SOURCES})
target_include_directories(aspose_pdf_foss
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    # Internal foundation headers live in a single tree —
    # ``#include "lexer.hpp"`` etc. resolve via this PRIVATE flag.
    # PRIVATE because these are implementation headers, not part of
    # the lib's public API surface.
    ${CMAKE_SOURCE_DIR}/include/internal
    # .inc helper data files for a few primitives are siblings of
    # their .cpp under src/internal/; they're #include'd into the
    # body's anonymous namespace at compile time.
    ${CMAKE_SOURCE_DIR}/src/internal
    # Generated standard14_outlines_bundle.inc lands in the build dir.
    ${CMAKE_BINARY_DIR}
)

# MSVC interprets source bytes as the system code page (CP-1252
# on en-US Windows, CP-936/932 on Asian locales) unless told
# otherwise. Many of our sources contain UTF-8 multibyte chars in
# both comments and string literals (em dashes, ≥, →, π, …) — see
# `error message strings throughout src/internal/`. /utf-8 forces
# both source and execution charset to UTF-8 so MSVC doesn't emit
# C4819 warnings or produce mojibake error messages at runtime.
# GCC / Clang accept UTF-8 by default and need no equivalent flag.
target_compile_options(aspose_pdf_foss PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /MP>
)

# ──────────────────────────────────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────────────────────────────────
# This aggregator builds the gtest-style unit-test suite. It picks
# up only files matching the gtest naming convention
# (`*_smoke_test.cpp` and `*.cc`); any standalone int-main()
# programs under tests/internal/ are excluded so they don't collide
# with gtest_main.
enable_testing()
file(GLOB_RECURSE ASPOSE_PDF_FOSS_TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/tests/*_smoke_test.cpp"
  "${CMAKE_SOURCE_DIR}/tests/*.cc"
)
list(FILTER ASPOSE_PDF_FOSS_TEST_SOURCES EXCLUDE REGEX
     "/tests/internal/")
if(NOT ASPOSE_PDF_FOSS_TEST_SOURCES)
  file(WRITE "${CMAKE_BINARY_DIR}/_stub_test.cpp" "int main(){return 0;}\n")
  set(ASPOSE_PDF_FOSS_TEST_SOURCES "${CMAKE_BINARY_DIR}/_stub_test.cpp")
endif()

add_executable(aspose_pdf_foss_tests ${ASPOSE_PDF_FOSS_TEST_SOURCES})
target_link_libraries(aspose_pdf_foss_tests PRIVATE
  aspose_pdf_foss
  GTest::gtest_main
)
# Tests that exercise foundation primitives directly need access to
# their headers — single PRIVATE include path covers them all under
# the flat layout.
target_include_directories(aspose_pdf_foss_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include/internal
)
target_compile_options(aspose_pdf_foss_tests PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /MP>
)

# Register the whole gtest binary as ONE ctest test. Running every
# case in a single process keeps ctest fast on all filesystems —
# notably WSL's /mnt 9p mount, where per-test gtest_discover_tests
# would re-launch (and re-page-in) the 15 MB binary hundreds of times.
# Per-test visibility is preserved via gtest's JUnit XML, which CI
# ingests (artifacts:reports:junit). To run a single case locally, use
# the binary's own filter, e.g.
#   ./aspose_pdf_foss_tests --gtest_filter=TextStateSmoke.MeasureStringHelvetica
if(ASPOSE_PDF_FOSS_TEST_SOURCES AND NOT ASPOSE_PDF_FOSS_TEST_SOURCES STREQUAL "${CMAKE_BINARY_DIR}/_stub_test.cpp")
  add_test(
    NAME aspose_pdf_foss_tests
    COMMAND aspose_pdf_foss_tests
            --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest-results.xml)
endif()

# ──────────────────────────────────────────────────────────────────────────
# Examples — standalone programs showing how to call the public API.
# ──────────────────────────────────────────────────────────────────────────
add_subdirectory(examples)

include(GNUInstallDirs)
install(TARGETS aspose_pdf_foss
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} OPTIONAL)
