#include <aspose/pdf/rectangle.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Aspose::Pdf {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

// Normalise (llx, lly, urx, ury) so that LLX <= URX and LLY <= URY.
void Normalise(double& llx, double& lly,
                double& urx, double& ury) noexcept {
    if (llx > urx) std::swap(llx, urx);
    if (lly > ury) std::swap(lly, ury);
}

}  // namespace

Rectangle::Rectangle(double llx, double lly,
                     double urx, double ury,
                     bool normalizeCoordinates) noexcept
    : llx_(llx), lly_(lly), urx_(urx), ury_(ury) {
    if (normalizeCoordinates) {
        Normalise(llx_, lly_, urx_, ury_);
    }
}

bool Rectangle::Equals(const Rectangle& other) const noexcept {
    return llx_ == other.llx_ && lly_ == other.lly_
        && urx_ == other.urx_ && ury_ == other.ury_;
}

bool Rectangle::NearEquals(const Rectangle& other,
                            double delta) const noexcept {
    return std::abs(llx_ - other.llx_) <= delta
        && std::abs(lly_ - other.lly_) <= delta
        && std::abs(urx_ - other.urx_) <= delta
        && std::abs(ury_ - other.ury_) <= delta;
}

Rectangle Rectangle::Intersect(
        const Rectangle& other) const noexcept {
    const double l = std::max(llx_, other.llx_);
    const double b = std::max(lly_, other.lly_);
    const double r = std::min(urx_, other.urx_);
    const double t = std::min(ury_, other.ury_);
    if (l > r || b > t) return Rectangle::Empty();
    return Rectangle{l, b, r, t, /*normalizeCoordinates=*/false};
}

Rectangle Rectangle::Join(
        const Rectangle& other) const noexcept {
    return Rectangle{
        std::min(llx_, other.llx_),
        std::min(lly_, other.lly_),
        std::max(urx_, other.urx_),
        std::max(ury_, other.ury_),
        /*normalizeCoordinates=*/false};
}

bool Rectangle::IsIntersect(
        const Rectangle& other) const noexcept {
    return !(other.urx_ < llx_ || other.llx_ > urx_
          || other.ury_ < lly_ || other.lly_ > ury_);
}

bool Rectangle::Contains(const Point& p,
                          bool inclusive) const noexcept {
    if (inclusive) {
        return p.X() >= llx_ && p.X() <= urx_
            && p.Y() >= lly_ && p.Y() <= ury_;
    }
    return p.X() > llx_ && p.X() < urx_
        && p.Y() > lly_ && p.Y() < ury_;
}

bool Rectangle::ContainsPoint(double x, double y) const noexcept {
    return x >= llx_ && x <= urx_ && y >= lly_ && y <= ury_;
}

bool Rectangle::ContainsLine(double x1, double y1,
                              double x2, double y2) const noexcept {
    return ContainsPoint(x1, y1) && ContainsPoint(x2, y2);
}

Point Rectangle::Center() const noexcept {
    return Point{(llx_ + urx_) / 2.0, (lly_ + ury_) / 2.0};
}

void Rectangle::Rotate(Rotation angle) noexcept {
    // Rotate the corners around the rectangle's center. The
    // bounding-box shape only changes when angle is 90 or 270
    // (the width/height swap).
    const double cx = (llx_ + urx_) / 2.0;
    const double cy = (lly_ + ury_) / 2.0;
    const double hw = (urx_ - llx_) / 2.0;
    const double hh = (ury_ - lly_) / 2.0;
    switch (angle) {
        case Rotation::None:
            return;
        case Rotation::on90:
        case Rotation::on270:
            llx_ = cx - hh;
            urx_ = cx + hh;
            lly_ = cy - hw;
            ury_ = cy + hw;
            return;
        case Rotation::on180:
            return;
    }
}

void Rectangle::Rotate(int angle) noexcept {
    // Normalise the integer angle into the Rotation enum's
    // 0/90/180/270 buckets.
    int a = angle % 360;
    if (a < 0) a += 360;
    Rotation r = Rotation::None;
    if (a == 90)       r = Rotation::on90;
    else if (a == 180) r = Rotation::on180;
    else if (a == 270) r = Rotation::on270;
    Rotate(r);
}

std::vector<Point> Rectangle::ToPoints() const {
    return {
        Point{llx_, lly_},
        Point{urx_, lly_},
        Point{urx_, ury_},
        Point{llx_, ury_},
    };
}

void Rectangle::MoveBy(double dx, double dy) noexcept {
    llx_ += dx;
    urx_ += dx;
    lly_ += dy;
    ury_ += dy;
}

double Rectangle::Width() const noexcept { return urx_ - llx_; }
double Rectangle::Height() const noexcept { return ury_ - lly_; }

double Rectangle::LLX() const noexcept { return llx_; }
void Rectangle::LLX(double v) noexcept { llx_ = v; }

double Rectangle::LLY() const noexcept { return lly_; }
void Rectangle::LLY(double v) noexcept { lly_ = v; }

double Rectangle::URX() const noexcept { return urx_; }
void Rectangle::URX(double v) noexcept { urx_ = v; }

double Rectangle::URY() const noexcept { return ury_; }
void Rectangle::URY(double v) noexcept { ury_ = v; }

Rectangle Rectangle::Empty() {
    // Canonical "empty rectangle" — the conventional sentinel
    // value with +inf lower-left and -inf upper-right so any
    // Union with a real rect collapses to that rect.
    return Rectangle{kInf, kInf, -kInf, -kInf,
                     /*normalizeCoordinates=*/false};
}

Rectangle Rectangle::Trivial() {
    // Canonical "trivial rectangle" — the origin-anchored unit.
    return Rectangle{0.0, 0.0, 0.0, 0.0,
                     /*normalizeCoordinates=*/false};
}

bool Rectangle::IsTrivial() const noexcept {
    return llx_ == 0.0 && lly_ == 0.0
        && urx_ == 0.0 && ury_ == 0.0;
}

bool Rectangle::IsEmpty() const noexcept {
    return llx_ > urx_ || lly_ > ury_;
}

bool Rectangle::IsPoint() const noexcept {
    return llx_ == urx_ && lly_ == ury_;
}

}  // namespace Aspose::Pdf
