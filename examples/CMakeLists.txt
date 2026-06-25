# Examples — small standalone programs showing how to call into
# Aspose::Pdf:: surfaces. Each .cpp is self-contained, compiles to
# its own executable under build/examples/, and takes any PDF as
# argv[1] (most also take an output path as argv[2]).
#
# Build along with the rest of the project; the executables land at
# build/examples/<name>. Run them by hand with a PDF of your choice.
#
# These are intentionally small + comment-light — the .cpp itself is
# the documentation. README.md beside this file explains what each
# one demonstrates.

set(ASPOSE_PDF_FOSS_EXAMPLES
  01_pages
  02_read_metadata
  03_write_metadata
  04_text_extraction
  05_save_roundtrip
  06_devices_surface
  07_render_png
  08_render_tiff
  09_indexed_tiff
  10_render_text_pdf
  11_render_standard14
  12_create_features
)

foreach(example ${ASPOSE_PDF_FOSS_EXAMPLES})
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE aspose_pdf_foss)
  target_compile_options(${example} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /MP>
  )
endforeach()

# 12_create_features embeds the bundled examples/pdflib.png; pass its
# absolute path so the example finds the asset regardless of the cwd.
target_compile_definitions(12_create_features PRIVATE
  CREATE_FEATURES_PNG="${CMAKE_CURRENT_SOURCE_DIR}/pdflib.png")
