// Smoke test for Aspose::Pdf::Devices::TiffSettings.
#include <aspose/pdf/color_depth.hpp>
#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/margins.hpp>
#include <aspose/pdf/shape_type.hpp>
#include <aspose/pdf/tiff_settings.hpp>

#include <iostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using namespace Aspose::Pdf::Devices;

    TiffSettings def;
    Check(def.Compression() == CompressionType::LZW,
          "default compression LZW");
    Check(def.Depth() == ColorDepth::Default,
          "default depth Default");
    Check(def.Shape() == ShapeType::None,
          "default shape None");
    Check(!def.SkipBlankPages(), "default SkipBlankPages false");

    TiffSettings full(CompressionType::CCITT4,
                      ColorDepth::Format1bpp,
                      Margins(5, 6, 7, 8),
                      true,
                      ShapeType::Landscape);
    Check(full.Compression() == CompressionType::CCITT4,
          "full ctor compression");
    Check(full.Depth() == ColorDepth::Format1bpp,
          "full ctor depth");
    Check(full.SkipBlankPages(), "full ctor skip blank pages");
    Check(full.Shape() == ShapeType::Landscape,
          "full ctor shape");
    Check(full.Margins().Left() == 5, "full ctor margins");

    TiffSettings setters;
    setters.Brightness(0.85f);
    setters.Compression(CompressionType::None);
    Check(setters.Brightness() > 0.84f && setters.Brightness() < 0.86f,
          "Brightness setter round-trips");
    Check(setters.Compression() == CompressionType::None,
          "Compression setter round-trips");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
