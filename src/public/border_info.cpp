// Body for Aspose::Pdf::BorderInfo — per-edge table border styling value type.

#include <aspose/pdf/border_info.hpp>

namespace Aspose::Pdf {

BorderInfo::BorderInfo() {
    // No edges drawn by default — zero every side's stroke width.
    left_.LineWidth(0.0f);
    right_.LineWidth(0.0f);
    top_.LineWidth(0.0f);
    bottom_.LineWidth(0.0f);
}

BorderInfo::BorderInfo(BorderSide borderSide) : BorderInfo() {
    GraphInfo info;
    info.LineWidth(1.0f);
    ApplySides(borderSide, info);
}

BorderInfo::BorderInfo(BorderSide borderSide,
                       const Aspose::Pdf::Color& borderColor)
    : BorderInfo() {
    GraphInfo info;
    info.LineWidth(1.0f);
    info.Color(borderColor);
    ApplySides(borderSide, info);
}

BorderInfo::BorderInfo(BorderSide borderSide, float borderWidth)
    : BorderInfo() {
    GraphInfo info;
    info.LineWidth(borderWidth);
    ApplySides(borderSide, info);
}

BorderInfo::BorderInfo(BorderSide borderSide, float borderWidth,
                       const Aspose::Pdf::Color& borderColor)
    : BorderInfo() {
    GraphInfo info;
    info.LineWidth(borderWidth);
    info.Color(borderColor);
    ApplySides(borderSide, info);
}

BorderInfo::BorderInfo(BorderSide borderSide, const GraphInfo& info)
    : BorderInfo() {
    ApplySides(borderSide, info);
}

void BorderInfo::ApplySides(BorderSide side, const GraphInfo& info) {
    if (HasSide(side, BorderSide::Left)) left_ = info;
    if (HasSide(side, BorderSide::Top)) top_ = info;
    if (HasSide(side, BorderSide::Right)) right_ = info;
    if (HasSide(side, BorderSide::Bottom)) bottom_ = info;
}

const GraphInfo& BorderInfo::Left() const noexcept { return left_; }
void BorderInfo::Left(const GraphInfo& value) { left_ = value; }
const GraphInfo& BorderInfo::Right() const noexcept { return right_; }
void BorderInfo::Right(const GraphInfo& value) { right_ = value; }
const GraphInfo& BorderInfo::Top() const noexcept { return top_; }
void BorderInfo::Top(const GraphInfo& value) { top_ = value; }
const GraphInfo& BorderInfo::Bottom() const noexcept { return bottom_; }
void BorderInfo::Bottom(const GraphInfo& value) { bottom_ = value; }

double BorderInfo::RoundedBorderRadius() const noexcept {
    return rounded_border_radius_;
}
void BorderInfo::RoundedBorderRadius(double value) {
    rounded_border_radius_ = value;
}

}  // namespace Aspose::Pdf
