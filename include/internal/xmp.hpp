// foundation/xmp — PDF XMP metadata packet parser
// (PDF 32000-1:2008 §14.3.2 / ISO 16684-1).
//
// v1 scope:
// simple-text + rdf:Alt (x-default) + rdf:Bag/Seq (TAB-joined)
// properties under <rdf:Description>, attribute-form properties
// supported. UTF-8 only.

#pragma once

#include <cstddef>
#include <map>
#include <span>
#include <string>

namespace foundation::xmp {

// Flat property bag from a parsed XMP packet. Keys are
// "prefix:name" (e.g. "dc:title", "pdf:Producer"). Values
// are UTF-8 text. Multi-value properties (rdf:Bag /
// rdf:Seq) join entries with TAB; rdf:Alt language
// variants flatten to the x-default value (or the first
// li if no x-default exists).
//
// `namespace_uris` carries every `xmlns:prefix → uri`
// declaration harvested across the parsed packet (rdf:RDF
// + rdf:Description, plus any nested elements). Lets the
// public Metadata wrapper populate its prefix registry so
// a subsequent save round-trips custom namespaces without
// falling back to the built-in DefaultNamespaceUris.
struct Properties {
    std::map<std::string, std::string> entries;
    std::map<std::string, std::string> namespace_uris;
};

Properties Parse(std::span<const std::byte> input);
std::string ToCanonical(const Properties& props);

}
