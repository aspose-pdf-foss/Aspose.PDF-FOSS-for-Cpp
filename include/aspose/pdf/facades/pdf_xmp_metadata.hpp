#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfXmpMetadata — dictionary-style facade over the
// bound document's XMP metadata. Mirrors canonical
// Aspose.Pdf.Facades.PdfXmpMetadata; extends SaveableFacade.
//
// v1 wires the string-keyed dictionary surface through the REAL
// foundation Metadata (Document::Metadata() — which parses the
// /Metadata packet on load and writes an incremental /Metadata update
// on Save). Add / Remove / Contains / ContainsKey / TryGetValue /
// this[] / Keys / Values / Count / Clear and the namespace-URI helpers
// all delegate to the bound doc's Metadata.
//
// Phased drops:
//   * DefaultMetadataProperties-keyed overloads — need the DMP <-> XMP
//     property-key name table (string-keyed overloads cover v1).
//   * Object-valued Add — non-idiomatic; XmpValue overload is the path.
//   * XmpPdfAExtensionObject Add / ExtensionFields — types not in cpp
//     catalog.
//   * GetXmpMetadata (byte[]) — the XMP packet byte serialisation is
//     Document-internal.
//   * GetEnumerator / CopyTo / KeyValuePair-based Add/Contains/Remove /
//     IsSynchronized / SyncRoot — legacy .NET collection idioms.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/xmp_value.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfXmpMetadata : public SaveableFacade {
public:
    PdfXmpMetadata() noexcept = default;
    explicit PdfXmpMetadata(Aspose::Pdf::Document& document);

    // ---- Namespace registry ----

    void RegisterNamespaceURI(const std::string& prefix,
                              const std::string& namespaceURI);
    std::string GetNamespaceURIByPrefix(const std::string& prefix) const;
    std::string GetPrefixByNamespaceURI(
        const std::string& namespaceURI) const;

    // ---- Dictionary surface (real — wired to Document::Metadata) ----

    void Add(const std::string& key, const Aspose::Pdf::XmpValue& value);
    void Clear();
    bool Contains(const std::string& key) const;
    bool Remove(const std::string& key);
    bool ContainsKey(const std::string& key) const;
    bool TryGetValue(const std::string& key,
                     Aspose::Pdf::XmpValue& value) const;

    std::vector<std::string> Keys() const;
    std::vector<Aspose::Pdf::XmpValue> Values() const;

    Aspose::Pdf::XmpValue& operator[](const std::string& key);

    bool IsFixedSize() const;
    bool IsReadOnly() const;
    int Count() const;
};

}  // namespace Aspose::Pdf::Facades
