#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Graph — a graph paragraph holding vector shapes (PDF
// vector graphics). Mirrors canonical Aspose.Pdf.Drawing.Graph (Aspose.PDF
// 26.4.0): a BaseParagraph with a shape list, default GraphInfo / border and
// placement. Rendered into the page content stream at Save (via
// Page.Paragraphs).
// =============================================================================

#include <memory>
#include <vector>

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/drawing/shape.hpp>
#include <aspose/pdf/graph_info.hpp>

namespace Aspose::Pdf::Drawing {

class Graph : public Aspose::Pdf::BaseParagraph {
public:
    Graph(double width, double height);

    // Canonical Graph.Shapes — the contained shapes (owned). Add shapes with
    // Shapes().push_back(std::make_unique<Line>(…)).
    std::vector<std::unique_ptr<Shape>>& Shapes() noexcept;
    const std::vector<std::unique_ptr<Shape>>& Shapes() const noexcept;

    // Canonical Graph.GraphInfo — default styling for the graph.
    const Aspose::Pdf::GraphInfo& GraphInfo() const noexcept;
    void GraphInfo(const Aspose::Pdf::GraphInfo& value);

    // Canonical Graph.Border.
    const BorderInfo& Border() const noexcept;
    void Border(const BorderInfo& value);

    // Canonical Graph.IsChangePosition.
    bool IsChangePosition() const noexcept;
    void IsChangePosition(bool value);

    // Canonical Graph.Left / Top placement.
    double Left() const noexcept;
    void Left(double value);
    double Top() const noexcept;
    void Top(double value);

    // Canonical Graph.Width / Height.
    double Width() const noexcept;
    void Width(double value);
    double Height() const noexcept;
    void Height(double value);

private:
    std::vector<std::unique_ptr<Shape>> shapes_;
    Aspose::Pdf::GraphInfo graph_info_;
    BorderInfo border_;
    bool is_change_position_ = false;
    double left_ = 0.0;
    double top_ = 0.0;
    double width_;
    double height_;
};

}  // namespace Aspose::Pdf::Drawing
