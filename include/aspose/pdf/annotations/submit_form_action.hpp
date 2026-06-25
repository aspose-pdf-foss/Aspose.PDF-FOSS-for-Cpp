#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::SubmitFormAction — an action that submits form
// field values to a URL (PDF §12.7.5.2). Mirrors canonical
// Aspose.Pdf.Annotations.SubmitFormAction (Aspose.PDF 26.4.0): a PdfAction
// carrying the submission /Flags bitfield and the target /F file specification
// (Url). The public bit-flag constants are dropped on the cpp track (set the
// Flags int directly).
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/pdf_action.hpp>
#include <aspose/pdf/file_specification.hpp>

namespace Aspose::Pdf::Annotations {

class SubmitFormAction : public PdfAction {
public:
    SubmitFormAction();

    // Canonical SubmitFormAction.Flags — the submission flags bitfield.
    int Flags() const noexcept;
    void Flags(int value);

    // Canonical SubmitFormAction.Url — the submission target file spec.
    const Aspose::Pdf::FileSpecification& Url() const noexcept;
    void Url(const Aspose::Pdf::FileSpecification& value);

    std::string ToString() const override;

protected:
    std::unique_ptr<PdfAction> CloneAction() const override;

private:
    int flags_ = 0;
    Aspose::Pdf::FileSpecification url_;
};

}  // namespace Aspose::Pdf::Annotations
