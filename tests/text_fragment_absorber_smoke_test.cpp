// =============================================================================
// text_fragment_absorber_smoke_test — Aspose::Pdf::Text::TextFragmentAbsorber
// phrase search. Mirrors pdflib's SearchText test intentions: a present
// phrase yields fragments; an absent phrase yields none.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>
#include <aspose/pdf/text_fragment_state.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextFragmentAbsorber;

std::string HelloWorldPdf() {
    // The text_extractor fixture reliably contains "Hello World".
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "hello_world.pdf")
        .string();
}

}  // namespace

TEST(TextFragmentAbsorber, FindsPresentPhrase) {
    Document doc{HelloWorldPdf()};  // contains "Hello World"
    TextFragmentAbsorber absorber{"World"};
    absorber.Visit(doc);

    auto& frags = absorber.TextFragments();
    ASSERT_EQ(frags.Count(), 1);
    EXPECT_EQ(frags[1].Text(), "World");  // 1-based indexer
}

TEST(TextFragmentAbsorber, AbsentPhraseYieldsNone) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{"Goodbye"};
    absorber.Visit(doc);
    EXPECT_EQ(absorber.TextFragments().Count(), 0);
}

TEST(TextFragmentAbsorber, EmptyPhraseYieldsNone) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{""};
    absorber.Visit(doc);
    EXPECT_EQ(absorber.TextFragments().Count(), 0);
}

TEST(TextFragmentAbsorber, VisitPageSearchesOnePage) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{"Hello"};
    absorber.Visit(doc.Pages()[1]);
    EXPECT_EQ(absorber.TextFragments().Count(), 1);
    EXPECT_EQ(absorber.TextFragments()[1].Text(), "Hello");
}

TEST(TextFragmentAbsorber, IndexerOutOfRangeThrows) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{"World"};
    absorber.Visit(doc);
    EXPECT_THROW(absorber.TextFragments()[2], std::out_of_range);
    EXPECT_THROW(absorber.TextFragments()[0], std::out_of_range);
}

std::filesystem::path TmpOut(const std::string& tag) {
    return std::filesystem::temp_directory_path() / ("asp_tfa_" + tag + ".pdf");
}

int CountMatches(const std::string& pdf, const std::string& phrase) {
    Document doc{pdf};
    TextFragmentAbsorber a{phrase};
    a.Visit(doc);
    return a.TextFragments().Count();
}

TEST(TextFragmentAbsorber, FragmentHasNonDegenerateRectangle) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{"World"};
    absorber.Visit(doc);
    ASSERT_EQ(absorber.TextFragments().Count(), 1);
    const auto rect = absorber.TextFragments()[1].Rectangle();
    EXPECT_GT(rect.Width(), 0.0);
    EXPECT_GT(rect.Height(), 0.0);
}

TEST(TextFragmentAbsorber, WorldIsRightOfHello) {
    Document doc{HelloWorldPdf()};  // "Hello World" in one show
    TextFragmentAbsorber h{"Hello"};
    h.Visit(doc);
    TextFragmentAbsorber w{"World"};
    w.Visit(doc);
    ASSERT_EQ(h.TextFragments().Count(), 1);
    ASSERT_EQ(w.TextFragments().Count(), 1);
    EXPECT_LT(h.TextFragments()[1].Rectangle().LLX(),
              w.TextFragments()[1].Rectangle().LLX());
}

TEST(TextFragmentAbsorber, DeleteFragmentRemovesText) {
    const auto out = TmpOut("del").string();
    {
        Document doc{HelloWorldPdf()};
        TextFragmentAbsorber absorber{"World"};
        absorber.Visit(doc);
        ASSERT_EQ(absorber.TextFragments().Count(), 1);
        absorber.TextFragments()[1].Text("");  // delete
        doc.Save(out);
    }
    EXPECT_EQ(CountMatches(out, "World"), 0);
    EXPECT_EQ(CountMatches(out, "Hello"), 1);  // the rest survives
    std::filesystem::remove(out);
}

TEST(TextFragmentAbsorber, ReplaceFragmentText) {
    const auto out = TmpOut("repl").string();
    {
        Document doc{HelloWorldPdf()};
        TextFragmentAbsorber absorber{"World"};
        absorber.Visit(doc);
        ASSERT_EQ(absorber.TextFragments().Count(), 1);
        absorber.TextFragments()[1].Text("Earth");
        doc.Save(out);
    }
    EXPECT_EQ(CountMatches(out, "World"), 0);
    EXPECT_EQ(CountMatches(out, "Earth"), 1);
    EXPECT_EQ(CountMatches(out, "Hello"), 1);
    std::filesystem::remove(out);
}

// T-4: the default (no-phrase) ctor is the canonical match-all mode —
// every positioned text run on the page is extracted as a fragment.
TEST(TextFragmentAbsorber, DefaultCtorExtractsAllRuns) {
    Document doc{HelloWorldPdf()};  // "Hello World" in one show
    TextFragmentAbsorber absorber;  // match-all
    absorber.Visit(doc);
    auto& frags = absorber.TextFragments();
    ASSERT_GE(frags.Count(), 1);
    // The whole run is captured, not a sub-phrase.
    EXPECT_NE(frags[1].Text().find("Hello"), std::string::npos);
}

// T-4: match-all fragments carry a non-degenerate Rectangle, a Position at
// the run origin, and the run's font size via TextState.
TEST(TextFragmentAbsorber, MatchAllFragmentsCarryPositionAndState) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber;
    absorber.Visit(doc);
    ASSERT_GE(absorber.TextFragments().Count(), 1);
    auto& frag = absorber.TextFragments()[1];

    const auto rect = frag.Rectangle();
    EXPECT_GT(rect.Width(), 0.0);
    EXPECT_GT(rect.Height(), 0.0);

    const auto pos = frag.Position();
    EXPECT_DOUBLE_EQ(pos.XIndent(), rect.LLX());
    EXPECT_DOUBLE_EQ(pos.YIndent(), rect.LLY());

    EXPECT_GT(frag.TextState().FontSize(), 0.0f);
}

TEST(TextFragmentAbsorber, MatchesAccumulateAcrossVisits) {
    Document doc{HelloWorldPdf()};
    TextFragmentAbsorber absorber{"o"};  // "Hello World" has 2 'o's
    absorber.Visit(doc);
    EXPECT_EQ(absorber.TextFragments().Count(), 2);
    absorber.Visit(doc);  // accumulates (canonical behaviour)
    EXPECT_EQ(absorber.TextFragments().Count(), 4);
}
