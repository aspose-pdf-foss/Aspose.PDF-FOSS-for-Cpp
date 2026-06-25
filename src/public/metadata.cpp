#include <aspose/pdf/metadata.hpp>

#include "xmp.hpp"

#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

namespace {

// Built-in fallback registry of well-known XMP namespace URIs.
// Used by ToXmpPacket when the caller hasn't registered a
// prefix explicitly — covers the prefixes a PDF /Metadata
// stream typically carries.
const std::map<std::string, std::string>& DefaultNamespaceUris() {
    static const std::map<std::string, std::string> kMap = {
        {"dc", "http://purl.org/dc/elements/1.1/"},
        {"pdf", "http://ns.adobe.com/pdf/1.3/"},
        {"xmp", "http://ns.adobe.com/xap/1.0/"},
        {"xmpMM", "http://ns.adobe.com/xap/1.0/mm/"},
        {"xmpRights", "http://ns.adobe.com/xap/1.0/rights/"},
        {"pdfaid", "http://www.aiim.org/pdfa/ns/id/"},
    };
    return kMap;
}

std::string XmlTextEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            default:   out += c;        break;
        }
    }
    return out;
}

std::string XmlAttrEscape(const std::string& text) {
    std::string out = XmlTextEscape(text);
    std::string replaced;
    replaced.reserve(out.size());
    for (char c : out) {
        if (c == '"') replaced += "&quot;";
        else replaced += c;
    }
    return replaced;
}

}  // namespace

Metadata::Metadata() = default;

Metadata::Metadata(std::span<const std::byte> xmpPacket) {
    if (xmpPacket.empty()) return;
    auto props = foundation::xmp::Parse(xmpPacket);
    for (auto& [key, value] : props.entries) {
        entries_.emplace(key, XmpValue(value));
    }
    // Hydrate the prefix → URI registry from the parsed packet
    // so a subsequent save round-trips custom namespaces without
    // falling back to DefaultNamespaceUris. Source-population
    // does NOT dirty (dirty_ stays false).
    for (auto& [prefix, uri] : props.namespace_uris) {
        prefix_to_uri_.emplace(prefix, uri);
        uri_to_prefix_.emplace(uri, prefix);
    }
}

void Metadata::RegisterNamespaceUri(const std::string& prefix,
                                    const std::string& namespaceUri) {
    prefix_to_uri_[prefix] = namespaceUri;
    uri_to_prefix_[namespaceUri] = prefix;
    dirty_ = true;
}

void Metadata::RegisterNamespaceUri(
        const std::string& prefix,
        const std::string& namespaceUri,
        const std::string& schemaDescription) {
    RegisterNamespaceUri(prefix, namespaceUri);
    schema_descriptions_[prefix] = schemaDescription;
}

std::string Metadata::GetNamespaceUriByPrefix(
        const std::string& prefix) const {
    auto it = prefix_to_uri_.find(prefix);
    return it == prefix_to_uri_.end() ? std::string{} : it->second;
}

std::string Metadata::GetPrefixByNamespaceUri(
        const std::string& namespaceUri) const {
    auto it = uri_to_prefix_.find(namespaceUri);
    return it == uri_to_prefix_.end() ? std::string{} : it->second;
}

void Metadata::Add(const std::string& key, const XmpValue& value) {
    auto [it, inserted] = entries_.emplace(key, value);
    if (!inserted) {
        throw std::runtime_error(
            "Metadata::Add: key already present");
    }
    dirty_ = true;
}

void Metadata::Clear() noexcept {
    if (entries_.empty()) return;
    entries_.clear();
    dirty_ = true;
}

bool Metadata::Contains(const std::string& key) const {
    return entries_.find(key) != entries_.end();
}

bool Metadata::Remove(const std::string& key) {
    bool removed = entries_.erase(key) > 0;
    if (removed) dirty_ = true;
    return removed;
}

bool Metadata::ContainsKey(const std::string& key) const {
    return Contains(key);
}

bool Metadata::TryGetValue(const std::string& key,
                           XmpValue& value) const {
    auto it = entries_.find(key);
    if (it == entries_.end()) return false;
    value = it->second;
    return true;
}

bool Metadata::IsFixedSize() const noexcept { return false; }
bool Metadata::IsReadOnly() const noexcept  { return false; }

std::vector<std::string> Metadata::Keys() const {
    std::vector<std::string> out;
    out.reserve(entries_.size());
    for (const auto& [k, v] : entries_) out.push_back(k);
    return out;
}

std::vector<XmpValue> Metadata::Values() const {
    std::vector<XmpValue> out;
    out.reserve(entries_.size());
    for (const auto& [k, v] : entries_) out.push_back(v);
    return out;
}

XmpValue& Metadata::operator[](const std::string& key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        it = entries_.emplace(key, XmpValue(std::string{})).first;
    }
    // Bracket access may be followed by assignment that mutates
    // the returned reference. Conservatively mark dirty — false
    // positives just trigger an unnecessary save-side re-emit
    // (harmless).
    dirty_ = true;
    return it->second;
}

const XmpValue& Metadata::at(const std::string& key) const {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        throw std::out_of_range(
            "Metadata::at: key not present");
    }
    return it->second;
}

int Metadata::Count() const noexcept {
    return static_cast<int>(entries_.size());
}

// v1.1 XMP packet serialiser — minimal RDF/XML envelope. Each
// entry's key splits on ':' into (prefix, local-name); the
// prefix must resolve to a URI via the per-instance registry
// or the built-in DefaultNamespaceUris fallback. Unknown
// prefixes throw.
std::vector<std::byte> Metadata::ToXmpPacket() const {
    std::vector<std::string> prefixes_used;
    {
        std::map<std::string, int> seen;
        for (const auto& [key, _val] : entries_) {
            auto colon = key.find(':');
            if (colon == std::string::npos || colon == 0) {
                throw std::runtime_error(
                    "Metadata key '" + key + "' is missing a "
                    "namespace prefix (expected 'prefix:local-name')");
            }
            std::string prefix = key.substr(0, colon);
            if (seen.emplace(prefix, 0).second) {
                prefixes_used.push_back(prefix);
            }
        }
    }
    std::vector<std::pair<std::string, std::string>> prefix_uris;
    for (const auto& prefix : prefixes_used) {
        auto local = prefix_to_uri_.find(prefix);
        std::string uri;
        if (local != prefix_to_uri_.end()) {
            uri = local->second;
        } else {
            auto fb = DefaultNamespaceUris().find(prefix);
            if (fb != DefaultNamespaceUris().end()) uri = fb->second;
        }
        if (uri.empty()) {
            throw std::runtime_error(
                "Metadata: prefix '" + prefix + "' has no "
                "registered namespace URI; call RegisterNamespaceUri "
                "before save");
        }
        prefix_uris.emplace_back(prefix, std::move(uri));
    }

    std::string sb;
    sb.reserve(512);
    sb += "<?xpacket begin=\"\xEF\xBB\xBF\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n";
    sb += "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n";
    sb += " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n";
    sb += "  <rdf:Description rdf:about=\"\"";
    for (const auto& [prefix, uri] : prefix_uris) {
        sb += "\n   xmlns:";
        sb += prefix;
        sb += "=\"";
        sb += XmlAttrEscape(uri);
        sb += "\"";
    }
    sb += ">\n";
    for (const auto& [key, val] : entries_) {
        sb += "   <";
        sb += key;
        sb += ">";
        sb += XmlTextEscape(val.ToString());
        sb += "</";
        sb += key;
        sb += ">\n";
    }
    sb += "  </rdf:Description>\n";
    sb += " </rdf:RDF>\n";
    sb += "</x:xmpmeta>\n";
    sb += "<?xpacket end=\"w\"?>\n";

    std::vector<std::byte> out;
    out.reserve(sb.size());
    for (char c : sb) out.push_back(static_cast<std::byte>(c));
    return out;
}

}
