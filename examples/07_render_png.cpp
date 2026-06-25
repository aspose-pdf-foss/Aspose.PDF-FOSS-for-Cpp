// Rasterise a PDF page to PNG via PngDevice::Process(Page, ostream).
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: 07_render_png <input.pdf> <page_number> "
                  << "<output.png>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    const int page_number = std::stoi(argv[2]);
    auto page = doc.Pages()[page_number];

    Aspose::Pdf::Devices::PngDevice png(
        Aspose::Pdf::Devices::Resolution(150));

    std::ofstream out(argv[3], std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open " << argv[3] << " for writing\n";
        return 1;
    }
    png.Process(page, out);
    out.close();

    std::cout << "Rendered page " << page_number << " of " << argv[1]
              << " to " << argv[3] << " (PNG @ 150 DPI)\n";
    return 0;
}
