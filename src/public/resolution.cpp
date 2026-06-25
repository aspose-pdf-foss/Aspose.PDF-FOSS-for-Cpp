#include <aspose/pdf/resolution.hpp>

namespace Aspose::Pdf::Devices {

Resolution::Resolution(int value) : x_(value), y_(value) {}
Resolution::Resolution(int valueX, int valueY) : x_(valueX), y_(valueY) {}

int Resolution::X() const noexcept { return x_; }
void Resolution::X(int value) noexcept { x_ = value; }

int Resolution::Y() const noexcept { return y_; }
void Resolution::Y(int value) noexcept { y_ = value; }

}  // namespace Aspose::Pdf::Devices
