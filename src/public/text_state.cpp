// Bodies for the text-state / font cluster: Font, TextState,
// TextFragmentState and FontRepository. Mirrors the canonical
// Aspose.Pdf.Text font-mutation surface used by the changeFont path.

#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/text_fragment_state.hpp>
#include <aspose/pdf/text_state.hpp>

#include <cstddef>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "standard14_metrics.hpp"
#include "truetype.hpp"

namespace Aspose::Pdf::Text {

// ---- Font ------------------------------------------------------------------

Font::Font(std::string name) noexcept : font_name_(std::move(name)) {}

const std::string& Font::FontName() const noexcept { return font_name_; }

const std::string& Font::BaseFont() const noexcept { return font_name_; }

bool Font::IsEmbedded() const noexcept { return is_embedded_; }
void Font::IsEmbedded(bool value) noexcept { is_embedded_ = value; }

bool Font::IsSubset() const noexcept { return is_subset_; }
void Font::IsSubset(bool value) noexcept { is_subset_ = value; }

// ---- TextState -------------------------------------------------------------

const Aspose::Pdf::Text::Font& TextState::Font() const noexcept {
    return font_;
}

void TextState::Font(const Aspose::Pdf::Text::Font& value) { font_ = value; }

float TextState::FontSize() const noexcept { return font_size_; }

void TextState::FontSize(float value) { font_size_ = value; }

const Aspose::Pdf::Color& TextState::ForegroundColor() const noexcept {
    return foreground_color_;
}
void TextState::ForegroundColor(const Aspose::Pdf::Color& value) {
    foreground_color_ = value;
}
const Aspose::Pdf::Color& TextState::BackgroundColor() const noexcept {
    return background_color_;
}
void TextState::BackgroundColor(const Aspose::Pdf::Color& value) {
    background_color_ = value;
}
const Aspose::Pdf::Color& TextState::StrokingColor() const noexcept {
    return stroking_color_;
}
void TextState::StrokingColor(const Aspose::Pdf::Color& value) {
    stroking_color_ = value;
}

bool TextState::Underline() const noexcept { return underline_; }
void TextState::Underline(bool value) { underline_ = value; }
bool TextState::StrikeOut() const noexcept { return strike_out_; }
void TextState::StrikeOut(bool value) { strike_out_ = value; }
bool TextState::Subscript() const noexcept { return subscript_; }
void TextState::Subscript(bool value) { subscript_ = value; }
bool TextState::Superscript() const noexcept { return superscript_; }
void TextState::Superscript(bool value) { superscript_ = value; }
bool TextState::Invisible() const noexcept { return invisible_; }
void TextState::Invisible(bool value) { invisible_ = value; }

float TextState::CharacterSpacing() const noexcept {
    return character_spacing_;
}
void TextState::CharacterSpacing(float value) { character_spacing_ = value; }
float TextState::WordSpacing() const noexcept { return word_spacing_; }
void TextState::WordSpacing(float value) { word_spacing_ = value; }
float TextState::LineSpacing() const noexcept { return line_spacing_; }
void TextState::LineSpacing(float value) { line_spacing_ = value; }
float TextState::HorizontalScaling() const noexcept {
    return horizontal_scaling_;
}
void TextState::HorizontalScaling(float value) { horizontal_scaling_ = value; }

Aspose::Pdf::HorizontalAlignment TextState::HorizontalAlignment()
        const noexcept {
    return horizontal_alignment_;
}
void TextState::HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value) {
    horizontal_alignment_ = value;
}

double TextState::MeasureString(const std::string& str) const {
    namespace s14 = foundation::standard14_metrics;
    const std::string& name = font_.FontName();
    const double fs = static_cast<double>(font_size_);

    // Fallback advance for glyphs the font's AFM lacks (and for unknown,
    // i.e. embedded, fonts): the font's own space width, or a generic 500.
    int fallback = s14::GetCharWidth(name, " ");
    if (fallback == 0) fallback = 500;

    double units = 0.0;   // accumulated advance in the 1000-unit em square
    std::size_t glyphs = 0;
    std::size_t spaces = 0;
    for (std::size_t i = 0; i < str.size();) {
        // Decode one UTF-8 codepoint's byte length from the lead byte.
        const unsigned char lead = static_cast<unsigned char>(str[i]);
        std::size_t len = 1;
        if ((lead & 0x80u) == 0x00u) len = 1;
        else if ((lead & 0xE0u) == 0xC0u) len = 2;
        else if ((lead & 0xF0u) == 0xE0u) len = 3;
        else if ((lead & 0xF8u) == 0xF0u) len = 4;
        if (i + len > str.size()) len = 1;  // truncated tail: treat as 1 byte
        // Malformed sequence (bad continuation byte): degrade to one byte so a
        // following real character is not silently swallowed.
        for (std::size_t k = 1; k < len; ++k) {
            if ((static_cast<unsigned char>(str[i + k]) & 0xC0u) != 0x80u) {
                len = 1;
                break;
            }
        }
        const std::string_view cp(str.data() + i, len);
        int w = s14::GetCharWidth(name, cp);
        if (w == 0) w = fallback;
        units += w;
        ++glyphs;
        if (len == 1 && lead == ' ') ++spaces;
        i += len;
    }

    double width = units / 1000.0 * fs;
    width += static_cast<double>(character_spacing_) * static_cast<double>(glyphs);
    width += static_cast<double>(word_spacing_) * static_cast<double>(spaces);
    width *= static_cast<double>(horizontal_scaling_) / 100.0;
    return width;
}

// ---- TextFragmentState -----------------------------------------------------

TextFragmentState::TextFragmentState(TextFragment* fragment) noexcept
    : fragment_(fragment) {}

// ---- FontRepository --------------------------------------------------------

Aspose::Pdf::Text::Font FontRepository::FindFont(const std::string& fontName) {
    return Aspose::Pdf::Text::Font(fontName);
}

Aspose::Pdf::Text::Font FontRepository::OpenFont(
        const std::string& fontFilePath) {
    std::ifstream f(fontFilePath, std::ios::binary);
    if (!f)
        throw std::runtime_error(
            "Aspose::Pdf::Text::FontRepository::OpenFont: cannot open '" +
            fontFilePath + "'");
    const std::string raw((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i)
        bytes[i] = static_cast<std::byte>(static_cast<unsigned char>(raw[i]));

    // Validate it is a TrueType font and read its canonical name.
    foundation::truetype::TrueType tt;
    try {
        tt = foundation::truetype::Parse(
            std::span<const std::byte>(bytes.data(), bytes.size()));
    } catch (const std::exception&) {
        throw std::runtime_error(
            "Aspose::Pdf::Text::FontRepository::OpenFont: '" + fontFilePath +
            "' is not a supported TrueType font");
    }
    std::string name = foundation::truetype::ToCanonical(tt);
    if (name.empty()) name = "EmbeddedFont";

    Aspose::Pdf::Text::Font font(name);
    font.is_embedded_ = true;
    font.program_ = std::move(bytes);
    return font;
}

}  // namespace Aspose::Pdf::Text
