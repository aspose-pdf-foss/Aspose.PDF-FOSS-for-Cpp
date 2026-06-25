#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextAbsorber — visitor that walks a Document or Page
// and accumulates the text it encounters into a single string. After
// Visit() returns, Text() returns the concatenated result.
//
// v1 surface: default-construct, Visit(Document) or Visit(Page), read Text.
// HasErrors always returns false in v1 — extraction surfaces hard failures
// via std::runtime_error, not through a soft errors collection.
// =============================================================================

#include <memory>
#include <string>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Text {

class TextAbsorber {
public:
    TextAbsorber();

    TextAbsorber(const TextAbsorber&) = delete;
    TextAbsorber& operator=(const TextAbsorber&) = delete;
    TextAbsorber(TextAbsorber&&) noexcept;
    TextAbsorber& operator=(TextAbsorber&&) noexcept;
    ~TextAbsorber();

    // Walk every page of the given document in order and accumulate the
    // text content. Subsequent Text() returns the concatenated result.
    void Visit(const Aspose::Pdf::Document& pdf);

    // Walk a single page and accumulate its text into Text().
    void Visit(const Aspose::Pdf::Page& page);

    // Accumulated extracted text from prior Visit() calls. Empty string
    // when no Visit has been called or the input had no extractable
    // text content.
    std::string Text() const;

    // True iff any extraction error was recorded during the last
    // Visit() call. v1 Visit paths surface only hard failures (which
    // propagate as exceptions), so a returning Visit means a clean run.
    bool HasErrors() const noexcept;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace Aspose::Pdf::Text
