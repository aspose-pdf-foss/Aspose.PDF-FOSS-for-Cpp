#pragma once

// =============================================================================
// Aspose::Pdf::Text::FontRepository — font lookup. Mirrors canonical
// Aspose.Pdf.Text.FontRepository (Aspose.PDF 26.4.0) as a strict subset: v1
// ships the single-name FindFont(string) lookup used by the changeFont path.
// =============================================================================

#include <string>

#include <aspose/pdf/font.hpp>

namespace Aspose::Pdf::Text {

class FontRepository {
public:
    // Canonical FontRepository.FindFont(fontName) — resolve a font by name.
    static Aspose::Pdf::Text::Font FindFont(const std::string& fontName);

    // Canonical FontRepository.OpenFont(fontFilePath) — load a TrueType font
    // program from a file, returning an embeddable Font (IsEmbedded true, with
    // its FontName taken from the font). Text drawn with this font is embedded
    // (FontFile2) on Save. Throws std::runtime_error on an unreadable or
    // non-TrueType file.
    static Aspose::Pdf::Text::Font OpenFont(const std::string& fontFilePath);
};

}  // namespace Aspose::Pdf::Text
