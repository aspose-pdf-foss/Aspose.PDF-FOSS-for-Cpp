#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::NamedAction — a predefined viewer action invoked by
// name (PDF §12.6.4.11). Mirrors canonical Aspose.Pdf.Annotations.NamedAction
// (Aspose.PDF 26.4.0): a PdfAction carrying a /N action name, constructed from
// a PredefinedAction.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/pdf_action.hpp>
#include <aspose/pdf/annotations/predefined_action.hpp>

namespace Aspose::Pdf::Annotations {

class NamedAction : public PdfAction {
public:
    explicit NamedAction(PredefinedAction action);

    // Canonical NamedAction.Name — the /N action name.
    const std::string& Name() const noexcept;
    void Name(std::string value);

    std::string ToString() const override;

protected:
    std::unique_ptr<PdfAction> CloneAction() const override;

private:
    std::string name_;
};

}  // namespace Aspose::Pdf::Annotations
