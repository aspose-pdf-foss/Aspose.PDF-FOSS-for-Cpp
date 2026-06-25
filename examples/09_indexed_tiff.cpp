// Render a PDF page to an 8-bit palettised TIFF (256-colour
// palette, median-cut quantisation via foundation::palette_quantiser).
//
// For caller-pluggable palette quantisation (custom algorithm,
// fixed corporate palette, etc.) — subclass
// Aspose::Pdf::IIndexBitmapConverter and pass it to one of the
// TiffDevice ctors that accept IIndexBitmapConverter&. This
// example uses the built-in default; see
// tests/i_index_bitmap_converter_smoke_test.cpp for a custom
// converter example.
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: 09_indexed_tiff <input.pdf> <page_number> "
                  << "<output.tiff>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    auto page = doc.Pages()[std::stoi(argv[2])];

    Aspose::Pdf::Devices::TiffSettings settings;
    settings.Depth(Aspose::Pdf::Devices::ColorDepth::Format8bpp);
    Aspose::Pdf::Devices::TiffDevice tiff(
        Aspose::Pdf::Devices::Resolution(150), settings);

    std::ofstream out(argv[3], std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open " << argv[3] << " for writing\n";
        return 1;
    }
    tiff.Process(page, out);
    out.close();

    std::cout << "Rendered page " << argv[2] << " of " << argv[1]
              << " to " << argv[3]
              << " as 8bpp palettised TIFF (256-colour palette via "
              << "median-cut, LZW @ 150 DPI)\n";
    return 0;
}
