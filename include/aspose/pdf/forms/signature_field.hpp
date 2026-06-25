#pragma once

// =============================================================================
// Aspose::Pdf::Forms::SignatureField — digital-signature form field
// (PDF §12.8 — digital signatures). Mirrors canonical
// Aspose.Pdf.Forms.SignatureField; extends Field.
//
// The Sign / Signature surface refers to the Aspose::Pdf::Forms::
// Signature class hierarchy (Signature abstract + PKCS1 / PKCS7 /
// PKCS7Detached / DocMDPSignature / ExternalSignature). That
// cluster lands at F6 — for now Signature is forward-declared,
// the bodies are stubs.
//
// Phased drops on this beat:
//   * Sign(Signature, Stream, string) — Stream cascade
//   * ExtractImage() / ExtractImage(ImageFormat) — Stream cascade +
//     ImageFormat cascade
//   * ExtractCertificate() — Stream cascade
//   * ExtractCertificateObject() — X509Certificate2 cascade
// =============================================================================

#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class Signature;  // F6

class SignatureField : public Field {
public:
    SignatureField(Aspose::Pdf::Page& page,
                   Aspose::Pdf::Rectangle rect);
    SignatureField(Aspose::Pdf::Document& doc,
                   Aspose::Pdf::Rectangle rect);

    // Apply a Signature object as the field's /V dict entry.
    // v1 stores the pointer; actual cryptographic signing is
    // wired by the F6 Signature cluster + F8 AcroForm save-through.
    void Sign(Forms::Signature& signature);

    // Returns the previously-Sign()'d Signature, or nullptr.
    // Canonical .NET getter — the cpp equivalent is the
    // nullable raw pointer.
    Forms::Signature* Signature() const noexcept;

private:
    Forms::Signature* signature_ = nullptr;
};

}  // namespace Aspose::Pdf::Forms
