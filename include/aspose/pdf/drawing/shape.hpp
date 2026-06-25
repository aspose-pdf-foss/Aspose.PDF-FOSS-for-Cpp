#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::Shape — abstract base for graph shapes (PDF vector
// graphics). Mirrors canonical Aspose.Pdf.Drawing.Shape (Aspose.PDF 26.4.0):
// every shape carries a GraphInfo (stroke/fill) and a bounds check. Concrete
// shapes (Line, Rectangle, Circle, Ellipse) derive from it.
// =============================================================================

#include <aspose/pdf/graph_info.hpp>

namespace Aspose::Pdf::Drawing {

class Shape {
public:
    virtual ~Shape() = default;

    // Canonical Shape.GraphInfo — stroke / fill styling.
    const Aspose::Pdf::GraphInfo& GraphInfo() const noexcept;
    void GraphInfo(const Aspose::Pdf::GraphInfo& value);

    // Canonical Shape.CheckBounds — whether the shape fits the given container
    // dimensions. Base implementation accepts any bounds; concrete shapes may
    // refine it.
    virtual bool CheckBounds(double containerWidth,
                             double containerHeight) const;

protected:
    Shape() = default;

    Aspose::Pdf::GraphInfo graph_info_;
};

}  // namespace Aspose::Pdf::Drawing
