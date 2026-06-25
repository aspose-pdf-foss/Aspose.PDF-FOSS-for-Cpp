#pragma once

// =============================================================================
// Aspose::Pdf::BorderInfo — the per-edge border styling of a table, row or cell
// (PDF table model). Mirrors canonical Aspose.Pdf.BorderInfo (Aspose.PDF
// 26.4.0): a GraphInfo for each of the four edges plus a rounded-corner radius.
// The constructors configure the edge(s) named by a BorderSide. A copyable
// value type.
// =============================================================================

#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/graph_info.hpp>

namespace Aspose::Pdf {

class BorderInfo {
public:
    // No edges drawn (every side's line width 0).
    BorderInfo();
    // Configure the edge(s) in `borderSide` with a default 1pt stroke.
    explicit BorderInfo(BorderSide borderSide);
    BorderInfo(BorderSide borderSide, const Aspose::Pdf::Color& borderColor);
    BorderInfo(BorderSide borderSide, float borderWidth);
    BorderInfo(BorderSide borderSide, float borderWidth,
               const Aspose::Pdf::Color& borderColor);
    BorderInfo(BorderSide borderSide, const GraphInfo& info);

    // Canonical per-edge GraphInfo accessors (get/set).
    const GraphInfo& Left() const noexcept;
    void Left(const GraphInfo& value);
    const GraphInfo& Right() const noexcept;
    void Right(const GraphInfo& value);
    const GraphInfo& Top() const noexcept;
    void Top(const GraphInfo& value);
    const GraphInfo& Bottom() const noexcept;
    void Bottom(const GraphInfo& value);

    // Canonical BorderInfo.RoundedBorderRadius — corner radius in points.
    double RoundedBorderRadius() const noexcept;
    void RoundedBorderRadius(double value);

private:
    void ApplySides(BorderSide side, const GraphInfo& info);

    GraphInfo left_;
    GraphInfo right_;
    GraphInfo top_;
    GraphInfo bottom_;
    double rounded_border_radius_ = 0.0;
};

}  // namespace Aspose::Pdf
