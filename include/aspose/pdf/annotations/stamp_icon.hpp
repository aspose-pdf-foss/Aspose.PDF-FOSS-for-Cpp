#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::StampIcon — rubber-stamp icon selector
// for StampAnnotation (PDF /Subtype /Stamp /Name entry per
// §12.5.6.13). Mirrors canonical Aspose.Pdf.Annotations.StampIcon.
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class StampIcon : int {
    Draft = 0,
    Approved = 1,
    Experimental = 2,
    NotApproved = 3,
    AsIs = 4,
    Expired = 5,
    NotForPublicRelease = 6,
    Confidential = 7,
    Final = 8,
    Sold = 9,
    Departmental = 10,
    ForComment = 11,
    ForPublicRelease = 12,
    TopSecret = 13,
};

}  // namespace Aspose::Pdf::Annotations
