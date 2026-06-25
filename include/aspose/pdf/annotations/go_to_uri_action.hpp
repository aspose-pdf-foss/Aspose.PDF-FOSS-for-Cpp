#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::GoToURIAction — a hyperlink (URI) action.
// Mirrors canonical Aspose.Pdf.Annotations.GoToURIAction (Aspose.PDF 26.4.0):
// a PdfAction carrying a target URI string. Used as LinkAnnotation::Action to
// make a link open an external resource.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/pdf_action.hpp>

namespace Aspose::Pdf::Annotations {

class GoToURIAction : public PdfAction {
public:
    explicit GoToURIAction(std::string uri);

    // Canonical GoToURIAction.URI — the target URI.
    const std::string& URI() const noexcept;
    void URI(std::string value);

    std::string GetECMAScriptString() const override;
    std::string ToString() const override;

protected:
    std::unique_ptr<PdfAction> CloneAction() const override;

private:
    std::string uri_;
};

}  // namespace Aspose::Pdf::Annotations
