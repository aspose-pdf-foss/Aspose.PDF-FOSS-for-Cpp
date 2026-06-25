// Smoke test for Aspose::Pdf::Devices::DocumentDevice — abstract
// base for multi-page document devices. Verifies the override
// mechanism for the abstract Process(Document, fromPage, toPage,
// Stream) and inheritance from PageDevice.
//
// End-to-end Process(Document, …) is exercised by tiff_device_test
// (the only concrete leaf in v1).
#include <aspose/pdf/document_device.hpp>
#include <aspose/pdf/page_device.hpp>

#include <iostream>
#include <ostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}

class StubDocDevice : public Aspose::Pdf::Devices::DocumentDevice {
public:
    using DocumentDevice::Process;
    void Process(const ::Aspose::Pdf::Document& /*document*/,
                 int /*fromPage*/, int /*toPage*/,
                 std::ostream& /*output*/) override {}
    void Process(const ::Aspose::Pdf::Page& /*page*/,
                 std::ostream& /*output*/) override {}
};
}

int main() {
    StubDocDevice dev;
    Aspose::Pdf::Devices::PageDevice* page_base = &dev;
    Check(page_base != nullptr, "DocumentDevice IS-A PageDevice");

    Aspose::Pdf::Devices::DocumentDevice* doc_base = &dev;
    Check(doc_base != nullptr, "subclass upcasts to DocumentDevice*");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
