#pragma once

// =============================================================================
// Aspose::Pdf::Text::Font — a resolved font. Mirrors canonical
// Aspose.Pdf.Text.Font (Aspose.PDF 26.4.0) as a strict subset: v1 ships
// FontName (the identity read used by the changeFont path). A Font is
// obtained from FontRepository::FindFont — it has no public constructor.
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Text {

class FontRepository;
class TextState;
class TextFragmentState;
class TextBuilder;
class TextParagraph;

class Font {
public:
    // Canonical Font.FontName — the font's name (e.g. "Helvetica").
    const std::string& FontName() const noexcept;

    // Canonical Font.BaseFont — the PostScript base-font name. For the
    // standard-14 / name-resolved fonts this is the font name.
    const std::string& BaseFont() const noexcept;

    // Canonical Font.IsEmbedded — whether the font program is embedded in the
    // document (get/set). False for name-resolved standard fonts.
    bool IsEmbedded() const noexcept;
    void IsEmbedded(bool value) noexcept;

    // Canonical Font.IsSubset — whether the embedded font is subsetted
    // (get/set). False for non-embedded fonts.
    bool IsSubset() const noexcept;
    void IsSubset(bool value) noexcept;

private:
    // Constructed by FontRepository::FindFont; default-constructed (empty)
    // as a stored member of TextState. Copyable.
    Font() = default;
    explicit Font(std::string name) noexcept;

    std::string font_name_;
    bool is_embedded_ = false;
    bool is_subset_ = false;
    // The font program (TrueType bytes) for a font loaded via
    // FontRepository::OpenFont; empty for name-resolved Standard-14 fonts.
    // Consumed by the text-draw path to embed the font (FontFile2).
    std::vector<std::byte> program_;

    friend class FontRepository;
    friend class TextState;
    friend class TextFragmentState;
    friend class TextBuilder;
    friend class TextParagraph;
    friend class Aspose::Pdf::Document;
};

}  // namespace Aspose::Pdf::Text
