#include <aspose/pdf/margins.hpp>

namespace Aspose::Pdf::Devices {

Margins::Margins() = default;

Margins::Margins(int left, int right, int top, int bottom)
    : left_(left), right_(right), top_(top), bottom_(bottom) {}

int Margins::Left() const noexcept   { return left_; }
void Margins::Left(int v) noexcept   { left_ = v; }

int Margins::Right() const noexcept  { return right_; }
void Margins::Right(int v) noexcept  { right_ = v; }

int Margins::Top() const noexcept    { return top_; }
void Margins::Top(int v) noexcept    { top_ = v; }

int Margins::Bottom() const noexcept { return bottom_; }
void Margins::Bottom(int v) noexcept { bottom_ = v; }

}  // namespace Aspose::Pdf::Devices
