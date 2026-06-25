#pragma once

// =============================================================================
// Aspose::Pdf::Text::Position — a text-placement coordinate. Mirrors canonical
// Aspose.Pdf.Text.Position (Aspose.PDF 26.4.0) as a strict subset: XIndent /
// YIndent in user-space points, used by TextFragment::Position / TextBuilder.
// =============================================================================

namespace Aspose::Pdf::Text {

class Position {
public:
    Position(double xIndent, double yIndent) noexcept;

    double XIndent() const noexcept;
    void XIndent(double value) noexcept;

    double YIndent() const noexcept;
    void YIndent(double value) noexcept;

private:
    double x_indent_ = 0.0;
    double y_indent_ = 0.0;
};

}  // namespace Aspose::Pdf::Text
