#include "xml.hpp"

#include <cctype>
#include <stdexcept>

namespace foundation::xml {

namespace {

[[noreturn]] void Fail(const std::string& what) {
    throw std::runtime_error("xml: " + what);
}

bool IsNameChar(char c) {
    const auto u = static_cast<unsigned char>(c);
    return std::isalnum(u) || c == '_' || c == '-' || c == '.' || c == ':' || u >= 0x80;
}

class Reader {
public:
    Reader(std::string_view s, bool latin1) : s_(s), latin1_(latin1) {}

    Element Document() {
        SkipMisc();
        if (pos_ >= s_.size() || s_[pos_] != '<') Fail("no document element");
        Element root = ReadElement();
        SkipMisc();
        return root;
    }

private:
    bool StartsWith(std::string_view p) const { return s_.substr(pos_, p.size()) == p; }

    void SkipWs() {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
    }

    void SkipPast(std::string_view end) {
        const auto e = s_.find(end, pos_);
        if (e == std::string_view::npos) Fail("unterminated construct");
        pos_ = e + end.size();
    }

    // Comments, processing instructions, DOCTYPE and whitespace.
    void SkipMisc() {
        for (;;) {
            SkipWs();
            if (StartsWith("<?")) SkipPast("?>");
            else if (StartsWith("<!--")) SkipPast("-->");
            else if (StartsWith("<!DOCTYPE") || StartsWith("<!doctype")) SkipDoctype();
            else return;
        }
    }

    void SkipDoctype() {
        int depth = 0;
        for (; pos_ < s_.size(); ++pos_) {
            if (s_[pos_] == '[') ++depth;
            else if (s_[pos_] == ']') --depth;
            else if (s_[pos_] == '>' && depth <= 0) { ++pos_; return; }
        }
        Fail("unterminated DOCTYPE");
    }

    std::string ReadName() {
        const std::size_t start = pos_;
        while (pos_ < s_.size() && IsNameChar(s_[pos_])) ++pos_;
        if (pos_ == start) Fail("expected a name");
        return std::string(s_.substr(start, pos_ - start));
    }

    // Raw characters (with entities) up to `stop`, decoded.
    std::string Decode(std::string_view raw) const {
        std::string out;
        for (std::size_t i = 0; i < raw.size(); ++i) {
            const char c = raw[i];
            if (c == '&') {
                const auto semi = raw.find(';', i);
                if (semi == std::string_view::npos) Fail("unterminated entity");
                const std::string_view ent = raw.substr(i + 1, semi - i - 1);
                if (ent == "lt") out += '<';
                else if (ent == "gt") out += '>';
                else if (ent == "amp") out += '&';
                else if (ent == "quot") out += '"';
                else if (ent == "apos") out += '\'';
                else if (!ent.empty() && ent[0] == '#') {
                    const bool hex = ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X');
                    const std::string digits(ent.substr(hex ? 2 : 1));
                    try {
                        AppendUtf8(out, static_cast<char32_t>(std::stoul(digits, nullptr, hex ? 16 : 10)));
                    } catch (...) {
                        Fail("bad character reference");
                    }
                } else {
                    Fail("unknown entity &" + std::string(ent) + ";");
                }
                i = semi;
            } else if (latin1_ && static_cast<unsigned char>(c) >= 0x80) {
                AppendUtf8(out, static_cast<unsigned char>(c));
            } else {
                out += c;
            }
        }
        return out;
    }

    Element ReadElement() {
        ++pos_;  // '<'
        Element e;
        e.name = ReadName();
        for (;;) {
            SkipWs();
            if (pos_ >= s_.size()) Fail("unterminated start tag");
            if (StartsWith("/>")) { pos_ += 2; return e; }
            if (s_[pos_] == '>') { ++pos_; break; }
            std::string key = ReadName();
            SkipWs();
            if (pos_ >= s_.size() || s_[pos_] != '=') Fail("expected '=' after " + key);
            ++pos_;
            SkipWs();
            if (pos_ >= s_.size() || (s_[pos_] != '"' && s_[pos_] != '\'')) Fail("expected a quoted value");
            const char q = s_[pos_++];
            const auto end = s_.find(q, pos_);
            if (end == std::string_view::npos) Fail("unterminated attribute value");
            e.attributes.emplace_back(std::move(key), Decode(s_.substr(pos_, end - pos_)));
            pos_ = end + 1;
        }
        // Content.
        for (;;) {
            if (pos_ >= s_.size()) Fail("unterminated element <" + e.name + ">");
            if (StartsWith("</")) {
                pos_ += 2;
                const std::string close = ReadName();
                if (close != e.name) Fail("mismatched </" + close + "> for <" + e.name + ">");
                SkipWs();
                if (pos_ >= s_.size() || s_[pos_] != '>') Fail("bad end tag");
                ++pos_;
                return e;
            }
            if (StartsWith("<!--")) { SkipPast("-->"); continue; }
            if (StartsWith("<?")) { SkipPast("?>"); continue; }
            if (StartsWith("<![CDATA[")) {
                pos_ += 9;
                const auto end = s_.find("]]>", pos_);
                if (end == std::string_view::npos) Fail("unterminated CDATA");
                e.text += std::string(s_.substr(pos_, end - pos_));
                pos_ = end + 3;
                continue;
            }
            if (s_[pos_] == '<') {
                e.children.push_back(ReadElement());
                continue;
            }
            const auto lt = s_.find('<', pos_);
            const auto end = lt == std::string_view::npos ? s_.size() : lt;
            e.text += Decode(s_.substr(pos_, end - pos_));
            pos_ = end;
        }
    }

    std::string_view s_;
    std::size_t pos_ = 0;
    bool latin1_;
};

}  // namespace

const std::string* Element::Attribute(std::string_view key) const {
    for (const auto& [k, v] : attributes)
        if (k == key) return &v;
    return nullptr;
}

const Element* Element::Child(std::string_view child_name) const {
    for (const auto& c : children)
        if (c.name == child_name) return &c;
    return nullptr;
}

Element Parse(std::string_view xml) {
    // UTF-8 BOM.
    if (xml.size() >= 3 && xml.substr(0, 3) == "\xEF\xBB\xBF") xml.remove_prefix(3);
    bool latin1 = false;
    if (xml.substr(0, 5) == "<?xml") {
        const auto end = xml.find("?>");
        std::string decl(xml.substr(0, end == std::string_view::npos ? 0 : end));
        for (auto& c : decl) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        latin1 = decl.find("8859") != std::string::npos || decl.find("latin1") != std::string::npos ||
                 decl.find("latin-1") != std::string::npos;
    }
    return Reader(xml, latin1).Document();
}

std::string Escape(std::string_view text) {
    std::string out;
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r')
                    out += "&#" + std::to_string(static_cast<int>(c)) + ";";
                else
                    out += c;
        }
    }
    return out;
}

std::vector<char32_t> DecodeUtf8(std::string_view text) {
    std::vector<char32_t> out;
    for (std::size_t i = 0; i < text.size();) {
        const auto b = static_cast<unsigned char>(text[i]);
        int extra = 0;
        char32_t cp = 0;
        if (b < 0x80) { cp = b; }
        else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; extra = 1; }
        else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; extra = 2; }
        else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; extra = 3; }
        else { out.push_back(0xFFFD); ++i; continue; }
        if (i + static_cast<std::size_t>(extra) >= text.size()) {  // truncated
            out.push_back(0xFFFD);
            break;
        }
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            const auto c = static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]);
            if ((c & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (c & 0x3F);
        }
        if (!ok) { out.push_back(0xFFFD); ++i; continue; }
        out.push_back(cp);
        i += static_cast<std::size_t>(extra) + 1;
    }
    return out;
}

void AppendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

}  // namespace foundation::xml
