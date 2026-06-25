// Smoke test for Aspose::Pdf::BitmapInfo.
#include <aspose/pdf/bitmap_info.hpp>

#include <cstdint>
#include <iostream>
#include <vector>
#include <utility>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using Aspose::Pdf::BitmapInfo;

    std::vector<std::uint8_t> pixels(24, 0xAA);
    const auto* pixels_data = pixels.data();
    BitmapInfo info(std::move(pixels), 2, 3,
                    BitmapInfo::PixelFormat::Rgba32);
    Check(info.Width() == 2, "Width");
    Check(info.Height() == 3, "Height");
    Check(info.Format() == BitmapInfo::PixelFormat::Rgba32, "Format");
    Check(info.PixelBytes().size() == 24, "PixelBytes size");
    Check(info.PixelBytes().data() == pixels_data,
          "PixelBytes data buffer moved (not copied)");

    Check(static_cast<int>(BitmapInfo::PixelFormat::Rgb24)  == 0, "Rgb24 ordinal");
    Check(static_cast<int>(BitmapInfo::PixelFormat::Bgr24)  == 1, "Bgr24 ordinal");
    Check(static_cast<int>(BitmapInfo::PixelFormat::Rgba32) == 2, "Rgba32 ordinal");
    Check(static_cast<int>(BitmapInfo::PixelFormat::Argb32) == 3, "Argb32 ordinal");
    Check(static_cast<int>(BitmapInfo::PixelFormat::Bgra32) == 4, "Bgra32 ordinal");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
