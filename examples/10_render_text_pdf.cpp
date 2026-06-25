// Render a text-bearing PDF to PNG end-to-end. Demonstrates that
// PngDevice::Process raster path handles glyphs from an embedded
// TrueType font, not just rectangles / vector primitives.
//
// Note: this example takes a PDF whose text is rendered with an
// EMBEDDED font (the bundled pdfs/text_hello.pdf carries a
// 442-byte FontFile2 stream). PDFs that reference Standard14 fonts
// like /Helvetica, /Times-Roman, etc. WITHOUT embedding render
// blank in v1 — Standard14 glyph synthesis is a follow-up.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: 10_render_text_pdf <input.pdf> <output.png>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::PngDevice png(
        Aspose::Pdf::Devices::Resolution(150));

    std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open " << argv[2] << " for writing\n";
        return 1;
    }
    png.Process(page, out);
    out.close();

    std::cout << "Rendered text page from " << argv[1]
              << " to " << argv[2] << " (PNG @ 150 DPI). "
              << "Glyphs come from the PDF's embedded font; the "
              << "image should show painted text, not a blank page.\n";
    return 0;
}
