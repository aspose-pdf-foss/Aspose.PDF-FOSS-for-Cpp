#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextFragmentState — the per-fragment text state returned
// by TextFragment::TextState. Mirrors canonical
// Aspose.Pdf.Text.TextFragmentState (Aspose.PDF 26.4.0) as a strict subset:
// v1 ships the font-mutation surface (Font, FontSize) inherited from TextState.
//
// Setting Font / FontSize mutates the fragment's state in place. Persisting
// the change into the page content stream (a /Tf rewrite + font-resource
// injection) is a separate content-rewriter capability deferred from v1; the
// document still saves and reloads cleanly with the original glyphs.
// =============================================================================

#include <aspose/pdf/text_state.hpp>

namespace Aspose::Pdf::Text {

class TextFragment;

class TextFragmentState : public TextState {
public:
    TextFragmentState() = default;

private:
    explicit TextFragmentState(TextFragment* fragment) noexcept;

    // Non-owning link back to the owning fragment (null for a standalone
    // state). Reserved for content-stream write-back when that lands.
    TextFragment* fragment_ = nullptr;

    friend class TextFragment;
};

}  // namespace Aspose::Pdf::Text
