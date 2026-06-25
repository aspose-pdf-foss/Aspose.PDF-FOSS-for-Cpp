#include <aspose/pdf/forms/signature_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

SignatureField::SignatureField(Aspose::Pdf::Page& /*page*/,
                                Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

SignatureField::SignatureField(Aspose::Pdf::Document& doc,
                                Aspose::Pdf::Rectangle rect)
    : Field(doc) {
    Rect(std::move(rect));
}

void SignatureField::Sign(Forms::Signature& signature) {
    signature_ = &signature;
}

Forms::Signature* SignatureField::Signature() const noexcept {
    return signature_;
}

}  // namespace Aspose::Pdf::Forms
