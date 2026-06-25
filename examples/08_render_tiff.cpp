// Rasterise every page of a PDF to a single multi-page TIFF via
// TiffDevice::Process(Document, fromPage, toPage, ostream).
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: 08_render_tiff <input.pdf> <output.tiff>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    auto& pages = doc.Pages();

    Aspose::Pdf::Devices::TiffDevice tiff(
        Aspose::Pdf::Devices::Resolution(150));

    std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open " << argv[2] << " for writing\n";
        return 1;
    }
    tiff.Process(doc, /*fromPage=*/1, static_cast<int>(pages.Count()), out);
    out.close();

    std::cout << "Rendered all " << pages.Count() << " page(s) of "
              << argv[1] << " to a multi-page TIFF at "
              << argv[2] << " (LZW @ 150 DPI)\n";
    return 0;
}
