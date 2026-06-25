#pragma once

// =============================================================================
// Aspose::Pdf::Facades::Facade — concrete base for the facade
// hierarchy. Holds the bound Document and implements the IFacade
// surface. Mirrors canonical Aspose.Pdf.Facades.Facade.
//
// Phased drops:
//   * BindPdf(Stream) — Stream cascade
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/facades/i_facade.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class Facade : public virtual IFacade {
public:
    Facade() noexcept = default;
    ~Facade() override = default;

    // ---- IFacade ----

    void BindPdf(const std::string& srcFile) override;
    void BindPdf(Aspose::Pdf::Document& srcDoc) override;
    void Close() override;

    // IDisposable surface. v1 Dispose() chains to Close().
    void Dispose();

    Aspose::Pdf::Document* Document() noexcept;
    const Aspose::Pdf::Document* Document() const noexcept;

protected:
    Aspose::Pdf::Document* document_ = nullptr;

private:
    // When the facade opened the document itself (via BindPdf
    // path), it owns the lifetime and frees on Close.
    std::unique_ptr<Aspose::Pdf::Document> owned_;
};

}  // namespace Aspose::Pdf::Facades
