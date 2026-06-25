// =============================================================================
// font_open_smoke_test — FontRepository::OpenFont. Loads a TrueType font from a
// file (real name, IsEmbedded), and text drawn with it is embedded (FontFile2)
// into the page so it renders in that font.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_state.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::Font;
using Aspose::Pdf::Text::FontRepository;
using Aspose::Pdf::Text::Position;
using Aspose::Pdf::Text::TextAbsorber;
using Aspose::Pdf::Text::TextBuilder;
using Aspose::Pdf::Text::TextFragment;

std::filesystem::path RepoRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}
std::string LiberationTtf() {
    return (RepoRoot() / "src" / "internal" / "standard14_outlines.fonts" /
            "LiberationSans-Regular.ttf")
        .string();
}
std::string HelloPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (RepoRoot() / "pdfs" / "hello_world.pdf").string();
}
std::string ReadAll(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

}  // namespace

// OpenFont loads a real, embeddable font.
TEST(FontOpenSmoke, LoadsEmbeddableFont) {
    Font f = FontRepository::OpenFont(LiberationTtf());
    EXPECT_TRUE(f.IsEmbedded());
    EXPECT_FALSE(f.FontName().empty());
}

// OpenFont throws on a missing file and on a non-font file.
TEST(FontOpenSmoke, ThrowsOnBadInput) {
    EXPECT_THROW(FontRepository::OpenFont("/no/such/font.ttf"),
                 std::runtime_error);
    EXPECT_THROW(FontRepository::OpenFont(HelloPdf()),  // a PDF, not a font
                 std::runtime_error);
}

// Text drawn with an opened font is embedded (FontFile2) and extractable.
TEST(FontOpenSmoke, EmbedsFontWhenDrawn) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "asp_openfont.pdf").string();
    {
        Document doc{HelloPdf()};
        Font f = FontRepository::OpenFont(LiberationTtf());
        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        TextFragment frag{"Embedded"};
        frag.TextState().Font(f);
        frag.TextState().FontSize(14.0f);
        frag.Position(Position{100.0, 500.0});
        builder.AppendText(frag);
        doc.Save(out);
    }
    // The saved PDF carries an embedded TrueType program.
    const std::string raw = ReadAll(out);
    EXPECT_NE(raw.find("FontFile2"), std::string::npos);
    EXPECT_NE(raw.find("/TrueType"), std::string::npos);
    // The drawn text is extractable.
    Document doc{out};
    TextAbsorber a;
    a.Visit(doc);
    EXPECT_NE(a.Text().find("Embedded"), std::string::npos);
    std::filesystem::remove(out);
}
