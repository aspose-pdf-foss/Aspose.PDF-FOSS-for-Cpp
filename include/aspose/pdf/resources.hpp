#pragma once

// =============================================================================
// Aspose::Pdf::Resources — a page's resource dictionary (PDF §7.8.3). Mirrors
// canonical Aspose.Pdf.Resources (Aspose.PDF 26.4.0) as the image-resource
// subset: Images returns the page's XImageCollection. Obtained via
// Page.Resources().
// =============================================================================

#include <aspose/pdf/x_image_collection.hpp>

namespace Aspose::Pdf {

class Resources {
public:
    Resources();
    ~Resources();
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;

    // Canonical Resources.Images — the page's embedded images.
    XImageCollection& Images() noexcept;

private:
    XImageCollection images_;

    friend class Document;
};

}  // namespace Aspose::Pdf
