// Smoke test for Aspose::Pdf::Devices::Resolution.
#include <aspose/pdf/resolution.hpp>

#include <iostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using Aspose::Pdf::Devices::Resolution;

    Resolution one(150);
    Check(one.X() == 150, "single-arg ctor sets X");
    Check(one.Y() == 150, "single-arg ctor sets Y");

    Resolution two(72, 96);
    Check(two.X() == 72, "two-arg ctor X");
    Check(two.Y() == 96, "two-arg ctor Y");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
