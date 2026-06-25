#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextFragmentCollection — the result list of a
// TextFragmentAbsorber search. Mirrors canonical
// Aspose.Pdf.Text.TextFragmentCollection (v1 subset: Count + the 1-based
// indexer; Add / Remove / Contains / enumerator deferred).
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/text_fragment.hpp>

namespace Aspose::Pdf::Text {

class TextFragmentAbsorber;

class TextFragmentCollection {
public:
    TextFragmentCollection() = default;

    // Number of fragments found.
    int Count() const noexcept;

    // 1-based indexer (canonical TextFragmentCollection is 1-based).
    // Throws std::out_of_range on a bad index.
    TextFragment& operator[](int index);
    const TextFragment& operator[](int index) const;

private:
    friend class TextFragmentAbsorber;
    std::vector<TextFragment> items_;
};

}  // namespace Aspose::Pdf::Text
