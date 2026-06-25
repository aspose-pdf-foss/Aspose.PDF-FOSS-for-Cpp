#pragma once

// =============================================================================
// Aspose::Pdf::Facades::VerticalAlignmentType — vertical-alignment
// selector for FormattedText / PdfFileStamp. Type-safe enum
// class with named static instances. Mirrors canonical
// Aspose.Pdf.Facades.VerticalAlignmentType.
// =============================================================================

#include <string>

namespace Aspose::Pdf::Facades {

class VerticalAlignmentType {
public:
    explicit VerticalAlignmentType(std::string name);

    static const VerticalAlignmentType& Top();
    static const VerticalAlignmentType& Center();
    static const VerticalAlignmentType& Bottom();

    const std::string& ToString() const noexcept;

    bool operator==(const VerticalAlignmentType& other) const noexcept;
    bool operator!=(const VerticalAlignmentType& other) const noexcept;

private:
    std::string name_;
};

}  // namespace Aspose::Pdf::Facades
