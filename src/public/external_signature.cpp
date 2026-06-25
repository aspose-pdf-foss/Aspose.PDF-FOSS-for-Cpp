#include <aspose/pdf/forms/external_signature.hpp>

namespace Aspose::Pdf::Forms {

ExternalSignature::ExternalSignature(const std::string& base64,
                                      bool detached)
    : base64_(base64),
      detached_(detached) {}

}  // namespace Aspose::Pdf::Forms
