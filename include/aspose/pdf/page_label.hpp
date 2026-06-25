#pragma once

// =============================================================================
// Aspose::Pdf::PageLabel — a page-label range descriptor (PDF §12.4.2, a
// page-label dictionary). Mirrors canonical Aspose.Pdf.PageLabel (Aspose.PDF
// 26.4.0): a numbering style, a starting value, and an optional label prefix
// that together describe how a run of pages is numbered.
// =============================================================================

#include <string>

#include <aspose/pdf/numbering_style.hpp>

namespace Aspose::Pdf {

class PageLabel {
public:
    PageLabel();

    // Canonical PageLabel.StartingValue — the first numeric value of the
    // range (PDF /St; default 1).
    int StartingValue() const noexcept;
    void StartingValue(int value);

    // Canonical PageLabel.NumberingStyle — the numbering style (PDF /S).
    Aspose::Pdf::NumberingStyle NumberingStyle() const noexcept;
    void NumberingStyle(Aspose::Pdf::NumberingStyle value);

    // Canonical PageLabel.Prefix — the label prefix prepended to every page
    // in the range (PDF /P; default empty).
    const std::string& Prefix() const noexcept;
    void Prefix(std::string value);

private:
    int starting_value_ = 1;
    Aspose::Pdf::NumberingStyle numbering_style_ =
        Aspose::Pdf::NumberingStyle::NumeralsArabic;
    std::string prefix_;
};

}  // namespace Aspose::Pdf
