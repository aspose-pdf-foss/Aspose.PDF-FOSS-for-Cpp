#pragma once

// =============================================================================
// Aspose::Pdf::Facades::IFacade — common interface implemented by
// every Facade-pattern wrapper (Bind PDF + Close). Mirrors
// canonical Aspose.Pdf.Facades.IFacade.
//
// Phased drops:
//   * BindPdf(Stream) — Stream cascade (auto-drop)
// =============================================================================

#include <string>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class IFacade {
public:
    virtual ~IFacade() = default;

    virtual void BindPdf(const std::string& srcFile) = 0;
    virtual void BindPdf(Aspose::Pdf::Document& srcDoc) = 0;
    virtual void Close() = 0;
};

}  // namespace Aspose::Pdf::Facades
