// Smoke test for Aspose::Pdf::RenderingOptions.
#include <aspose/pdf/rendering_options.hpp>

#include <iostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using Aspose::Pdf::RenderingOptions;

    RenderingOptions opts;
    Check(!opts.AnalyzeFonts(), "AnalyzeFonts default false");
    Check(opts.DefaultFontName().empty(), "DefaultFontName default empty");
    Check(opts.HeightExtraUnits() == 0.0f,
          "HeightExtraUnits default 0");

    opts.AnalyzeFonts(true);
    opts.DefaultFontName("Liberation Serif");
    opts.MaxFontsCacheSize(64);
    opts.WidthExtraUnits(1.5f);

    Check(opts.AnalyzeFonts(), "AnalyzeFonts setter");
    Check(opts.DefaultFontName() == "Liberation Serif",
          "DefaultFontName setter");
    Check(opts.MaxFontsCacheSize() == 64,
          "MaxFontsCacheSize setter");
    Check(opts.WidthExtraUnits() == 1.5f,
          "WidthExtraUnits setter");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
