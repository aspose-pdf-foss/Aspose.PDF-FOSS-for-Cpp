#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::JavascriptAction — an action that runs an
// ECMAScript (JavaScript) program (PDF §12.6.4.16). Mirrors canonical
// Aspose.Pdf.Annotations.JavascriptAction (Aspose.PDF 26.4.0): a PdfAction
// carrying the script source.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/pdf_action.hpp>

namespace Aspose::Pdf::Annotations {

class JavascriptAction : public PdfAction {
public:
    explicit JavascriptAction(std::string javaScript);

    // Canonical JavascriptAction.Script — the script source.
    const std::string& Script() const noexcept;
    void Script(std::string value);

    // The action's ECMAScript representation — the script itself.
    std::string GetECMAScriptString() const override;
    std::string ToString() const override;

protected:
    std::unique_ptr<PdfAction> CloneAction() const override;

private:
    std::string script_;
};

}  // namespace Aspose::Pdf::Annotations
