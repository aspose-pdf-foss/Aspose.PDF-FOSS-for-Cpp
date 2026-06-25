// Bodies for the Aspose::Pdf::Drawing shape family: Shape (abstract base),
// Graph (container paragraph), and the concrete Line / Rectangle / Circle /
// Ellipse shapes. Mirrors canonical Aspose.Pdf.Drawing.*.

#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/drawing/shape.hpp>

#include <utility>
#include <vector>

namespace Aspose::Pdf::Drawing {

// ---- Shape -----------------------------------------------------------------

const Aspose::Pdf::GraphInfo& Shape::GraphInfo() const noexcept {
    return graph_info_;
}
void Shape::GraphInfo(const Aspose::Pdf::GraphInfo& value) {
    graph_info_ = value;
}
bool Shape::CheckBounds(double, double) const { return true; }

// ---- Line ------------------------------------------------------------------

Line::Line(std::vector<float> positionArray)
    : position_array_(std::move(positionArray)) {}

const std::vector<float>& Line::PositionArray() const noexcept {
    return position_array_;
}
void Line::PositionArray(std::vector<float> value) {
    position_array_ = std::move(value);
}
bool Line::CheckBounds(double cw, double ch) const {
    for (std::size_t i = 0; i + 1 < position_array_.size(); i += 2) {
        const double x = position_array_[i];
        const double y = position_array_[i + 1];
        if (x < 0 || y < 0 || x > cw || y > ch) return false;
    }
    return true;
}

// ---- Rectangle -------------------------------------------------------------

Rectangle::Rectangle(float left, float bottom, float width, float height)
    : left_(left), bottom_(bottom), width_(width), height_(height) {}

double Rectangle::RoundedCornerRadius() const noexcept {
    return rounded_corner_radius_;
}
void Rectangle::RoundedCornerRadius(double value) {
    rounded_corner_radius_ = value;
}
double Rectangle::Left() const noexcept { return left_; }
void Rectangle::Left(double value) { left_ = value; }
double Rectangle::Bottom() const noexcept { return bottom_; }
void Rectangle::Bottom(double value) { bottom_ = value; }
double Rectangle::Width() const noexcept { return width_; }
void Rectangle::Width(double value) { width_ = value; }
double Rectangle::Height() const noexcept { return height_; }
void Rectangle::Height(double value) { height_ = value; }
bool Rectangle::CheckBounds(double cw, double ch) const {
    return left_ >= 0 && bottom_ >= 0 && left_ + width_ <= cw &&
           bottom_ + height_ <= ch;
}

// ---- Circle ----------------------------------------------------------------

Circle::Circle(float posX, float posY, float radius)
    : pos_x_(posX), pos_y_(posY), radius_(radius) {}

double Circle::PosX() const noexcept { return pos_x_; }
void Circle::PosX(double value) { pos_x_ = value; }
double Circle::PosY() const noexcept { return pos_y_; }
void Circle::PosY(double value) { pos_y_ = value; }
double Circle::Radius() const noexcept { return radius_; }
void Circle::Radius(double value) { radius_ = value; }
bool Circle::CheckBounds(double cw, double ch) const {
    return pos_x_ - radius_ >= 0 && pos_y_ - radius_ >= 0 &&
           pos_x_ + radius_ <= cw && pos_y_ + radius_ <= ch;
}

// ---- Ellipse ---------------------------------------------------------------

Ellipse::Ellipse(double left, double bottom, double width, double height)
    : left_(left), bottom_(bottom), width_(width), height_(height) {}

double Ellipse::Left() const noexcept { return left_; }
void Ellipse::Left(double value) { left_ = value; }
double Ellipse::Bottom() const noexcept { return bottom_; }
void Ellipse::Bottom(double value) { bottom_ = value; }
double Ellipse::Width() const noexcept { return width_; }
void Ellipse::Width(double value) { width_ = value; }
double Ellipse::Height() const noexcept { return height_; }
void Ellipse::Height(double value) { height_ = value; }
bool Ellipse::CheckBounds(double cw, double ch) const {
    return left_ >= 0 && bottom_ >= 0 && left_ + width_ <= cw &&
           bottom_ + height_ <= ch;
}

// ---- Graph -----------------------------------------------------------------

Graph::Graph(double width, double height)
    : width_(width), height_(height) {}

std::vector<std::unique_ptr<Shape>>& Graph::Shapes() noexcept {
    return shapes_;
}
const std::vector<std::unique_ptr<Shape>>& Graph::Shapes() const noexcept {
    return shapes_;
}

const Aspose::Pdf::GraphInfo& Graph::GraphInfo() const noexcept {
    return graph_info_;
}
void Graph::GraphInfo(const Aspose::Pdf::GraphInfo& value) {
    graph_info_ = value;
}

const BorderInfo& Graph::Border() const noexcept { return border_; }
void Graph::Border(const BorderInfo& value) { border_ = value; }

bool Graph::IsChangePosition() const noexcept { return is_change_position_; }
void Graph::IsChangePosition(bool value) { is_change_position_ = value; }

double Graph::Left() const noexcept { return left_; }
void Graph::Left(double value) { left_ = value; }
double Graph::Top() const noexcept { return top_; }
void Graph::Top(double value) { top_ = value; }
double Graph::Width() const noexcept { return width_; }
void Graph::Width(double value) { width_ = value; }
double Graph::Height() const noexcept { return height_; }
void Graph::Height(double value) { height_ = value; }

}  // namespace Aspose::Pdf::Drawing
