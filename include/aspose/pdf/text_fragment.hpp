#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextFragment — a run of text located by a
// TextFragmentAbsorber search. Mirrors canonical Aspose.Pdf.Text.TextFragment
// (a strict subset: v1 ships Text — the matched string, with write-back —
// and Rectangle — the located bounding box. Position / TextState / segment
// members are deferred with the Position / TextFragmentState cascades).
//
// Setting Text on a fragment returned by a search registers a content-stream
// edit applied at the owning Document's next Save(): the matched run is
// replaced in place (empty string = deletion).
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <string>

#include <aspose/pdf/position.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_fragment_state.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Text {

class TextFragmentAbsorber;
class TextBuilder;

class TextFragment {
public:
    TextFragment() = default;

    // Construct an unlinked fragment carrying `text` — the starting point for
    // positioned insertion via TextBuilder.
    explicit TextFragment(std::string text);

    // The fragment's text. The setter rewrites the located run on the
    // owning Document's next Save() (no-op for an unlinked fragment).
    const std::string& Text() const noexcept;
    void Text(std::string value);

    // Bounding box of the fragment in user space (origin + font-size
    // height; width is font-metric-approximate in v1).
    Aspose::Pdf::Rectangle Rectangle() const;

    // Canonical TextFragment.TextState — the fragment's text rendering state
    // (Font / FontSize). Mutations apply in place (see TextFragmentState for
    // the v1 content-persistence boundary).
    Aspose::Pdf::Text::TextFragmentState& TextState() noexcept;

    // Canonical TextFragment.Position — the placement coordinate used when the
    // fragment is appended to a page via TextBuilder.
    Aspose::Pdf::Text::Position Position() const noexcept;
    void Position(Aspose::Pdf::Text::Position value) noexcept;

private:
    friend class TextFragmentAbsorber;
    friend class TextBuilder;

    std::string text_;
    Aspose::Pdf::Text::TextFragmentState state_;
    Aspose::Pdf::Text::Position position_{0.0, 0.0};

    // Write-back link (null doc_ => unlinked, Text setter only stores).
    Aspose::Pdf::Document* doc_ = nullptr;
    std::uint32_t stream_id_ = 0;
    std::size_t occurrence_ = 0;
    std::string original_literal_;
    std::size_t match_offset_ = 0;
    std::size_t match_len_ = 0;

    // Bounding box.
    double rx_ = 0.0;
    double ry_ = 0.0;
    double rw_ = 0.0;
    double rh_ = 0.0;
};

}  // namespace Aspose::Pdf::Text
