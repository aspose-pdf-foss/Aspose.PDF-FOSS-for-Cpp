// =============================================================================
// font_subset_smoke_test — Font.IsSubset (E6). When an opened TrueType font is
// embedded with IsSubset, only the used glyph outlines are kept, so the
// embedded program (and the saved file) is much smaller than a full embed,
// while the text still renders / extracts and the font stays valid.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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
std::string Ttf() {
    return (RepoRoot() / "src" / "internal" / "standard14_outlines.fonts" /
            "LiberationSans-Regular.ttf")
        .string();
}
std::string HelloPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (RepoRoot() / "pdfs" / "hello_world.pdf").string();
}

// Draw "Hi" with the opened font (optionally subset) and return the saved size.
std::uintmax_t DrawAndSave(bool subset, const std::string& out) {
    Document doc{HelloPdf()};
    Font f = FontRepository::OpenFont(Ttf());
    f.IsSubset(subset);
    Page page = doc.Pages()[1];
    TextBuilder builder{page};
    TextFragment frag{"Hi"};
    frag.TextState().Font(f);
    frag.TextState().FontSize(14.0f);
    frag.Position(Position{100.0, 500.0});
    builder.AppendText(frag);
    doc.Save(out);
    return std::filesystem::file_size(out);
}

std::string ReadAll(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

}  // namespace

// Subsetting yields a substantially smaller embed than the full font.
TEST(FontSubsetSmoke, SubsetShrinksEmbed) {
    const std::string full =
        (std::filesystem::temp_directory_path() / "asp_font_full.pdf").string();
    const std::string sub =
        (std::filesystem::temp_directory_path() / "asp_font_sub.pdf").string();
    const std::uintmax_t full_size = DrawAndSave(false, full);
    const std::uintmax_t sub_size = DrawAndSave(true, sub);

    EXPECT_LT(sub_size, full_size);
    // The subset BaseFont carries the canonical 6-letter '+' tag.
    EXPECT_NE(ReadAll(sub).find("+"), std::string::npos);
    EXPECT_NE(ReadAll(sub).find("FontFile2"), std::string::npos);

    std::filesystem::remove(full);
    std::filesystem::remove(sub);
}

// The subset font is still valid and the text round-trips.
TEST(FontSubsetSmoke, SubsetTextStillExtracts) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "asp_font_subx.pdf").string();
    DrawAndSave(true, out);
    Document doc{out};
    TextAbsorber a;
    a.Visit(doc);
    EXPECT_NE(a.Text().find("Hi"), std::string::npos);
    std::filesystem::remove(out);
}
