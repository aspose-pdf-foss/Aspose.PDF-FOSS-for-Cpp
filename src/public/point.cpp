#include <aspose/pdf/point.hpp>

#include <cmath>

namespace Aspose::Pdf {

Point::Point(double x, double y) noexcept : x_(x), y_(y) {}

double Point::X() const noexcept { return x_; }
void Point::X(double v) noexcept { x_ = v; }

double Point::Y() const noexcept { return y_; }
void Point::Y(double v) noexcept { y_ = v; }

double Point::Distance(const Point& p1, const Point& p2) noexcept {
    const double dx = p1.x_ - p2.x_;
    const double dy = p1.y_ - p2.y_;
    return std::sqrt(dx * dx + dy * dy);
}

Point Point::Trivial() { return Point{0.0, 0.0}; }

}  // namespace Aspose::Pdf
