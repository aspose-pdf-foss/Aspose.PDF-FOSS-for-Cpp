// Smoke test for Aspose::Pdf::Devices::PageDevice — abstract base
// for single-page devices. Verifies the override mechanism: a
// subclass can override the abstract Process(Page, Stream); the
// concrete file-path overload is inherited as-is.
//
// End-to-end Process(Page, …) is exercised by the concrete-leaf
// smoke tests (png_device_test, jpeg_device_test, …) which build
// a real Document and a real Page.
#include <aspose/pdf/page_device.hpp>

#include <iostream>
#include <ostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}

class StubDevice : public Aspose::Pdf::Devices::PageDevice {
public:
    int n_stream_calls = 0;
    using PageDevice::Process;
    void Process(const ::Aspose::Pdf::Page& /*page*/,
                 std::ostream& /*output*/) override {
        ++n_stream_calls;
    }
};
}

int main() {
    StubDevice dev;
    Check(dev.n_stream_calls == 0, "fresh device counter starts at 0");

    Aspose::Pdf::Devices::PageDevice* upcast = &dev;
    Check(upcast != nullptr, "subclass upcasts to PageDevice*");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
