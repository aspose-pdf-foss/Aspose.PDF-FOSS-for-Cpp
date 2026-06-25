#pragma once

// =============================================================================
// Aspose::Pdf::Metadata — typed accessor over a Document's
// /Metadata XMP packet.
//
// String-keyed dictionary of XmpValue, populated from an XMP
// packet via foundation::xmp. v1 keeps the canonical's
// string-keyed members and the namespace registration helpers.
// Dropped from canonical for v1: the IDictionary<,>
// KeyValuePair-overloads of Add/Contains/Remove, CopyTo,
// GetEnumerator (the IDictionary interface itself is dropped),
// PDF/A ExtensionFields, BCL-typed SyncRoot / NamespaceManager /
// IsSynchronized.
//
// See capabilities/metadata.yaml for the language-neutral
// contract and the per-target BCL substitutions.
// =============================================================================

#include <aspose/pdf/xmp_value.hpp>

#include <cstddef>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace Aspose::Pdf {

class Metadata {
public:
    Metadata(const Metadata&) = delete;
    Metadata& operator=(const Metadata&) = delete;
    Metadata(Metadata&&) noexcept = default;
    Metadata& operator=(Metadata&&) noexcept = default;
    ~Metadata() = default;

    void RegisterNamespaceUri(const std::string& prefix,
                              const std::string& namespaceUri);
    void RegisterNamespaceUri(const std::string& prefix,
                              const std::string& namespaceUri,
                              const std::string& schemaDescription);
    std::string GetNamespaceUriByPrefix(const std::string& prefix) const;
    std::string GetPrefixByNamespaceUri(
        const std::string& namespaceUri) const;

    void Add(const std::string& key, const XmpValue& value);
    void Clear() noexcept;
    bool Contains(const std::string& key) const;
    bool Remove(const std::string& key);
    bool ContainsKey(const std::string& key) const;
    bool TryGetValue(const std::string& key, XmpValue& value) const;

    bool IsFixedSize() const noexcept;
    bool IsReadOnly() const noexcept;
    std::vector<std::string> Keys() const;
    std::vector<XmpValue> Values() const;

    XmpValue& operator[](const std::string& key);
    const XmpValue& at(const std::string& key) const;

    int Count() const noexcept;

private:
    friend class Document;

    Metadata();
    explicit Metadata(std::span<const std::byte> xmpPacket);

    // v1.1 write-side: every mutation flips Dirty. Document::Save
    // reads the flag and emits a fresh /Metadata stream via
    // foundation::pdf_writer_incremental when set.
    bool Dirty() const noexcept { return dirty_; }
    void ClearDirty() noexcept { dirty_ = false; }
    std::vector<std::byte> ToXmpPacket() const;

    std::map<std::string, XmpValue> entries_;
    std::map<std::string, std::string> prefix_to_uri_;
    std::map<std::string, std::string> uri_to_prefix_;
    std::map<std::string, std::string> schema_descriptions_;
    bool dirty_ = false;
};

}
