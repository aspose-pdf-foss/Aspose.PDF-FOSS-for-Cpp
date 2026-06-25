#include <aspose/pdf/annotations/characteristics.hpp>

namespace Aspose::Pdf::Annotations {

Aspose::Pdf::Rotation Characteristics::Rotate() const noexcept {
    return rotate_;
}
void Characteristics::Rotate(Aspose::Pdf::Rotation v) noexcept {
    rotate_ = v;
}

}  // namespace Aspose::Pdf::Annotations
