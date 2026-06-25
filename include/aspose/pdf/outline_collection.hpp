#pragma once

// =============================================================================
// Aspose::Pdf::OutlineCollection — the document-level outline (bookmark) root
// (PDF §12.3.3 — the Catalog's /Outlines dictionary). Mirrors canonical
// Aspose.Pdf.OutlineCollection (Aspose.PDF 26.4.0): the top of the outline
// tree, returned by Document::Outlines(). All collection operations are
// inherited from Outlines; OutlineCollection is the concrete root type.
// =============================================================================

#include <aspose/pdf/outlines.hpp>

namespace Aspose::Pdf {

class OutlineCollection : public Outlines {
public:
    OutlineCollection();
    ~OutlineCollection() override;

private:
    friend class Document;
};

}  // namespace Aspose::Pdf
