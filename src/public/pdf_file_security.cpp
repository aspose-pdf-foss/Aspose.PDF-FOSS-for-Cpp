#include <aspose/pdf/facades/pdf_file_security.hpp>

#include <functional>
#include <stdexcept>

#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

namespace {

// Map the facade's (Algorithm, KeySize) pair onto the document
// crypto selector. 256-bit keys exist only as AES; otherwise the
// cipher choice drives RC4 vs AES.
Aspose::Pdf::CryptoAlgorithm MapAlgo(Algorithm cipher, KeySize keySize) {
    if (keySize == KeySize::x256) {
        return Aspose::Pdf::CryptoAlgorithm::AESx256;
    }
    if (cipher == Algorithm::AES) {
        return Aspose::Pdf::CryptoAlgorithm::AESx128;
    }
    return keySize == KeySize::x40 ? Aspose::Pdf::CryptoAlgorithm::RC4x40
                                   : Aspose::Pdf::CryptoAlgorithm::RC4x128;
}

bool RunOp(bool allowExceptions, bool isTry, std::string& lastError,
           const std::function<void()>& op) {
    try {
        op();
        return true;
    } catch (const std::exception& e) {
        lastError = e.what();
        if (!isTry && allowExceptions) {
            throw;
        }
        return false;
    }
}

}  // namespace

PdfFileSecurity::PdfFileSecurity(const std::string& inputFile,
                                 const std::string& outputFile)
    : input_file_(inputFile), output_file_(outputFile) {}

PdfFileSecurity::PdfFileSecurity(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

PdfFileSecurity::PdfFileSecurity(Aspose::Pdf::Document& document,
                                 const std::string& outputFile)
    : output_file_(outputFile) {
    BindPdf(document);
}

void PdfFileSecurity::BindPdf(const std::string& srcFile) {
    input_file_ = srcFile;
}

// ===== Throwing operation cores ==============================================

void PdfFileSecurity::EncryptImpl(const std::string& userPassword,
                                  const std::string& ownerPassword,
                                  const DocumentPrivilege& privilege,
                                  KeySize keySize, Algorithm cipher) {
    const Aspose::Pdf::CryptoAlgorithm algo = MapAlgo(cipher, keySize);
    if (!input_file_.empty()) {
        Aspose::Pdf::Document doc(input_file_);
        doc.Encrypt(userPassword, ownerPassword, privilege, algo);
        doc.Save(output_file_);
    } else if (document_ != nullptr) {
        document_->Encrypt(userPassword, ownerPassword, privilege, algo);
        document_->Save(output_file_);
    } else {
        throw std::runtime_error("PdfFileSecurity: no input bound");
    }
}

void PdfFileSecurity::DecryptImpl(const std::string& ownerPassword) {
    if (!input_file_.empty()) {
        Aspose::Pdf::Document doc(input_file_, ownerPassword);
        doc.Decrypt();
        doc.Save(output_file_);
    } else if (document_ != nullptr) {
        document_->Decrypt();
        document_->Save(output_file_);
    } else {
        throw std::runtime_error("PdfFileSecurity: no input bound");
    }
}

void PdfFileSecurity::ChangePasswordImpl(const std::string& ownerPassword,
                                         const std::string& newUserPassword,
                                         const std::string& newOwnerPassword,
                                         const DocumentPrivilege& privilege,
                                         KeySize keySize, Algorithm cipher) {
    const Aspose::Pdf::CryptoAlgorithm algo = MapAlgo(cipher, keySize);
    if (!input_file_.empty()) {
        Aspose::Pdf::Document doc(input_file_, ownerPassword);
        doc.Decrypt();
        doc.Encrypt(newUserPassword, newOwnerPassword, privilege, algo);
        doc.Save(output_file_);
    } else if (document_ != nullptr) {
        document_->Decrypt();
        document_->Encrypt(newUserPassword, newOwnerPassword, privilege, algo);
        document_->Save(output_file_);
    } else {
        throw std::runtime_error("PdfFileSecurity: no input bound");
    }
}

// ===== Encrypt ===============================================================

bool PdfFileSecurity::EncryptFile(const std::string& userPassword,
                                  const std::string& ownerPassword,
                                  const DocumentPrivilege& privilege,
                                  KeySize keySize) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        EncryptImpl(userPassword, ownerPassword, privilege, keySize,
                    Algorithm::RC4);
    });
}

bool PdfFileSecurity::EncryptFile(const std::string& userPassword,
                                  const std::string& ownerPassword,
                                  const DocumentPrivilege& privilege,
                                  KeySize keySize, Algorithm cipher) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        EncryptImpl(userPassword, ownerPassword, privilege, keySize, cipher);
    });
}

bool PdfFileSecurity::TryEncryptFile(const std::string& userPassword,
                                     const std::string& ownerPassword,
                                     const DocumentPrivilege& privilege,
                                     KeySize keySize) {
    return RunOp(allow_exceptions_, true, last_exception_, [&] {
        EncryptImpl(userPassword, ownerPassword, privilege, keySize,
                    Algorithm::RC4);
    });
}

// ===== Decrypt ===============================================================

bool PdfFileSecurity::DecryptFile(const std::string& ownerPassword) {
    return RunOp(allow_exceptions_, false, last_exception_,
                 [&] { DecryptImpl(ownerPassword); });
}

bool PdfFileSecurity::TryDecryptFile(const std::string& ownerPassword) {
    return RunOp(allow_exceptions_, true, last_exception_,
                 [&] { DecryptImpl(ownerPassword); });
}

// ===== Permissions ===========================================================

bool PdfFileSecurity::SetPrivilege(const DocumentPrivilege&) {
    // v1 stub — re-applying permissions while preserving the existing
    // passwords needs the bound document's recovered credentials,
    // which v1 does not carry. Use SetPrivilege(user, owner, priv).
    return false;
}

bool PdfFileSecurity::SetPrivilege(const std::string& userPassword,
                                   const std::string& ownerPassword,
                                   const DocumentPrivilege& privilege) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        EncryptImpl(userPassword, ownerPassword, privilege, KeySize::x128,
                    Algorithm::AES);
    });
}

bool PdfFileSecurity::TrySetPrivilege(const std::string& userPassword,
                                      const std::string& ownerPassword,
                                      const DocumentPrivilege& privilege) {
    return RunOp(allow_exceptions_, true, last_exception_, [&] {
        EncryptImpl(userPassword, ownerPassword, privilege, KeySize::x128,
                    Algorithm::AES);
    });
}

// ===== Change password =======================================================

bool PdfFileSecurity::ChangePassword(const std::string& ownerPassword,
                                     const std::string& newUserPassword,
                                     const std::string& newOwnerPassword) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           DocumentPrivilege::AllowAll(), KeySize::x128,
                           Algorithm::RC4);
    });
}

bool PdfFileSecurity::ChangePassword(const std::string& ownerPassword,
                                     const std::string& newUserPassword,
                                     const std::string& newOwnerPassword,
                                     const DocumentPrivilege& privilege,
                                     KeySize keySize) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           privilege, keySize, Algorithm::RC4);
    });
}

bool PdfFileSecurity::ChangePassword(const std::string& ownerPassword,
                                     const std::string& newUserPassword,
                                     const std::string& newOwnerPassword,
                                     const DocumentPrivilege& privilege,
                                     KeySize keySize, Algorithm cipher) {
    return RunOp(allow_exceptions_, false, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           privilege, keySize, cipher);
    });
}

bool PdfFileSecurity::TryChangePassword(const std::string& ownerPassword,
                                        const std::string& newUserPassword,
                                        const std::string& newOwnerPassword) {
    return RunOp(allow_exceptions_, true, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           DocumentPrivilege::AllowAll(), KeySize::x128,
                           Algorithm::RC4);
    });
}

bool PdfFileSecurity::TryChangePassword(const std::string& ownerPassword,
                                        const std::string& newUserPassword,
                                        const std::string& newOwnerPassword,
                                        const DocumentPrivilege& privilege,
                                        KeySize keySize) {
    return RunOp(allow_exceptions_, true, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           privilege, keySize, Algorithm::RC4);
    });
}

bool PdfFileSecurity::TryChangePassword(const std::string& ownerPassword,
                                        const std::string& newUserPassword,
                                        const std::string& newOwnerPassword,
                                        const DocumentPrivilege& privilege,
                                        KeySize keySize, Algorithm cipher) {
    return RunOp(allow_exceptions_, true, last_exception_, [&] {
        ChangePasswordImpl(ownerPassword, newUserPassword, newOwnerPassword,
                           privilege, keySize, cipher);
    });
}

// ===== Properties ============================================================

void PdfFileSecurity::InputFile(const std::string& value) {
    input_file_ = value;
}

void PdfFileSecurity::OutputFile(const std::string& value) {
    output_file_ = value;
}

bool PdfFileSecurity::AllowExceptions() const noexcept {
    return allow_exceptions_;
}

void PdfFileSecurity::AllowExceptions(bool value) noexcept {
    allow_exceptions_ = value;
}

}  // namespace Aspose::Pdf::Facades
