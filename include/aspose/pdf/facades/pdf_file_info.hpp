#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfFileInfo — facade for the bound PDF's
// /Info dictionary + miscellaneous file-level metadata (page
// count / page geometry / PDF version / encryption status).
// Mirrors canonical Aspose.Pdf.Facades.PdfFileInfo; extends
// SaveableFacade.
//
// /Info dict access (Author / Title / etc.) routes through the
// bound Document's `DocumentInfo` accessors when a document is
// bound; otherwise stored locally and flushed on the next Save.
//
// Phased drops:
//   * Stream-bound ctors / SaveNewInfo(Stream) / Save(Stream) —
//     Stream cascade
//   * .ctor(*, password, ICustomSecurityHandler) — ICustomSecurityHandler
//     cascade
//   * GetDocumentPrivilege — Aspose.Pdf.Facades.DocumentPrivilege
//     cascade
//   * Header (Dictionary<string,string>) — Dictionary cascade
//   * InputStream (Stream) — Stream cascade
// =============================================================================

#include <map>
#include <string>

#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/password_type.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfFileInfo : public SaveableFacade {
public:
    PdfFileInfo() noexcept = default;
    explicit PdfFileInfo(const std::string& inputFile);
    PdfFileInfo(const std::string& inputFile,
                const std::string& password);
    explicit PdfFileInfo(Aspose::Pdf::Document& document);

    // ---- Methods ----

    void BindPdf(Aspose::Pdf::Document& srcDoc) override;

    // Re-route the inherited file-binding overload so callers can
    // do `info.BindPdf("foo.pdf");` without ambiguity.
    using SaveableFacade::BindPdf;

    void ClearInfo();

    // Generic /Info dict key access (canonical /T /Author /Creator
    // /etc. by name). Returns "" when unset.
    std::string GetMetaInfo(const std::string& name) const;
    void SetMetaInfo(const std::string& name, const std::string& value);

    // Page geometry accessors. v1 returns canonical defaults
    // (612 x 792 letter size; rotation=0; offsets=0). Real
    // per-page geometry lands when Page exposes Rect / Rotate /
    // CropBox.
    float GetPageHeight(int pageNum) const;
    float GetPageRotation(int pageNum) const;
    float GetPageWidth(int pageNum) const;
    float GetPageXOffset(int pageNum) const;
    float GetPageYOffset(int pageNum) const;

    std::string GetPdfVersion() const;

    // Save the info-dict edits back to disk without re-emitting
    // the whole document. v1 delegates to Document::Save.
    // Returns true on success.
    bool SaveNewInfo(const std::string& outputFile);
    bool SaveNewInfoWithXmp(const std::string& outputFileName);

    // ---- Inherited methods (Save / Close) ----

    using SaveableFacade::Save;
    using Facade::Close;

    // ---- Properties — /Info string entries ----

    std::string Author() const;
    void Author(std::string value);

    std::string CreationDate() const;
    void CreationDate(std::string value);

    std::string Creator() const;
    void Creator(std::string value);

    std::string Keywords() const;
    void Keywords(std::string value);

    std::string ModDate() const;
    void ModDate(std::string value);

    std::string Producer() const;

    std::string Subject() const;
    void Subject(std::string value);

    std::string Title() const;
    void Title(std::string value);

    // ---- Properties — file-level metadata ----

    bool IsEncrypted() const noexcept;
    bool IsPdfFile() const noexcept;
    bool UseStrictValidation() const noexcept;
    void UseStrictValidation(bool value) noexcept;
    bool HasCollection() const noexcept;

    const std::string& InputFile() const noexcept;
    void InputFile(std::string value);

    int NumberOfPages() const noexcept;

    Aspose::Pdf::PasswordType PasswordType() const noexcept;
    bool HasOpenPassword() const noexcept;
    bool HasEditPassword() const noexcept;

private:
    std::string input_file_;
    bool use_strict_validation_ = false;
    Aspose::Pdf::PasswordType password_type_ =
        Aspose::Pdf::PasswordType::None;
    // Local storage for /Info keys that DocumentInfo doesn't
    // surface as typed accessors (CreationDate / ModDate /
    // custom keys via GetMetaInfo / SetMetaInfo). Values aren't
    // currently flushed through SaveNewInfo — that's a follow-on
    // that wires DocumentInfo's internal Get/Set or adds a public
    // generic accessor.
    std::map<std::string, std::string> extra_meta_;
};

}  // namespace Aspose::Pdf::Facades
