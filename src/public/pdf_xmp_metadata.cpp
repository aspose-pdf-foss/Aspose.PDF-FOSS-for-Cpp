#include <aspose/pdf/facades/pdf_xmp_metadata.hpp>

#include <stdexcept>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/metadata.hpp>

namespace Aspose::Pdf::Facades {

namespace {
[[noreturn]] void NoDocument() {
    throw std::runtime_error("PdfXmpMetadata: no document bound");
}
}  // namespace

PdfXmpMetadata::PdfXmpMetadata(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

void PdfXmpMetadata::RegisterNamespaceURI(const std::string& prefix,
                                          const std::string& namespaceURI) {
    if (document_ == nullptr) {
        NoDocument();
    }
    document_->Metadata().RegisterNamespaceUri(prefix, namespaceURI);
}

std::string PdfXmpMetadata::GetNamespaceURIByPrefix(
    const std::string& prefix) const {
    if (document_ == nullptr) {
        return {};
    }
    return document_->Metadata().GetNamespaceUriByPrefix(prefix);
}

std::string PdfXmpMetadata::GetPrefixByNamespaceURI(
    const std::string& namespaceURI) const {
    if (document_ == nullptr) {
        return {};
    }
    return document_->Metadata().GetPrefixByNamespaceUri(namespaceURI);
}

void PdfXmpMetadata::Add(const std::string& key,
                         const Aspose::Pdf::XmpValue& value) {
    if (document_ == nullptr) {
        NoDocument();
    }
    document_->Metadata().Add(key, value);
}

void PdfXmpMetadata::Clear() {
    if (document_ != nullptr) {
        document_->Metadata().Clear();
    }
}

bool PdfXmpMetadata::Contains(const std::string& key) const {
    return document_ != nullptr && document_->Metadata().Contains(key);
}

bool PdfXmpMetadata::Remove(const std::string& key) {
    return document_ != nullptr && document_->Metadata().Remove(key);
}

bool PdfXmpMetadata::ContainsKey(const std::string& key) const {
    return document_ != nullptr && document_->Metadata().ContainsKey(key);
}

bool PdfXmpMetadata::TryGetValue(const std::string& key,
                                 Aspose::Pdf::XmpValue& value) const {
    return document_ != nullptr &&
           document_->Metadata().TryGetValue(key, value);
}

std::vector<std::string> PdfXmpMetadata::Keys() const {
    if (document_ == nullptr) {
        return {};
    }
    return document_->Metadata().Keys();
}

std::vector<Aspose::Pdf::XmpValue> PdfXmpMetadata::Values() const {
    if (document_ == nullptr) {
        return {};
    }
    return document_->Metadata().Values();
}

Aspose::Pdf::XmpValue& PdfXmpMetadata::operator[](const std::string& key) {
    if (document_ == nullptr) {
        NoDocument();
    }
    return document_->Metadata()[key];
}

bool PdfXmpMetadata::IsFixedSize() const {
    return document_ != nullptr && document_->Metadata().IsFixedSize();
}

bool PdfXmpMetadata::IsReadOnly() const {
    return document_ != nullptr && document_->Metadata().IsReadOnly();
}

int PdfXmpMetadata::Count() const {
    return document_ == nullptr ? 0 : document_->Metadata().Count();
}

}  // namespace Aspose::Pdf::Facades
