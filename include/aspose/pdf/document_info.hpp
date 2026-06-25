#pragma once

// =============================================================================
// Aspose::Pdf::DocumentInfo — typed view over the PDF /Info dictionary.
//
// DLL surface (Aspose.PDF 26.4.0): sealed, base Dictionary<string, string>.
// v1 anchors the named-string surface (Title / Creator / Author /
// Subject / Keywords / Producer / Trapped) plus the directly-declared
// dict-mutation methods. Date / timezone properties and the inherited
// Dictionary<string,string> surface are out of scope until the
// corresponding placeholder types land — see capabilities/document_info.yaml.
//
// Lifetime: a DocumentInfo borrows the owning Document — typical use is
// `auto& info = doc.Info();` then read/write properties; mutations are
// flushed by a subsequent `doc.Save(...)` via pdf_writer_incremental.
// =============================================================================

#include <string>

namespace Aspose::Pdf {

class Document;

class DocumentInfo {
public:
    explicit DocumentInfo(Document& document) noexcept;

    DocumentInfo(const DocumentInfo&) = delete;
    DocumentInfo& operator=(const DocumentInfo&) = delete;
    DocumentInfo(DocumentInfo&&) noexcept = default;
    DocumentInfo& operator=(DocumentInfo&&) noexcept = default;
    ~DocumentInfo() = default;

    void Clear();
    void Add(const std::string& key, const std::string& value);
    void Remove(const std::string& key);
    void ClearCustomData();
    static bool IsPredefinedKey(const std::string& key) noexcept;

    const std::string& Title() const noexcept;
    void Title(std::string value);

    const std::string& Creator() const noexcept;
    void Creator(std::string value);

    const std::string& Author() const noexcept;
    void Author(std::string value);

    const std::string& Subject() const noexcept;
    void Subject(std::string value);

    const std::string& Keywords() const noexcept;
    void Keywords(std::string value);

    const std::string& Producer() const noexcept;
    void Producer(std::string value);

    const std::string& Trapped() const noexcept;
    void Trapped(std::string value);

    // Canonical indexer (this[string]) — reads any /Info entry (predefined
    // or custom) by key; returns "" when absent. Backs custom-property
    // read-back.
    const std::string& operator[](const std::string& key) const;

private:
    void EnsureLoaded() const;
    const std::string& Get(const std::string& key) const noexcept;
    void Set(const std::string& key, std::string value);

    Document* doc_ = nullptr;
};

}
