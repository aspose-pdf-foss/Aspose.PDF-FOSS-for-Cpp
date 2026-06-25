#include <aspose/pdf/forms/doc_mdp_signature.hpp>

namespace Aspose::Pdf::Forms {

DocMDPSignature::DocMDPSignature(
        Signature& signature,
        DocMDPAccessPermissions accessPermissions)
    : signature_(&signature),
      access_permissions_(accessPermissions) {}

DocMDPAccessPermissions DocMDPSignature::AccessPermissions() const noexcept {
    return access_permissions_;
}

}  // namespace Aspose::Pdf::Forms
