#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

namespace foundation::transform {

struct Matrix {
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
};

struct Point {
    double x;
    double y;
};

struct Rect {
    double xmin;
    double ymin;
    double xmax;
    double ymax;
};

constexpr Matrix Identity() noexcept {
    return Matrix{1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
}

Matrix Translate(double tx, double ty) noexcept;
Matrix Scale(double sx, double sy) noexcept;
Matrix Rotate(double radians) noexcept;
Matrix Skew(double alpha_x_radians, double beta_y_radians) noexcept;
Matrix Compose(const Matrix& lhs, const Matrix& rhs) noexcept;
std::optional<Matrix> Inverse(const Matrix& m) noexcept;
Point Apply(const Matrix& m, const Point& p) noexcept;
Rect ApplyToRect(const Matrix& m, const Rect& r) noexcept;
Matrix Viewport(const Rect& media_box, int rotate_degrees,
                std::uint32_t pixel_width, std::uint32_t pixel_height);

}
