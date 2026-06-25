// Body for Aspose::Pdf::GraphInfo — stroke / fill styling value type.

#include <aspose/pdf/graph_info.hpp>

#include <utility>
#include <vector>

namespace Aspose::Pdf {

double GraphInfo::X() const noexcept { return x_; }
double GraphInfo::Y() const noexcept { return y_; }

float GraphInfo::LineWidth() const noexcept { return line_width_; }
void GraphInfo::LineWidth(float value) { line_width_ = value; }

const Aspose::Pdf::Color& GraphInfo::Color() const noexcept { return color_; }
void GraphInfo::Color(const Aspose::Pdf::Color& value) { color_ = value; }

const Aspose::Pdf::Color& GraphInfo::FillColor() const noexcept {
    return fill_color_;
}
void GraphInfo::FillColor(const Aspose::Pdf::Color& value) {
    fill_color_ = value;
}

const std::vector<int>& GraphInfo::DashArray() const noexcept {
    return dash_array_;
}
void GraphInfo::DashArray(std::vector<int> value) {
    dash_array_ = std::move(value);
}

int GraphInfo::DashPhase() const noexcept { return dash_phase_; }
void GraphInfo::DashPhase(int value) { dash_phase_ = value; }

bool GraphInfo::IsDoubled() const noexcept { return is_doubled_; }
void GraphInfo::IsDoubled(bool value) { is_doubled_ = value; }

double GraphInfo::SkewAngleX() const noexcept { return skew_angle_x_; }
void GraphInfo::SkewAngleX(double value) { skew_angle_x_ = value; }
double GraphInfo::SkewAngleY() const noexcept { return skew_angle_y_; }
void GraphInfo::SkewAngleY(double value) { skew_angle_y_ = value; }
double GraphInfo::ScalingRateX() const noexcept { return scaling_rate_x_; }
void GraphInfo::ScalingRateX(double value) { scaling_rate_x_ = value; }
double GraphInfo::ScalingRateY() const noexcept { return scaling_rate_y_; }
void GraphInfo::ScalingRateY(double value) { scaling_rate_y_ = value; }
double GraphInfo::RotationAngle() const noexcept { return rotation_angle_; }
void GraphInfo::RotationAngle(double value) { rotation_angle_ = value; }

}  // namespace Aspose::Pdf
