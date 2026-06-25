// Smoke test for Aspose::Pdf::Devices::ImageDevice — abstract
// raster-image base. Carries width / height / resolution /
// coordinate-type / rendering-options / form-presentation-mode
// state inherited by PngDevice / JpegDevice / BmpDevice.
//
// Verifies state round-trips through a minimal in-test subclass
// without instantiating any concrete leaf.
#include <aspose/pdf/form_presentation_mode.hpp>
#include <aspose/pdf/image_device.hpp>
#include <aspose/pdf/page_coordinate_type.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/rendering_options.hpp>
#include <aspose/pdf/resolution.hpp>

#include <iostream>
#include <ostream>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}

class StubImageDevice : public Aspose::Pdf::Devices::ImageDevice {
public:
    using ImageDevice::ImageDevice;
    using ImageDevice::Process;
    void Process(const ::Aspose::Pdf::Page& /*page*/,
                 std::ostream& /*output*/) override {}
};
}

int main() {
    using Aspose::Pdf::Devices::Resolution;
    using Aspose::Pdf::PageCoordinateType;
    using Aspose::Pdf::Devices::FormPresentationMode;

    StubImageDevice def;
    Check(def.Width() == 0, "default Width");
    Check(def.Height() == 0, "default Height");

    StubImageDevice sized(800, 600, Resolution(150));
    Check(sized.Width() == 800, "sized Width");
    Check(sized.Height() == 600, "sized Height");
    Check(sized.Resolution().X() == 150, "sized Resolution.X");

    sized.CoordinateType(PageCoordinateType::CropBox);
    Check(sized.CoordinateType() == PageCoordinateType::CropBox,
          "CoordinateType setter round-trips");

    sized.FormPresentationMode(FormPresentationMode::Editor);
    Check(sized.FormPresentationMode() == FormPresentationMode::Editor,
          "FormPresentationMode setter round-trips");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
