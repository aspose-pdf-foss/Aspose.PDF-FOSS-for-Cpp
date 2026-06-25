#pragma once

// =============================================================================
// Aspose::Pdf::Facades::AlignmentType — horizontal-alignment
// selector for FormattedText / PdfFileStamp. Type-safe enum
// class with named static instances (canonical declares as a
// class with public static readonly fields). Mirrors canonical
// Aspose.Pdf.Facades.AlignmentType.
// =============================================================================

#include <string>

namespace Aspose::Pdf::Facades {

class AlignmentType {
public:
    explicit AlignmentType(std::string name);

    static const AlignmentType& Left();
    static const AlignmentType& Center();
    static const AlignmentType& Right();

    const std::string& ToString() const noexcept;

    bool operator==(const AlignmentType& other) const noexcept;
    bool operator!=(const AlignmentType& other) const noexcept;

private:
    std::string name_;
};

}  // namespace Aspose::Pdf::Facades
