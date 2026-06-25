// Demonstrate the rendering-device surface from this beat.
// Process(Page, ostream) overrides exist + are wired to
// foundation::page_renderer / foundation::text_extractor — but
// driving them requires a Page instance, which canonical
// PageCollection produces via its indexer (NYI in v1 surface).
//
// Snippet shows the parts that ARE callable today: ctors,
// property accessors, value round-trips.

#include <aspose/pdf/bmp_device.hpp>
#include <aspose/pdf/jpeg_device.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/text_device.hpp>

#include <iostream>

int main() {
    using namespace Aspose::Pdf;

    // PNG device, fixed-size output, transparent background
    Devices::Resolution res(150, 150);
    Devices::PngDevice  png(800, 600, res);
    png.TransparentBackground(true);
    std::cout << "PngDevice "
              << png.Width() << "x" << png.Height()
              << " @ " << png.Resolution().X() << " DPI"
              << ", transparent=" << std::boolalpha
              << png.TransparentBackground() << "\n";

    // JPEG device with quality (ctor parameter, no public Quality property)
    Devices::JpegDevice jpeg(PageSize::A4(), Devices::Resolution(300), 85);
    std::cout << "JpegDevice from A4 @ "
              << jpeg.Resolution().X() << " DPI, "
              << jpeg.Width() << "x" << jpeg.Height() << "\n";

    // BMP device sized to PageLetter
    Devices::BmpDevice bmp(PageSize::PageLetter());
    std::cout << "BmpDevice from PageLetter "
              << bmp.Width() << "x" << bmp.Height() << "\n";

    // TextDevice — default-construct; UTF-8 output verbatim
    Devices::TextDevice text;
    std::cout << "TextDevice default-constructed\n";

    return 0;
}
