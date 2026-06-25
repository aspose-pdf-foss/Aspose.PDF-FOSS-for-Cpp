#pragma once

// =============================================================================
// Aspose::Pdf::SvgLoadOptions — load options that import an SVG file as a PDF
// via Document(filename, options). Mirrors canonical Aspose.Pdf.SvgLoadOptions
// (Aspose.PDF 26.4.0) as a strict subset: the default ctor and AdjustPageSize.
// The ConversionEngine field and PageInfo property are deferred.
//
// v1 conversion subset: the importer sizes the page to the SVG width / height
// (or viewBox) and renders the basic vector primitives (rect, line, circle,
// ellipse, polyline, polygon and the M/L/H/V/Z path subset) with their fill /
// stroke presentation attributes. Text, transforms, curves, gradients and CSS
// styling are deferred.
// =============================================================================

#include <aspose/pdf/load_options.hpp>

namespace Aspose::Pdf {

class SvgLoadOptions : public LoadOptions {
public:
    SvgLoadOptions();

    // Canonical SvgLoadOptions.AdjustPageSize — fit the page to the SVG bounds.
    bool AdjustPageSize() const noexcept;
    void AdjustPageSize(bool value) noexcept;

private:
    bool adjust_page_size_ = false;
};

}  // namespace Aspose::Pdf
