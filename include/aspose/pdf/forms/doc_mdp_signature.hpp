#pragma once

// =============================================================================
// Aspose::Pdf::Forms::DocMDPSignature — Document MDP signature
// wrapper (PDF §12.8.2.2 — DocMDP). Pairs a Signature with the
// access-permission level (MDP /P entry). Mirrors canonical
// Aspose.Pdf.Forms.DocMDPSignature.
// =============================================================================

#include <aspose/pdf/forms/doc_mdp_access_permissions.hpp>
#include <aspose/pdf/forms/signature.hpp>

namespace Aspose::Pdf::Forms {

class DocMDPSignature {
public:
    DocMDPSignature(Signature& signature,
                    DocMDPAccessPermissions accessPermissions);

    DocMDPAccessPermissions AccessPermissions() const noexcept;

private:
    Signature* signature_ = nullptr;
    DocMDPAccessPermissions access_permissions_ =
        DocMDPAccessPermissions::NoChanges;
};

}  // namespace Aspose::Pdf::Forms
