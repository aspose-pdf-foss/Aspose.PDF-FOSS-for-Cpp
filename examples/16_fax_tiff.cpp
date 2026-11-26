// Render every page of a PDF into a bi-level, fax-compressed TIFF
// (CCITT Group 4, the format fax and archive systems expect).
//
//   usage: 16_fax_tiff <input.pdf> <output.tiff>
#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_device.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: 16_fax_tiff <input.pdf> <output.tiff>\n";
        return 2;
    }
    namespace dev = Aspose::Pdf::Devices;
    Aspose::Pdf::Document doc(argv[1]);

    // CCITT codes black-and-white pages: the page is rendered 1 bit per
    // pixel whatever the colour depth setting. CCITT3 gives Group 3.
    dev::TiffSettings settings(Aspose::Pdf::Devices::CompressionType::CCITT4);
    dev::TiffDevice tiff(dev::Resolution(200), settings);

    std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "cannot open " << argv[2] << " for writing\n";
        return 1;
    }
    tiff.Process(doc, 1, static_cast<int>(doc.Pages().Count()), out);
    out.close();
    std::cout << "Wrote " << doc.Pages().Count() << " page(s) of " << argv[1]
              << " to " << argv[2] << " as a CCITT Group 4 TIFF @ 200 DPI\n";
    return 0;
}
