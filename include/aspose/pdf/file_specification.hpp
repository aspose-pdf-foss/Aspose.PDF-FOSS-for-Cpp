#pragma once

// =============================================================================
// Aspose::Pdf::FileSpecification — embedded-file descriptor
// (PDF §7.11). Mirrors canonical Aspose.Pdf.FileSpecification
// (Aspose.PDF 26.4.0); extends System.Object + System.IDisposable.
//
// The v1 cpp surface is a pure data carrier — the actual
// /EmbeddedFile stream creation at Document.Save time lands in a
// follow-on beat with embedded-file plumbing through PdfWriter.
//
// Phased drops:
//   * .ctor(Stream, name) / .ctor(Stream, name, description) — Stream cascade
//   * .ctor(Stream, name, description) — Stream cascade
//   * StreamContents / Contents — Stream cascade
//   * CollectionItem / EncryptedPayload / Params — class cascade
//     (Aspose.Pdf.{CollectionItem, EncryptedPayload, FileParams}
//     not yet in the cpp catalog)
// =============================================================================

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include <aspose/pdf/af_relationship.hpp>
#include <aspose/pdf/file_encoding.hpp>

namespace Aspose::Pdf::Annotations { class Annotation; }

namespace Aspose::Pdf {

class Document;


class FileSpecification {
public:
    FileSpecification() noexcept = default;
    explicit FileSpecification(std::string file);
    FileSpecification(std::string file, std::string description);
    FileSpecification(std::string fileName,
                      Aspose::Pdf::Annotations::Annotation& annot);

    // IDisposable.Dispose — clears the held fields so the
    // FileSpecification can be safely re-used or replaced.
    void Dispose();

    // PDF /CI dictionary key/value passthrough (Collection Item
    // attributes — keys are PDF name objects, values are strings).
    std::string GetValue(const std::string& key) const;
    void SetValue(const std::string& key, std::string value);

    // ---- Properties ----

    Aspose::Pdf::FileEncoding Encoding() const noexcept;
    void Encoding(Aspose::Pdf::FileEncoding value) noexcept;

    bool IncludeContents() const noexcept;
    void IncludeContents(bool value) noexcept;

    const std::string& Description() const noexcept;
    void Description(std::string value) noexcept;

    Aspose::Pdf::AFRelationship AFRelationship() const noexcept;
    void AFRelationship(Aspose::Pdf::AFRelationship value) noexcept;

    const std::string& MIMEType() const noexcept;
    void MIMEType(std::string value) noexcept;

    const std::string& Name() const noexcept;
    void Name(std::string value) noexcept;

    const std::string& UnicodeName() const noexcept;
    void UnicodeName(std::string value) noexcept;

    const std::string& FileSystem() const noexcept;
    void FileSystem(std::string value) noexcept;

private:
    std::string file_;
    std::string description_;
    std::string mime_type_;
    std::string name_;
    std::string unicode_name_;
    std::string file_system_;
    std::map<std::string, std::string> ci_values_;
    Aspose::Pdf::FileEncoding encoding_ = Aspose::Pdf::FileEncoding::None;
    Aspose::Pdf::AFRelationship af_relationship_ =
        Aspose::Pdf::AFRelationship::Unspecified;
    bool include_contents_ = true;
    Aspose::Pdf::Annotations::Annotation* annot_ = nullptr;

    // In-memory embedded-file bytes. Populated by Document on
    // load-on-open (the decoded /EmbeddedFile stream) and consumed by the
    // Save writer + attachment extraction. When empty, the writer falls
    // back to reading Name() as a filesystem path. Not public surface —
    // the canonical Stream-based Contents accessor remains dropped.
    friend class Aspose::Pdf::Document;
    std::vector<std::byte> content_;
};

}  // namespace Aspose::Pdf
