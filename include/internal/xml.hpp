#pragma once

// =============================================================================
// foundation::xml — a small non-validating XML reader for the interchange
// formats the facades import (bookmark XML, XFDF): elements, attributes,
// text, CDATA, the predefined and numeric character entities. Comments,
// processing instructions and DOCTYPE are skipped; DTDs are not processed.
// Text is returned as UTF-8 (ISO-8859-1 input, per the XML declaration, is
// converted).
// =============================================================================

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace foundation::xml {

struct Element {
    std::string name;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<Element> children;
    std::string text;  // the element's own text (children's text excluded)

    // nullptr when absent.
    const std::string* Attribute(std::string_view key) const;
    const Element* Child(std::string_view child_name) const;
};

// The document element. Throws std::runtime_error on malformed input.
Element Parse(std::string_view xml);

// Escape text / attribute values (& < > " and control characters).
std::string Escape(std::string_view text);

// Decode UTF-8 into code points (invalid bytes become U+FFFD) and encode
// one code point as UTF-8.
std::vector<char32_t> DecodeUtf8(std::string_view text);
void AppendUtf8(std::string& out, char32_t cp);

}  // namespace foundation::xml
