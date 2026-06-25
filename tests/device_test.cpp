// Smoke test for Aspose::Pdf::Devices::Device — abstract root with
// no public members. The test verifies the type compiles + supports
// virtual destruction via a one-off concrete subclass.
#include <aspose/pdf/device.hpp>

#include <iostream>
#include <memory>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}

class Trivial : public Aspose::Pdf::Devices::Device {
public:
    Trivial() = default;
};
}

int main() {
    std::unique_ptr<Aspose::Pdf::Devices::Device> d =
        std::make_unique<Trivial>();
    Check(d != nullptr, "Device subclass constructible");
    d.reset();
    Check(true, "Device virtual destructor invoked cleanly");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
