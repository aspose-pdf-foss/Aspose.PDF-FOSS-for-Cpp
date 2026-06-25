// Render a PDF that references a Standard14 font WITHOUT
// embedding it. This is the most common shape of real-world
// PDFs — Office "Save as PDF" with default Calibri / Arial,
// most older scientific papers with Times-Roman, etc.
//
// PDF 32000-1:2008 §9.6.2.2 says the 14 standard PostScript
// fonts (Helvetica / Times-Roman / Courier × 4 styles each,
// plus Symbol + ZapfDingbats) are ALWAYS available to a
// conforming reader without being embedded. The bundled fixture
// hello_world.pdf carries exactly this shape — its font dict is
// just `<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>`,
// no /FontDescriptor, no /FontFile, no embedded glyph data at
// all.
//
// foundation::standard14_outlines supplies the missing outlines:
//   1. Probe per-OS default font dirs (Helvetica.ttc on macOS,
//      Arial.ttf on Windows, Liberation*.ttf on most Linux
//      desktops, etc.).
//   2. Fall back to bundled Liberation Sans / Serif / Mono
//      compiled into the static lib (~4 MB total, OFL 1.1).
//
// Either way the output PNG paints actual glyphs. Symbol +
// ZapfDingbats are deferred — no permissively-licensed metric-
// equivalent substitute located yet.
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
        std::cerr << "usage: 11_render_standard14 <input.pdf> "
                  << "<output.png>\n";
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

    std::cout << "Rendered " << argv[1] << " (Standard14 reference, "
              << "no embedded font) to " << argv[2]
              << " (PNG @ 150 DPI). Glyphs come from a system font "
              << "or the bundled Liberation substitute via "
              << "foundation::standard14_outlines.\n";
    return 0;
}
