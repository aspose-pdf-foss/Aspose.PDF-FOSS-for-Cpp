// =============================================================================
// text_absorber_smoke_test — exercises Aspose::Pdf::Text::TextAbsorber
// end-to-end. Open a real PDF, run Visit(Document), assert the
// extracted text matches the canonical-derived expected string.
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures";
}

std::string ReadTextFile(const std::filesystem::path& p) {
    std::ifstream f(p);
    std::stringstream ss;
    ss << f.rdbuf();
    auto s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

}  // namespace

TEST(TextAbsorberSmoke, VisitDocument_ExtractsExpectedText) {
    const auto pdf = FixtureRoot() / "text_extractor" / "encoding_winansi.pdf";
    const auto exp = FixtureRoot() / "text_extractor" / "encoding_winansi.expected";
    ASSERT_TRUE(std::filesystem::exists(pdf)) << pdf;
    ASSERT_TRUE(std::filesystem::exists(exp)) << exp;

    Aspose::Pdf::Document doc(pdf.string());
    Aspose::Pdf::Text::TextAbsorber absorber;
    absorber.Visit(doc);

    const std::string expected = ReadTextFile(exp);
    EXPECT_EQ(absorber.Text(), expected);
    EXPECT_FALSE(absorber.HasErrors());
}

TEST(TextAbsorberSmoke, FreshAbsorber_StartsEmpty) {
    Aspose::Pdf::Text::TextAbsorber absorber;
    EXPECT_EQ(absorber.Text(), "");
    EXPECT_FALSE(absorber.HasErrors());
}
