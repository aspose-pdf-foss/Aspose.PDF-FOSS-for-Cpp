// Smoke test for Aspose::Pdf::Devices::Margins.
#include <aspose/pdf/margins.hpp>

#include <iostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
}

int main() {
    using Aspose::Pdf::Devices::Margins;

    Margins zero;
    Check(zero.Left()   == 0, "default Left   == 0");
    Check(zero.Right()  == 0, "default Right  == 0");
    Check(zero.Top()    == 0, "default Top    == 0");
    Check(zero.Bottom() == 0, "default Bottom == 0");

    Margins m(10, 20, 30, 40);
    Check(m.Left()   == 10 && m.Right() == 20 &&
          m.Top()    == 30 && m.Bottom() == 40,
          "four-arg ctor round-trips");

    Margins s;
    s.Left(1); s.Right(2); s.Top(3); s.Bottom(4);
    Check(s.Left() == 1 && s.Right() == 2 &&
          s.Top()  == 3 && s.Bottom() == 4,
          "setters round-trip");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
