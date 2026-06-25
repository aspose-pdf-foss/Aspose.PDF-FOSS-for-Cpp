// Smoke test for Aspose::Pdf::PageSize.
#include <aspose/pdf/page_size.hpp>

#include <iostream>
#include <string>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using Aspose::Pdf::PageSize;

    PageSize sized(100.0f, 200.0f);
    Check(sized.Width() == 100.0f, "ctor sets Width");
    Check(sized.Height() == 200.0f, "ctor sets Height");

    sized.Width(150.0f);
    sized.Height(250.0f);
    Check(sized.Width() == 150.0f, "Width setter round-trips");
    Check(sized.Height() == 250.0f, "Height setter round-trips");

    sized.IsLandscape(true);
    Check(sized.IsLandscape(), "IsLandscape setter round-trips");

    PageSize a4 = PageSize::A4();
    Check(a4.Width() > 0 && a4.Height() > 0, "A4 factory yields positive dims");
    Check(a4.Height() > a4.Width(), "A4 portrait: Height > Width");

    PageSize letter = PageSize::PageLetter();
    Check(letter.Width() > 0 && letter.Height() > 0,
          "Letter factory yields positive dims");

    // Canonical Aspose.PDF 26.4.0 named-page-size dimensions
    // (extracted from the reference values). Any drift here
    // indicates a value mismatch against the canonical assembly.
    struct Expected { const char* name; PageSize value; float w, h; };
    Expected expected[] = {
        {"A0",         PageSize::A0(),         2384.0f, 3370.0f},
        {"A1",         PageSize::A1(),         1684.0f, 2384.0f},
        {"A2",         PageSize::A2(),         1190.0f, 1684.0f},
        {"A3",         PageSize::A3(),          842.0f, 1190.0f},
        {"A4",         PageSize::A4(),          595.0f,  842.0f},
        {"A5",         PageSize::A5(),          421.0f,  595.0f},
        {"A6",         PageSize::A6(),          297.0f,  421.0f},
        {"B5",         PageSize::B5(),          501.0f,  709.0f},
        {"PageLetter", PageSize::PageLetter(),  612.0f,  792.0f},
        {"PageLegal",  PageSize::PageLegal(),   612.0f, 1008.0f},
        {"PageLedger", PageSize::PageLedger(), 1224.0f,  792.0f},
        {"P11x17",     PageSize::P11x17(),      792.0f, 1224.0f},
    };
    for (const auto& e : expected) {
        std::string label = std::string("canonical ") + e.name + " width";
        Check(e.value.Width() == e.w, label.c_str());
        label = std::string("canonical ") + e.name + " height";
        Check(e.value.Height() == e.h, label.c_str());
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
