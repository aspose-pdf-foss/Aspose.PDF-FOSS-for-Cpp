#pragma once

// =============================================================================
// Aspose::Pdf::Drawing::PatternColorSpace — base of the pattern "colours" a
// Color can carry instead of a flat RGB value (Color::PatternColorSpace).
// Mirrors canonical Aspose.Pdf.Drawing.PatternColorSpace (Aspose.PDF 26.4.0).
// The concrete kinds are GradientAxialShading and GradientRadialShading.
// =============================================================================

namespace Aspose::Pdf::Drawing {

class PatternColorSpace {
public:
    virtual ~PatternColorSpace() = default;

protected:
    PatternColorSpace() = default;
    PatternColorSpace(const PatternColorSpace&) = default;
    PatternColorSpace& operator=(const PatternColorSpace&) = default;
};

}  // namespace Aspose::Pdf::Drawing
