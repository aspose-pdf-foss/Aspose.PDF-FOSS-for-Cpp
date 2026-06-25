// =============================================================================
// outlines_persistence_smoke_test — beat N-4b of the navigation cluster. Wires
// the public OutlineCollection tree through Document::Save (writing /Outlines
// with Title, /F bold/italic flags, /Count open/closed sign, the typed /Dest
// verb, and /A /URI). Proves the full round-trip: build → save → reopen → read.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/annotations/go_to_uri_action.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/outlines.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

}  // namespace

TEST(OutlinesPersistenceSmoke, SaveAndReopenRoundTrip) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_outlines_save.pdf")
            .string();

    // 1. Build an outline tree: a bold/closed Chapter 1 (XYZ dest on page 1)
    //    with a child section, a Chapter 2 (Fit dest on page 2), and a web
    //    link node.
    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        OutlineCollection& root = doc.Outlines();

        OutlineItemCollection ch1{root};
        ch1.Title("Chapter 1");
        ch1.Bold(true);
        ch1.Open(false);
        ch1.Destination(XYZExplicitDestination{1, 100.0, 700.0, 2.0});
        root.Add(ch1);

        OutlineItemCollection sec{root};
        sec.Title("Section 1.1");
        sec.Italic(true);
        root[1].Add(sec);

        OutlineItemCollection ch2{root};
        ch2.Title("Chapter 2");
        ch2.Destination(XYZExplicitDestination{2, 0.0, 0.0, 1.0});
        root.Add(ch2);

        OutlineItemCollection web{root};
        web.Title("Homepage");
        web.Action(GoToURIAction{"https://example.org"});
        root.Add(web);

        doc.Save(out);
    }

    // 2. Reopen — the tree loads back from /Outlines.
    {
        Document doc{out};
        OutlineCollection& root = doc.Outlines();
        ASSERT_EQ(root.Count(), 3);

        OutlineItemCollection& ch1 = root[1];
        EXPECT_EQ(ch1.Title(), "Chapter 1");
        EXPECT_TRUE(ch1.Bold());
        EXPECT_FALSE(ch1.Italic());
        EXPECT_FALSE(ch1.Open());  // /Count negative
        ASSERT_NE(ch1.Destination(), nullptr);
        EXPECT_EQ(ch1.Destination()->ToString(), "1 XYZ 100 700 2");

        ASSERT_EQ(ch1.Count(), 1);
        OutlineItemCollection* sec = ch1.First();
        ASSERT_NE(sec, nullptr);
        EXPECT_EQ(sec->Title(), "Section 1.1");
        EXPECT_TRUE(sec->Italic());

        EXPECT_EQ(root[2].Title(), "Chapter 2");
        EXPECT_NE(root[2].Destination(), nullptr);

        OutlineItemCollection& web = root[3];
        EXPECT_EQ(web.Title(), "Homepage");
        ASSERT_NE(web.Action(), nullptr);
        EXPECT_EQ(web.Action()->ToString(), "https://example.org");
    }

    std::filesystem::remove(out);
}
