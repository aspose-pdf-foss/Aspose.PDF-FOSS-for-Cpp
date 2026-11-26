// Sign a PDF with your own certificate (a PKCS#12 .pfx / .p12 file with an
// RSA or EC key), then reopen the result and verify the signature.
//
//   usage: 13_sign_and_verify <input.pdf> <signer.p12> <password> <output.pdf>
//
// The repository's test certificate works for a try-out:
//   tests/fixtures/pkcs12/signer_aes.p12 (password "test"), or ec256.p12
//   for an ECDSA key.
// Verification checks the signed bytes' digest and the signature against
// the embedded certificate — not the certificate's trust chain.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_signature.hpp>
#include <aspose/pdf/forms/pkcs7.hpp>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: 13_sign_and_verify <input.pdf> <signer.p12> "
                     "<password> <output.pdf>\n";
        return 2;
    }
    namespace fac = Aspose::Pdf::Facades;
    {
        fac::PdfFileSignature signer{argv[1], argv[4]};
        Aspose::Pdf::Forms::PKCS7 certificate{argv[2], argv[3]};
        certificate.Reason("Approved");
        certificate.Location("Example 13");
        signer.Sign("Signature1", certificate);
        signer.Save();
    }

    fac::PdfFileSignature reader{argv[4]};
    const bool valid = reader.VerifySignature("Signature1");
    std::cout << "Signed " << argv[1] << " -> " << argv[4] << "\n"
              << "  signer:   " << reader.GetSignerName("Signature1") << "\n"
              << "  reason:   " << reader.GetReason("Signature1") << "\n"
              << "  covers whole document: " << std::boolalpha
              << reader.CoversWholeDocument("Signature1") << "\n"
              << "  signature valid:       " << valid << "\n";
    return valid ? 0 : 1;
}
