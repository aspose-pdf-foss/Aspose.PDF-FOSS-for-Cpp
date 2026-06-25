// foundation/xmp. See xmp.hpp +
// the project spec for the contract.

#include "xmp.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace foundation::xmp {

namespace {

struct NsEntry {
    std::string_view uri;
    std::string_view canonical;
};

constexpr NsEntry kCanonicalNs[] = {
    {"http://purl.org/dc/elements/1.1/",        "dc"},
    {"http://ns.adobe.com/pdf/1.3/",            "pdf"},
    {"http://ns.adobe.com/xap/1.0/",            "xmp"},
    {"http://ns.adobe.com/xap/1.0/mm/",         "xmpMM"},
    {"http://ns.adobe.com/xap/1.0/rights/",     "xmpRights"},
    {"http://ns.adobe.com/pdfx/1.3/",           "pdfx"},
    // v2 additions (2026-04-26): four more prefixes to match
    // the reference XmpMetadata.PrefixToNamespace table. Needed
    // for the XmpMetadata public-API capability — without them,
    // pdfaid:part / pdfaid:conformance / etc. are silently
    // dropped on parse.
    {"http://www.aiim.org/pdfa/ns/id/",         "pdfaid"},
    {"http://ns.adobe.com/photoshop/1.0/",      "photoshop"},
    {"http://ns.adobe.com/tiff/1.0/",           "tiff"},
    {"http://ns.adobe.com/exif/1.0/",           "exif"},
};

constexpr std::string_view kRdfNs =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
constexpr std::string_view kXmlNs =
    "http://www.w3.org/XML/1998/namespace";

std::string_view CanonicalPrefixForUri(std::string_view uri) {
    for (const auto& e : kCanonicalNs) {
        if (e.uri == uri) return e.canonical;
    }
    return {};
}

std::string_view StripXpacket(std::string_view body) {
    auto begin_pos = body.find("<?xpacket");
    if (begin_pos != std::string_view::npos) {
        auto end_pi = body.find("?>", begin_pos);
        if (end_pi != std::string_view::npos) {
            std::size_t after = end_pi + 2;
            while (after < body.size()
                   && (body[after] == ' ' || body[after] == '\n'
                       || body[after] == '\r' || body[after] == '\t')) {
                ++after;
            }
            body.remove_prefix(after);
        }
    }
    auto end_pos = body.rfind("<?xpacket");
    if (end_pos != std::string_view::npos) {
        body = body.substr(0, end_pos);
        while (!body.empty()
               && (body.back() == ' ' || body.back() == '\n'
                   || body.back() == '\r' || body.back() == '\t')) {
            body.remove_suffix(1);
        }
    }
    return body;
}

enum class NodeKind {
    Element,
    EndElement,
    Text,
};

struct Attribute {
    std::string name;
    std::string value;
};

struct Node {
    NodeKind kind;
    std::string name;
    std::vector<Attribute> attrs;
    bool self_closing = false;
    std::string text;
};

[[noreturn]] void Throw(const char* msg) {
    throw std::runtime_error(std::string("foundation::xmp: ") + msg);
}

std::string DecodeEntities(std::string_view raw) {
    std::string out;
    out.reserve(raw.size());
    std::size_t i = 0;
    while (i < raw.size()) {
        char c = raw[i];
        if (c != '&') { out.push_back(c); ++i; continue; }
        auto semi = raw.find(';', i);
        if (semi == std::string_view::npos) {
            out.push_back(c); ++i; continue;
        }
        std::string_view entity = raw.substr(i + 1, semi - i - 1);
        if (entity == "amp") out.push_back('&');
        else if (entity == "lt") out.push_back('<');
        else if (entity == "gt") out.push_back('>');
        else if (entity == "quot") out.push_back('"');
        else if (entity == "apos") out.push_back('\'');
        else if (!entity.empty() && entity[0] == '#') {
            std::uint32_t cp = 0;
            std::size_t start = 1;
            int base = 10;
            if (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X')) {
                base = 16;
                start = 2;
            }
            for (std::size_t k = start; k < entity.size(); ++k) {
                char h = entity[k];
                int d = -1;
                if (h >= '0' && h <= '9') d = h - '0';
                else if (base == 16 && h >= 'a' && h <= 'f') d = h - 'a' + 10;
                else if (base == 16 && h >= 'A' && h <= 'F') d = h - 'A' + 10;
                else { cp = 0; break; }
                cp = cp * static_cast<std::uint32_t>(base) + static_cast<std::uint32_t>(d);
            }
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            } else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0xFFFF) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        } else {
            out.push_back(c);
            ++i;
            continue;
        }
        i = semi + 1;
    }
    return out;
}

bool IsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void SkipSpace(std::string_view body, std::size_t& i) {
    while (i < body.size() && IsSpace(body[i])) ++i;
}

std::string ReadName(std::string_view body, std::size_t& i) {
    std::size_t start = i;
    while (i < body.size()) {
        char c = body[i];
        if (IsSpace(c) || c == '=' || c == '>' || c == '/' || c == '<') break;
        ++i;
    }
    return std::string(body.substr(start, i - start));
}

std::string ReadAttrValue(std::string_view body, std::size_t& i) {
    if (i >= body.size() || (body[i] != '"' && body[i] != '\'')) {
        Throw("attribute value without quote");
    }
    char quote = body[i];
    ++i;
    std::size_t start = i;
    while (i < body.size() && body[i] != quote) ++i;
    if (i >= body.size()) Throw("unterminated attribute value");
    std::string raw(body.substr(start, i - start));
    ++i;
    return DecodeEntities(raw);
}

std::vector<Node> Tokenize(std::string_view body) {
    std::vector<Node> nodes;
    std::size_t i = 0;
    while (i < body.size()) {
        if (body[i] != '<') {
            std::size_t start = i;
            while (i < body.size() && body[i] != '<') ++i;
            std::string_view raw = body.substr(start, i - start);
            std::size_t a = 0, b = raw.size();
            while (a < b && IsSpace(raw[a])) ++a;
            while (b > a && IsSpace(raw[b - 1])) --b;
            std::string trimmed(raw.substr(a, b - a));
            if (!trimmed.empty()) {
                Node n;
                n.kind = NodeKind::Text;
                n.text = DecodeEntities(trimmed);
                nodes.push_back(std::move(n));
            }
            continue;
        }
        if (i + 3 < body.size() && body[i + 1] == '!'
            && body[i + 2] == '-' && body[i + 3] == '-') {
            auto end = body.find("-->", i + 4);
            if (end == std::string_view::npos) Throw("unterminated comment");
            i = end + 3;
            continue;
        }
        if (i + 1 < body.size() && body[i + 1] == '?') {
            auto end = body.find("?>", i + 2);
            if (end == std::string_view::npos) Throw("unterminated PI");
            i = end + 2;
            continue;
        }
        if (i + 1 < body.size() && body[i + 1] == '/') {
            i += 2;
            SkipSpace(body, i);
            Node n;
            n.kind = NodeKind::EndElement;
            n.name = ReadName(body, i);
            SkipSpace(body, i);
            if (i >= body.size() || body[i] != '>') Throw("malformed end tag");
            ++i;
            nodes.push_back(std::move(n));
            continue;
        }
        ++i;
        SkipSpace(body, i);
        Node n;
        n.kind = NodeKind::Element;
        n.name = ReadName(body, i);
        while (true) {
            SkipSpace(body, i);
            if (i >= body.size()) Throw("EOF inside tag");
            if (body[i] == '/' || body[i] == '>') break;
            Attribute a;
            a.name = ReadName(body, i);
            SkipSpace(body, i);
            if (i >= body.size() || body[i] != '=') {
                Throw("attribute without '=' separator");
            }
            ++i;
            SkipSpace(body, i);
            a.value = ReadAttrValue(body, i);
            n.attrs.push_back(std::move(a));
        }
        if (body[i] == '/') {
            n.self_closing = true;
            ++i;
            SkipSpace(body, i);
        }
        if (i >= body.size() || body[i] != '>') Throw("expected '>'");
        ++i;
        nodes.push_back(std::move(n));
    }
    return nodes;
}

struct NsFrame {
    std::vector<std::pair<std::string, std::string>> bindings;
};

class NsStack {
 public:
    NsStack() { stack_.push_back({}); }

    void Push(const std::vector<Attribute>& attrs) {
        NsFrame frame;
        for (const auto& a : attrs) {
            if (a.name == "xmlns") {
                frame.bindings.emplace_back("", a.value);
            } else if (a.name.starts_with("xmlns:")) {
                frame.bindings.emplace_back(
                    a.name.substr(6), a.value);
            }
        }
        stack_.push_back(std::move(frame));
    }

    void Pop() {
        if (!stack_.empty()) stack_.pop_back();
    }

    std::string Resolve(std::string_view prefix) const {
        for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
            for (const auto& [p, u] : it->bindings) {
                if (p == prefix) return u;
            }
        }
        return {};
    }

 private:
    std::vector<NsFrame> stack_;
};

std::pair<std::string_view, std::string_view> SplitQname(
        std::string_view qname) {
    auto colon = qname.find(':');
    if (colon == std::string_view::npos) {
        return {std::string_view{}, qname};
    }
    return {qname.substr(0, colon), qname.substr(colon + 1)};
}

std::string QnameToKey(std::string_view qname, const NsStack& ns) {
    auto [prefix, local] = SplitQname(qname);
    std::string uri = ns.Resolve(prefix);
    auto canon = CanonicalPrefixForUri(uri);
    if (!canon.empty()) {
        // Well-known URI — normalise to the canonical prefix so
        // non-standard sources (e.g. xmlns:foo="dc-uri") still
        // emit `dc:title` keys.
        return std::string(canon) + ":" + std::string(local);
    }
    // v1.1: fall back to the source-declared prefix when the
    // URI isn't in the canonical table. Lets custom namespaces
    // (e.g. xmlns:custom="http://example.com/v1/") round-trip
    // through parse without their properties being silently
    // dropped. Requires that the source actually declared the
    // prefix — an unresolved prefix (no xmlns binding visible)
    // still drops.
    if (uri.empty()) return {};
    return std::string(prefix) + ":" + std::string(local);
}

bool IsRdfElement(std::string_view qname, std::string_view local_match,
                  const NsStack& ns) {
    auto [prefix, local] = SplitQname(qname);
    if (local != local_match) return false;
    return ns.Resolve(prefix) == kRdfNs;
}

std::size_t FindMatchingEnd(const std::vector<Node>& nodes,
                             std::size_t start) {
    if (nodes[start].self_closing) return start;
    int depth = 1;
    for (std::size_t i = start + 1; i < nodes.size(); ++i) {
        if (nodes[i].kind == NodeKind::Element && !nodes[i].self_closing) {
            if (nodes[i].name == nodes[start].name) ++depth;
        } else if (nodes[i].kind == NodeKind::EndElement) {
            if (nodes[i].name == nodes[start].name) {
                --depth;
                if (depth == 0) return i;
            }
        }
    }
    Throw("element not closed");
}

template <typename P>
std::pair<std::size_t, std::size_t> FindChildElement(
        const std::vector<Node>& nodes,
        std::size_t parent_start, std::size_t parent_end,
        P predicate) {
    constexpr std::size_t npos = std::string::npos;
    if (nodes[parent_start].self_closing) return {npos, npos};
    int depth = 0;
    for (std::size_t i = parent_start + 1; i < parent_end; ++i) {
        if (nodes[i].kind == NodeKind::Element) {
            if (depth == 0 && predicate(nodes[i].name)) {
                std::size_t end = FindMatchingEnd(nodes, i);
                return {i, end};
            }
            if (!nodes[i].self_closing) ++depth;
        } else if (nodes[i].kind == NodeKind::EndElement) {
            if (depth > 0) --depth;
        }
    }
    return {npos, npos};
}

struct LiEntry {
    std::string text;
    std::string lang;
};

std::vector<LiEntry> CollectLi(
        const std::vector<Node>& nodes,
        std::size_t array_start, std::size_t array_end,
        NsStack& ns) {
    std::vector<LiEntry> out;
    int depth = 0;
    for (std::size_t i = array_start + 1; i < array_end; ++i) {
        const Node& n = nodes[i];
        if (n.kind == NodeKind::Element) {
            if (!n.self_closing) ns.Push(n.attrs);
            if (depth == 0 && IsRdfElement(n.name, "li", ns)) {
                LiEntry li;
                for (const auto& a : n.attrs) {
                    auto [pfx, local] = SplitQname(a.name);
                    if (local == "lang" && ns.Resolve(pfx) == kXmlNs) {
                        li.lang = a.value;
                        break;
                    }
                }
                std::size_t li_end = FindMatchingEnd(nodes, i);
                for (std::size_t k = i + 1; k < li_end; ++k) {
                    if (nodes[k].kind == NodeKind::Text) {
                        li.text += nodes[k].text;
                    }
                }
                out.push_back(std::move(li));
                if (!n.self_closing) ns.Pop();
                i = li_end;
                continue;
            }
            if (!n.self_closing) ++depth;
        } else if (n.kind == NodeKind::EndElement) {
            if (depth > 0) --depth;
            else ns.Pop();
        }
    }
    return out;
}

std::string ExtractPropertyValue(
        const std::vector<Node>& nodes,
        std::size_t prop_start, std::size_t prop_end,
        NsStack& ns) {
    if (nodes[prop_start].self_closing) return "";

    auto find_array = [&](std::string_view kind)
            -> std::pair<std::size_t, std::size_t> {
        return FindChildElement(
            nodes, prop_start, prop_end,
            [&](const std::string& name) {
                return IsRdfElement(name, kind, ns);
            });
    };

    auto walk_array = [&](std::size_t arr_start, std::size_t arr_end,
                          const char* kind) -> std::string {
        if (!nodes[arr_start].self_closing) {
            ns.Push(nodes[arr_start].attrs);
        }
        auto items = CollectLi(nodes, arr_start, arr_end, ns);
        if (!nodes[arr_start].self_closing) ns.Pop();

        std::string out;
        if (std::string_view{kind} == "Alt") {
            for (const auto& li : items) {
                if (li.lang == "x-default") return li.text;
            }
            if (!items.empty()) return items[0].text;
            return std::string{};
        } else if (std::string_view{kind} == "Seq") {
            for (std::size_t k = 0; k < items.size(); ++k) {
                if (k > 0) out.push_back('\t');
                out += items[k].text;
            }
            return out;
        } else {
            std::vector<std::string> sorted;
            sorted.reserve(items.size());
            for (const auto& li : items) sorted.push_back(li.text);
            std::sort(sorted.begin(), sorted.end());
            for (std::size_t k = 0; k < sorted.size(); ++k) {
                if (k > 0) out.push_back('\t');
                out += sorted[k];
            }
            return out;
        }
    };

    auto [alt_s, alt_e] = find_array("Alt");
    if (alt_s != std::string::npos) return walk_array(alt_s, alt_e, "Alt");
    auto [seq_s, seq_e] = find_array("Seq");
    if (seq_s != std::string::npos) return walk_array(seq_s, seq_e, "Seq");
    auto [bag_s, bag_e] = find_array("Bag");
    if (bag_s != std::string::npos) return walk_array(bag_s, bag_e, "Bag");

    std::string out;
    int depth = 0;
    for (std::size_t i = prop_start + 1; i < prop_end; ++i) {
        const Node& n = nodes[i];
        if (n.kind == NodeKind::Element && !n.self_closing) ++depth;
        else if (n.kind == NodeKind::EndElement) --depth;
        if (depth == 0 && n.kind == NodeKind::Text) out += n.text;
    }
    return out;
}

}  // namespace

Properties Parse(std::span<const std::byte> input) {
    if (input.empty()) Throw("empty input");

    std::string_view body(
        reinterpret_cast<const char*>(input.data()), input.size());
    body = StripXpacket(body);

    auto nodes = Tokenize(body);
    if (nodes.empty()) Throw("empty packet body");

    NsStack ns;
    std::size_t rdf_start = std::string::npos;
    std::size_t rdf_end = std::string::npos;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const Node& n = nodes[i];
        if (n.kind == NodeKind::Element) {
            ns.Push(n.attrs);
            if (IsRdfElement(n.name, "RDF", ns)) {
                rdf_start = i;
                rdf_end = FindMatchingEnd(nodes, i);
                break;
            }
            if (n.self_closing) ns.Pop();
        } else if (n.kind == NodeKind::EndElement) {
            ns.Pop();
        }
    }
    if (rdf_start == std::string::npos) Throw("no <rdf:RDF> element");

    Properties props;
    // Harvest every xmlns:prefix → uri declaration across the
    // parsed packet. First-declaration-wins (matches python's
    // setdefault posture). Lets the public Metadata wrapper
    // populate its prefix registry so a subsequent save
    // round-trips custom namespaces without DefaultNamespaceUris
    // fallback.
    for (const auto& n : nodes) {
        if (n.kind != NodeKind::Element) continue;
        for (const auto& a : n.attrs) {
            if (!a.name.starts_with("xmlns:")) continue;
            std::string prefix(a.name.substr(6));
            if (prefix.empty()) continue;
            props.namespace_uris.try_emplace(
                std::move(prefix), std::string(a.value));
        }
    }

    int depth = 0;
    for (std::size_t i = rdf_start + 1; i < rdf_end; ++i) {
        const Node& n = nodes[i];
        if (n.kind == NodeKind::Element) {
            ns.Push(n.attrs);
            if (depth == 0 && IsRdfElement(n.name, "Description", ns)) {
                std::size_t desc_end = FindMatchingEnd(nodes, i);

                for (const auto& a : n.attrs) {
                    if (a.name == "xmlns" || a.name.starts_with("xmlns:")) continue;
                    if (a.name.starts_with("rdf:")) continue;
                    std::string key = QnameToKey(a.name, ns);
                    if (!key.empty()) props.entries[key] = a.value;
                }

                if (!n.self_closing) {
                    int prop_depth = 0;
                    for (std::size_t j = i + 1; j < desc_end; ++j) {
                        const Node& m = nodes[j];
                        if (m.kind == NodeKind::Element) {
                            if (!m.self_closing) ns.Push(m.attrs);
                            if (prop_depth == 0) {
                                std::string key = QnameToKey(m.name, ns);
                                std::size_t prop_end =
                                    FindMatchingEnd(nodes, j);
                                if (!key.empty()) {
                                    std::string val = ExtractPropertyValue(
                                        nodes, j, prop_end, ns);
                                    props.entries[key] = val;
                                }
                                if (!m.self_closing) {
                                    ns.Pop();
                                    j = prop_end;
                                    continue;
                                }
                            }
                            if (!m.self_closing) ++prop_depth;
                        } else if (m.kind == NodeKind::EndElement) {
                            if (prop_depth > 0) --prop_depth;
                            ns.Pop();
                        }
                    }
                    ns.Pop();
                    i = desc_end;
                    continue;
                }
                ns.Pop();
            } else {
                if (!n.self_closing) ++depth;
                else ns.Pop();
            }
        } else if (n.kind == NodeKind::EndElement) {
            if (depth > 0) --depth;
            else ns.Pop();
        }
    }

    return props;
}

std::string ToCanonical(const Properties& props) {
    std::string out;
    for (const auto& [k, v] : props.entries) {
        out += k;
        out.push_back('\t');
        out += v;
        out.push_back('\n');
    }
    return out;
}

}  // namespace foundation::xmp
