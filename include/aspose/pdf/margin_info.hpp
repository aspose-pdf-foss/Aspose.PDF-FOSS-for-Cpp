#pragma once

// =============================================================================
// Aspose::Pdf::MarginInfo — page / paragraph margin carrier
// consumed by BaseParagraph::Margin. Mirrors canonical
// Aspose.Pdf.MarginInfo.
//
// Implements System.ICloneable via copy construction (the
// canonical Clone() returning System.Object is dropped per cpp
// drops.yaml).
// =============================================================================

namespace Aspose::Pdf {

class MarginInfo {
public:
    MarginInfo() noexcept = default;
    MarginInfo(double left, double bottom,
               double right, double top) noexcept;

    double Left() const noexcept;
    void Left(double value) noexcept;

    double Right() const noexcept;
    void Right(double value) noexcept;

    double Top() const noexcept;
    void Top(double value) noexcept;

    double Bottom() const noexcept;
    void Bottom(double value) noexcept;

private:
    double left_ = 0.0;
    double right_ = 0.0;
    double top_ = 0.0;
    double bottom_ = 0.0;
};

}  // namespace Aspose::Pdf
