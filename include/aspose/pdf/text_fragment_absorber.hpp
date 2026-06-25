#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextFragmentAbsorber — phrase-search visitor. Walks a
// Document or Page, finds occurrences of a search phrase, and exposes the
// matches as TextFragments. Mirrors canonical
// Aspose.Pdf.Text.TextFragmentAbsorber (extends TextAbsorber).
//
// v1 subset: default ctor + phrase ctor + Visit(Document)/Visit(Page) +
// TextFragments. The Regex / TextSearchOptions / TextEditOptions ctors and
// Visit(XForm) are dropped (those types are not in the cpp catalog). The
// default ctor is the canonical match-all mode: every positioned text run on
// the visited page(s) becomes a TextFragment carrying its Text, Rectangle,
// Position and TextState (FontSize). The phrase ctor locates occurrences of a
// search phrase.
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Text {

class TextFragmentAbsorber {
public:
    TextFragmentAbsorber();
    explicit TextFragmentAbsorber(const std::string& phrase);

    // Search every page of the document (Visit(Document)) or a single page
    // (Visit(Page)); matches accumulate in TextFragments().
    void Visit(const Aspose::Pdf::Document& pdf);
    void Visit(const Aspose::Pdf::Page& page);

    TextFragmentCollection& TextFragments() noexcept;
    const TextFragmentCollection& TextFragments() const noexcept;

private:
    void BuildFromShows(
        const std::vector<Aspose::Pdf::Document::TextShow>& shows,
        Aspose::Pdf::Document* doc);

    std::string phrase_;
    bool match_all_ = false;  // true for the default (extract-all) ctor
    TextFragmentCollection fragments_;
};

}  // namespace Aspose::Pdf::Text
