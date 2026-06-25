#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfFileSecurity — file-level encryption /
// decryption / password-change / permission facade. Mirrors canonical
// Aspose.Pdf.Facades.PdfFileSecurity; extends SaveableFacade.
//
// The facade is input-file -> output-file: set InputFile (or BindPdf /
// a Document ctor) + OutputFile, then call EncryptFile / DecryptFile /
// ChangePassword / SetPrivilege. Each operation loads the input fresh,
// applies the change through the real foundation crypto (Document::
// Encrypt / Decrypt — the same Standard Security Handler path proven by
// the document encrypt/decrypt round-trip tests) and writes the output.
//
// Throwing vs Try* forms:
//   * EncryptFile / DecryptFile / SetPrivilege / ChangePassword return
//     bool; on error they rethrow when AllowExceptions is true and
//     otherwise return false.
//   * TryEncryptFile / TryDecryptFile / TrySetPrivilege /
//     TryChangePassword never rethrow — they return false and record
//     the failure.
//
// v1 note: SetPrivilege(privilege) alone (no passwords) is a stub —
// re-applying permissions while preserving the existing passwords needs
// the bound document's recovered credentials, which v1 does not carry.
// Use SetPrivilege(user, owner, privilege) instead.
//
// Phased drops:
//   * Stream ctors / BindPdf(Stream) / InputStream / OutputStream —
//     Stream cascade.
//   * LastException (System.Exception getter) — System.Exception
//     cascade.
// =============================================================================

#include <string>

#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/facades/algorithm.hpp>
#include <aspose/pdf/facades/key_size.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfFileSecurity : public SaveableFacade {
public:
    PdfFileSecurity() noexcept = default;
    PdfFileSecurity(const std::string& inputFile,
                    const std::string& outputFile);
    explicit PdfFileSecurity(Aspose::Pdf::Document& document);
    PdfFileSecurity(Aspose::Pdf::Document& document,
                    const std::string& outputFile);

    // Set the input source. Overrides the base file-binding to record
    // the path for the file-to-file operations (no Document is opened
    // until an operation runs).
    void BindPdf(const std::string& srcFile) override;
    using SaveableFacade::BindPdf;

    // ---- Encrypt ----

    bool EncryptFile(const std::string& userPassword,
                     const std::string& ownerPassword,
                     const DocumentPrivilege& privilege,
                     KeySize keySize);
    bool EncryptFile(const std::string& userPassword,
                     const std::string& ownerPassword,
                     const DocumentPrivilege& privilege,
                     KeySize keySize,
                     Algorithm cipher);
    bool TryEncryptFile(const std::string& userPassword,
                        const std::string& ownerPassword,
                        const DocumentPrivilege& privilege,
                        KeySize keySize);

    // ---- Decrypt ----

    bool DecryptFile(const std::string& ownerPassword);
    bool TryDecryptFile(const std::string& ownerPassword);

    // ---- Permissions ----

    // v1 stub — needs the bound doc's recovered passwords. Returns
    // false. Use the (user, owner, privilege) overload.
    bool SetPrivilege(const DocumentPrivilege& privilege);
    bool SetPrivilege(const std::string& userPassword,
                      const std::string& ownerPassword,
                      const DocumentPrivilege& privilege);
    bool TrySetPrivilege(const std::string& userPassword,
                         const std::string& ownerPassword,
                         const DocumentPrivilege& privilege);

    // ---- Change password ----

    bool ChangePassword(const std::string& ownerPassword,
                        const std::string& newUserPassword,
                        const std::string& newOwnerPassword);
    bool ChangePassword(const std::string& ownerPassword,
                        const std::string& newUserPassword,
                        const std::string& newOwnerPassword,
                        const DocumentPrivilege& privilege,
                        KeySize keySize);
    bool ChangePassword(const std::string& ownerPassword,
                        const std::string& newUserPassword,
                        const std::string& newOwnerPassword,
                        const DocumentPrivilege& privilege,
                        KeySize keySize,
                        Algorithm cipher);
    bool TryChangePassword(const std::string& ownerPassword,
                           const std::string& newUserPassword,
                           const std::string& newOwnerPassword);
    bool TryChangePassword(const std::string& ownerPassword,
                           const std::string& newUserPassword,
                           const std::string& newOwnerPassword,
                           const DocumentPrivilege& privilege,
                           KeySize keySize);
    bool TryChangePassword(const std::string& ownerPassword,
                           const std::string& newUserPassword,
                           const std::string& newOwnerPassword,
                           const DocumentPrivilege& privilege,
                           KeySize keySize,
                           Algorithm cipher);

    // ---- Properties ----

    void InputFile(const std::string& value);
    void OutputFile(const std::string& value);

    bool AllowExceptions() const noexcept;
    void AllowExceptions(bool value) noexcept;

private:
    // Throwing operation cores shared by the Try / non-Try pairs.
    void EncryptImpl(const std::string& userPassword,
                     const std::string& ownerPassword,
                     const DocumentPrivilege& privilege,
                     KeySize keySize, Algorithm cipher);
    void DecryptImpl(const std::string& ownerPassword);
    void ChangePasswordImpl(const std::string& ownerPassword,
                            const std::string& newUserPassword,
                            const std::string& newOwnerPassword,
                            const DocumentPrivilege& privilege,
                            KeySize keySize, Algorithm cipher);

    std::string input_file_;
    std::string output_file_;
    std::string last_exception_;  // recorded by Try* / non-throwing path
    bool allow_exceptions_ = false;
};

}  // namespace Aspose::Pdf::Facades
