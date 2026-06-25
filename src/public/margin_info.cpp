#include <aspose/pdf/margin_info.hpp>

namespace Aspose::Pdf {

MarginInfo::MarginInfo(double left, double bottom,
                       double right, double top) noexcept
    : left_(left), right_(right), top_(top), bottom_(bottom) {}

double MarginInfo::Left() const noexcept   { return left_; }
void MarginInfo::Left(double v) noexcept   { left_ = v; }

double MarginInfo::Right() const noexcept  { return right_; }
void MarginInfo::Right(double v) noexcept  { right_ = v; }

double MarginInfo::Top() const noexcept    { return top_; }
void MarginInfo::Top(double v) noexcept    { top_ = v; }

double MarginInfo::Bottom() const noexcept { return bottom_; }
void MarginInfo::Bottom(double v) noexcept { bottom_ = v; }

}  // namespace Aspose::Pdf
