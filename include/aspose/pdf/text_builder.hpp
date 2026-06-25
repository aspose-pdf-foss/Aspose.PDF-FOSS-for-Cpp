#pragma once

// =============================================================================
// Aspose::Pdf::Text::TextBuilder — positioned text insertion. Mirrors
// canonical Aspose.Pdf.Text.TextBuilder (Aspose.PDF 26.4.0) as a strict
// subset: construct over a Page, then AppendText(fragment) draws the
// fragment's Text at its Position using its TextState font/size. The text is
// written into the page content stream and is extractable after save+reload.
// =============================================================================

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Text {

class TextFragment;
class TextParagraph;

class TextBuilder {
public:
    explicit TextBuilder(Aspose::Pdf::Page& page);

    // Canonical TextBuilder.AppendText(textFragment) — append the fragment's
    // text to the page at fragment.Position.
    void AppendText(Aspose::Pdf::Text::TextFragment& textFragment);

    // Canonical TextBuilder.AppendParagraph(textParagraph) — lay out the
    // paragraph's lines (word-wrapped to its Rectangle width, horizontally /
    // vertically aligned, with first / subsequent indents) and draw them onto
    // the page. Persisted on the owning Document's next Save().
    void AppendParagraph(Aspose::Pdf::Text::TextParagraph& textParagraph);

private:
    Aspose::Pdf::Page* page_ = nullptr;
};

}  // namespace Aspose::Pdf::Text
